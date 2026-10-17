#Requires -Version 5.1
[CmdletBinding()]
param(
    [ValidateSet('Menu', 'All', 'Setup', 'Build', 'Run')]
    [string]$Action = 'Menu',
    [string]$ClientPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot
$BuildPath = Join-Path $RepoRoot 'build/x64'
$OutputPath = Join-Path $RepoRoot 'bin/Win'

function Invoke-Checked {
    param([string]$Command, [string[]]$Arguments)
    & $Command @Arguments | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "$Command failed (exit code $LASTEXITCODE). See the output above."
    }
}

function Get-VisualStudio {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (Test-Path -LiteralPath $vswhere -PathType Leaf) {
        $installation = & $vswhere -latest -products '*' -version '[17.0,18.0)' `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($LASTEXITCODE -ne 0) {
            throw "vswhere failed (exit code $LASTEXITCODE)."
        }
        if ($installation) {
            return [string]$installation
        }
    }
    return $null
}

function Get-WindowsSdk {
    $key = Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots' `
        -ErrorAction SilentlyContinue
    if ($key -and $key.PSObject.Properties['KitsRoot10']) {
        $includePath = Join-Path $key.KitsRoot10 'Include'
        if (Test-Path -LiteralPath $includePath -PathType Container) {
            foreach ($version in Get-ChildItem -LiteralPath $includePath -Directory) {
                $header = Join-Path $version.FullName 'um/Windows.h'
                $library = Join-Path $key.KitsRoot10 "Lib/$($version.Name)/um/x64/kernel32.lib"
                $runtime = Join-Path $key.KitsRoot10 "Lib/$($version.Name)/ucrt/x64/ucrt.lib"
                if ((Test-Path -LiteralPath $header -PathType Leaf) -and
                    (Test-Path -LiteralPath $library -PathType Leaf) -and
                    (Test-Path -LiteralPath $runtime -PathType Leaf)) {
                    return $true
                }
            }
        }
    }
    return $false
}

function Get-CMake {
    param([string]$VisualStudio)
    # Prefer VS's bundled CMake; a new installation need not be on PATH yet.
    $candidates = @()
    if ($VisualStudio) {
        $candidates += Join-Path $VisualStudio 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
    }
    $candidates += Join-Path $env:ProgramFiles 'CMake/bin/cmake.exe'
    $command = Get-Command cmake.exe -CommandType Application -ErrorAction SilentlyContinue
    if ($command) {
        $candidates += $command.Source
    }
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return $candidate
        }
    }
    return $null
}

function Install-BuildTools {
    $winget = Get-Command winget.exe -CommandType Application -ErrorAction SilentlyContinue
    Write-Host 'VS 2022 C++ Build Tools, a Windows SDK and CMake are required.'
    Write-Host 'This may download several GB and show an administrator approval prompt.'
    if ((Read-Host 'Install/repair these tools now? [y/N]') -notmatch '^(?i:y|yes)$') {
        throw 'Installation cancelled. No build was started.'
    }

    $installer = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/setup.exe'
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $existing = $null
    if (Test-Path -LiteralPath $vswhere -PathType Leaf) {
        $instances = & $vswhere -latest -products '*' -version '[17.0,18.0)' -format json
        if ($LASTEXITCODE -ne 0) {
            throw "vswhere failed (exit code $LASTEXITCODE)."
        }
        $existing = $instances | ConvertFrom-Json | Select-Object -First 1
    }
    if ($existing -and (Test-Path -LiteralPath $installer -PathType Leaf)) {
        # The installer, unlike winget install, can add components to an existing VS.
        $workload = 'Microsoft.VisualStudio.Workload.NativeDesktop'
        if ($existing.productId -eq 'Microsoft.VisualStudio.Product.BuildTools') {
            $workload = 'Microsoft.VisualStudio.Workload.VCTools'
        }
        $arguments = 'modify --installPath "{0}" --add {1} --add Microsoft.VisualStudio.Component.VC.CMake.Project --includeRecommended --passive --norestart' -f $existing.installationPath, $workload
        $process = Start-Process -FilePath $installer -ArgumentList $arguments -Wait -PassThru
        if ($process.ExitCode -eq 3010) {
            throw 'Build tools were installed, but Windows must restart. Restart and run this script again.'
        }
        if ($process.ExitCode -ne 0) {
            throw "Visual Studio Installer failed (exit code $($process.ExitCode))."
        }
    } else {
        if (-not $winget) {
            throw 'Install App Installer (winget), or install VS 2022 Desktop development with C++ and CMake manually. Then retry.'
        }
        $override = '--wait --passive --norestart --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.CMake.Project --includeRecommended'
        Invoke-Checked $winget.Source @(
            'install', '--id', 'Microsoft.VisualStudio.2022.BuildTools', '--exact',
            '--source', 'winget', '--override', $override
        )
    }
}

function Get-BuildEnvironment {
    $visualStudio = Get-VisualStudio
    $cmake = Get-CMake $visualStudio
    if (-not $visualStudio -or -not $cmake -or -not (Get-WindowsSdk)) {
        throw 'Build tools are incomplete. Choose 2 (setup) first: VS 2022 C++ tools, Windows SDK and CMake are required.'
    }
    return @{ VisualStudio = $visualStudio; CMake = $cmake }
}

function Initialize-Environment {
    $visualStudio = Get-VisualStudio
    if (-not $visualStudio -or -not (Get-CMake $visualStudio) -or -not (Get-WindowsSdk)) {
        Install-BuildTools
    }
    $tools = Get-BuildEnvironment
    Invoke-Checked $tools.CMake @('--version')
    Write-Host "Build environment ready: $($tools.VisualStudio)"
}

function Build-Client {
    $tools = Get-BuildEnvironment
    Invoke-Checked $tools.CMake @(
        '-S', $RepoRoot, '-B', $BuildPath, '-G', 'Visual Studio 17 2022', '-A', 'x64',
        "-DCMAKE_GENERATOR_INSTANCE=$($tools.VisualStudio)",
        '-DWOWCLIENT_OUTPUT_OVERRIDE=', '-DWOWCLIENT_CRASH_LOG=ON',
        '-DWOWDECOMP_DISABLE_SIGNATURE_CHECKS=OFF'
    )
    Invoke-Checked $tools.CMake @('--build', $BuildPath, '--config', 'Release', '--target', 'WowClient', '--parallel')
    $executable = Join-Path $OutputPath 'WowClient64.exe'
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        throw "Build finished without the expected executable: $executable"
    }
    Write-Host "Build ready: $executable"
}

function Initialize-GameData {
    $dataPath = Join-Path $OutputPath 'Data'
    if (Test-Path -LiteralPath $dataPath -PathType Container) {
        return
    }
    if (Test-Path -LiteralPath $dataPath) {
        throw "$dataPath exists but is not a directory. Move it manually before retrying."
    }
    $source = $ClientPath
    if (-not $source) {
        Write-Host 'Game data is not included. Use your own WoW 3.3.5a (12340) client.'
        $source = Read-Host 'Path to the client folder or its Data folder (blank to cancel)'
    }
    if ([string]::IsNullOrWhiteSpace($source)) {
        throw 'No game data selected. The client was not started.'
    }
    $source = $source.Trim().Trim('"')
    if (Test-Path -LiteralPath (Join-Path $source 'Data') -PathType Container) {
        $source = Join-Path $source 'Data'
    }
    if (-not (Test-Path -LiteralPath $source -PathType Container)) {
        throw "Game data folder does not exist: $source"
    }
    $source = (Get-Item -LiteralPath $source).FullName
    $mpq = Get-ChildItem -LiteralPath $source -Filter '*.MPQ' -File -Recurse |
        Select-Object -First 1
    if (-not $mpq) {
        throw "No MPQ files found in $source. Select a WoW 3.3.5a (12340) Data folder."
    }
    New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null
    # A junction avoids copying the multi-GB Data folder and needs no symlink privilege.
    New-Item -ItemType Junction -Path $dataPath -Value $source | Out-Null
    Write-Host "Linked game data: $dataPath -> $source"
}

function Start-Client {
    $executable = Join-Path $OutputPath 'WowClient64.exe'
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        throw 'WowClient64.exe was not found. Choose 3 (build) or 1 (all) first.'
    }
    Initialize-GameData
    Write-Host 'Using your existing Data/<locale>/realmlist.wtf; server settings are not changed.'
    Start-Process -FilePath $executable -WorkingDirectory $OutputPath | Out-Null
    Write-Host 'Client launched.'
}

function Invoke-Action {
    param([string]$SelectedAction)
    switch ($SelectedAction) {
        'All' { Initialize-Environment; Build-Client; Start-Client }
        'Setup' { Initialize-Environment }
        'Build' { Build-Client }
        'Run' { Start-Client }
        default { throw "Unknown action: $SelectedAction" }
    }
}

try {
    if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT) {
        throw 'wowdecomp currently targets Windows 10/11 only. Native macOS builds are not supported.'
    }
    if (-not [Environment]::Is64BitOperatingSystem) {
        throw 'This launcher builds the x64 client and requires 64-bit Windows.'
    }
    if ($Action -ne 'Menu') {
        Invoke-Action $Action
    } else {
        while ($true) {
            Write-Host "`n=== wowdecomp (Windows x64 / Release) ==="
            Write-Host '1. Setup, build and run'
            Write-Host '2. Setup/check build environment'
            Write-Host '3. Build client'
            Write-Host '4. Run client'
            Write-Host '0. Exit'
            $choice = Read-Host 'Select a number'
            if ($choice -eq '0') { break }
            $selected = switch ($choice) {
                '1' { 'All' }
                '2' { 'Setup' }
                '3' { 'Build' }
                '4' { 'Run' }
            }
            if (-not $selected) {
                Write-Host 'Invalid selection. Enter 0, 1, 2, 3 or 4.'
                continue
            }
            try {
                Invoke-Action $selected
            } catch {
                Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
            }
        }
    }
} catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}
