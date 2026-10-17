#Requires -Version 5.1
# Run with: powershell -NoProfile -File Tools/tests/play.Tests.ps1
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$scriptPath = Join-Path (Split-Path -Parent $PSScriptRoot) 'play.ps1'
$tokens = $null
$parseErrors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile(
    $scriptPath, [ref]$tokens, [ref]$parseErrors
)
if ($parseErrors.Count) {
    throw ($parseErrors | Out-String)
}
# Load only functions so the real installer, menu and platform guard do not execute.
$definitions = $ast.FindAll({
    param($node)
    $node -is [System.Management.Automation.Language.FunctionDefinitionAst]
}, $false)
$loadFunctions = [scriptblock]::Create(($definitions.Extent.Text -join "`n"))

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Throws {
    param([scriptblock]$Body, [string]$Pattern)
    $caught = $false
    try {
        & $Body
    } catch {
        $caught = $true
        Assert-True ($_.Exception.Message -like $Pattern) "Unexpected error: $_"
    }
    Assert-True $caught "Expected an error matching: $Pattern"
}

function Test-Case {
    param([string]$Name, [scriptblock]$Body)
    . $loadFunctions
    & $Body
    Write-Host "PASS: $Name"
}

$temporary = Join-Path ([IO.Path]::GetTempPath()) ("wowdecomp-play-" + [guid]::NewGuid())
New-Item -ItemType Directory -Path $temporary | Out-Null
try {
    $RepoRoot = Join-Path $temporary 'repo with spaces'
    $BuildPath = Join-Path $RepoRoot 'build/x64'
    $OutputPath = Join-Path $RepoRoot 'bin/Win'
    $ClientPath = ''

    Test-Case 'All runs setup, build and launch in order' {
        $calls = [Collections.Generic.List[string]]::new()
        function Initialize-Environment { $calls.Add('setup') }
        function Build-Client { $calls.Add('build') }
        function Start-Client { $calls.Add('run') }
        Invoke-Action All
        Assert-True (($calls -join ',') -eq 'setup,build,run') 'Wrong action order'
    }
    Test-Case 'A failed build prevents launch' {
        function Initialize-Environment {}
        function Build-Client { throw 'build failed' }
        function Start-Client { throw 'must not launch' }
        Assert-Throws { Invoke-Action All } 'build failed'
    }
    Test-Case 'Setup does not build or launch' {
        function Initialize-Environment {}
        function Build-Client { throw 'must not build' }
        function Start-Client { throw 'must not launch' }
        Invoke-Action Setup
        Assert-Throws { Invoke-Action Unknown } 'Unknown action:*'
    }
    Test-Case 'Native failures are not reported as success' {
        function Fake-Native { $global:LASTEXITCODE = 7 }
        Assert-Throws { Invoke-Checked Fake-Native @() } '*exit code 7*'
    }
    Test-Case 'Setup reuses an installed environment' {
        function Get-VisualStudio { 'C:\VS 2022' }
        function Get-CMake { 'cmake.exe' }
        function Get-WindowsSdk { $true }
        function Install-BuildTools { throw 'must not install' }
        function Invoke-Checked {}
        Initialize-Environment
    }
    Test-Case 'Build requires a complete environment' {
        function Get-VisualStudio { 'C:\VS 2022' }
        function Get-CMake { 'cmake.exe' }
        function Get-WindowsSdk { $false }
        Assert-Throws { Get-BuildEnvironment } '*Choose 2*'
    }
    Test-Case 'Setup installs missing tools and verifies the result' {
        $state = @{ Ready = $false; Installs = 0 }
        function Get-VisualStudio { if ($state.Ready) { 'C:\VS 2022' } }
        function Get-CMake { if ($state.Ready) { 'cmake.exe' } }
        function Get-WindowsSdk { $state.Ready }
        function Install-BuildTools { $state.Installs++; $state.Ready = $true }
        function Invoke-Checked {}
        Initialize-Environment
        Assert-True ($state.Installs -eq 1) 'Expected one installation'
    }
    Test-Case 'Declining installation makes no system changes' {
        function Get-Command { $null }
        function Read-Host { 'n' }
        function Start-Process { throw 'must not install' }
        function Invoke-Checked { throw 'must not install' }
        Assert-Throws { Install-BuildTools } '*Installation cancelled*'
    }
    Test-Case 'Build uses x64 Release and preserves paths with spaces' {
        function Get-BuildEnvironment {
            @{ VisualStudio = 'C:\VS 2022'; CMake = 'cmake.exe' }
        }
        $calls = [Collections.Generic.List[object]]::new()
        function Invoke-Checked {
            param($Command, $Arguments)
            $calls.Add($Arguments)
        }
        function Test-Path { $true }
        Build-Client
        Assert-True ($calls.Count -eq 2) 'Expected configure and build'
        Assert-True ($calls[0][1] -eq $RepoRoot) 'Source path split or changed'
        Assert-True ($calls[0][3] -eq $BuildPath) 'Build path split or changed'
        Assert-True ($calls[0] -contains 'x64') 'Not targeting x64'
        Assert-True ($calls[0] -contains '-DWOWDECOMP_DISABLE_SIGNATURE_CHECKS=OFF') 'Signature checks changed'
        Assert-True ($calls[1] -contains 'Release') 'Not building Release'
        Assert-True ($calls[1] -contains 'WowClient') 'Wrong target'
    }
    Test-Case 'Missing build output fails explicitly' {
        function Get-BuildEnvironment { @{ VisualStudio = 'VS'; CMake = 'cmake.exe' } }
        function Invoke-Checked {}
        function Test-Path { $false }
        Assert-Throws { Build-Client } '*without the expected executable*'
    }
    Test-Case 'Configure failure prevents compilation' {
        function Get-BuildEnvironment { @{ VisualStudio = 'VS'; CMake = 'cmake.exe' } }
        $state = @{ Calls = 0 }
        function Invoke-Checked { $state.Calls++; throw 'configure failed' }
        Assert-Throws { Build-Client } 'configure failed'
        Assert-True ($state.Calls -eq 1) 'Compilation ran after configure failed'
    }
    Test-Case 'Run refuses a missing executable' {
        function Test-Path { $false }
        function Initialize-GameData { throw 'must not prepare data' }
        function Start-Process { throw 'must not launch' }
        Assert-Throws { Start-Client } '*was not found*'
    }
    Test-Case 'Run sets the executable directory as its working directory' {
        function Test-Path { $true }
        function Initialize-GameData {}
        function Start-Process {
            param($FilePath, $WorkingDirectory)
            Assert-True ($FilePath -eq (Join-Path $OutputPath 'WowClient64.exe')) 'Wrong executable'
            Assert-True ($WorkingDirectory -eq $OutputPath) 'Wrong working directory'
        }
        Start-Client
    }
    Test-Case 'Invalid game data prevents launch' {
        function Test-Path { $true }
        function Initialize-GameData { throw 'invalid data' }
        function Start-Process { throw 'must not launch' }
        Assert-Throws { Start-Client } 'invalid data'
    }
    Test-Case 'An existing Data directory is preserved' {
        function Test-Path { $true }
        function New-Item { throw 'must not replace existing data' }
        Initialize-GameData
    }
    Test-Case 'Cancelling data selection fails explicitly' {
        function Test-Path { $false }
        function Read-Host { '' }
        Assert-Throws { Initialize-GameData } '*No game data selected*'
    }
    Test-Case 'Folders without MPQ files are rejected' {
        $ClientPath = Join-Path $temporary 'empty-client'
        New-Item -ItemType Directory -Path $ClientPath | Out-Null
        Assert-Throws { Initialize-GameData } '*No MPQ files found*'
    }
    Test-Case 'Client paths with spaces link Data without copying files' {
        $ClientPath = Join-Path $temporary 'client with spaces'
        $sourceData = Join-Path $ClientPath 'Data'
        New-Item -ItemType Directory -Path $sourceData -Force | Out-Null
        New-Item -ItemType File -Path (Join-Path $sourceData 'common.MPQ') | Out-Null
        $links = [Collections.Generic.List[string]]::new()
        function New-Item {
            param($ItemType, $Path, $Value, [switch]$Force)
            if ($ItemType -eq 'Junction') {
                Assert-True ($Path -eq (Join-Path $OutputPath 'Data')) 'Wrong junction path'
                Assert-True ($Value -eq $sourceData) 'Wrong source data folder'
                $links.Add($Path)
            } else {
                Microsoft.PowerShell.Management\New-Item -ItemType $ItemType -Path $Path -Force:$Force
            }
        }
        Initialize-GameData
        Assert-True ($links.Count -eq 1) 'Expected exactly one junction'
    }
    Write-Host 'All launcher checks passed.'
} finally {
    Remove-Item -LiteralPath $temporary -Recurse -Force
}
