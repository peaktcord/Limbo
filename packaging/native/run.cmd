@echo off
pushd "%~dp0"
limbo_sdl.exe --scale 3 %*
set "exit_code=%errorlevel%"
popd
if not "%exit_code%"=="0" pause
exit /b %exit_code%
