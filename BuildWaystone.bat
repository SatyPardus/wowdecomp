@echo off
setlocal

set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "BUILD=%ROOT%\out\build\x64-Release"
set "BIN=%ROOT%\bin"
set "STAGE=%TEMP%\waystone_pkg"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

for /f "usebackq delims=" %%i in (`git -C "%ROOT%" rev-parse --short^=7 HEAD`) do set "HASH=%%i"
if not defined HASH goto :nohash

git -C "%ROOT%" diff --quiet HEAD
if errorlevel 1 echo WARNING: uncommitted changes, the zip will not match commit %HASH%

if not exist "%VSWHERE%" goto :novs
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -version [17.0^,18.0^) -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR goto :novs

set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 goto :novs

if exist "%BUILD%\build.ninja" goto :build
cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -S "%ROOT%" -B "%BUILD%"
if errorlevel 1 goto :fail

:build
cmake --build "%BUILD%" --target Wow XvidDecoder
if errorlevel 1 goto :fail

if not exist "%BIN%\Win\Wow64.exe" goto :missing
if not exist "%BIN%\Win\Wow64.map" goto :missing
if not exist "%BIN%\Win\XvidDecoder.dll" goto :missing

if exist "%STAGE%" rmdir /s /q "%STAGE%"
mkdir "%STAGE%"
copy /y "%BIN%\Win\Wow64.exe" "%STAGE%\Waystone.exe" >nul
copy /y "%BIN%\Win\Wow64.map" "%STAGE%\Waystone.map" >nul
copy /y "%BIN%\Win\XvidDecoder.dll" "%STAGE%\XvidDecoder.dll" >nul

set "ZIP=%BIN%\Waystone-%HASH%.zip"
powershell -NoProfile -Command "Compress-Archive -Path '%STAGE%\*' -DestinationPath '%ZIP%' -Force"
if errorlevel 1 goto :fail
rmdir /s /q "%STAGE%"

echo Created %ZIP%
exit /b 0

:nohash
echo ERROR: could not read the git commit hash
exit /b 1

:novs
echo ERROR: Visual Studio 2022 with the C++ x64 tools was not found
exit /b 1

:missing
echo ERROR: build output missing in %BIN%\Win
exit /b 1

:fail
echo ERROR: build or packaging failed
exit /b 1
