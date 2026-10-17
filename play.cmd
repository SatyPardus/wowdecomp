@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\play.ps1" %*
set "result=%errorlevel%"
if not "%result%"=="0" pause
exit /b %result%
