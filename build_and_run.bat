@echo off
title Build and Run TFT180 Simulator

cd /d "%~dp0"
call "%~dp0build.bat"
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo [LAUNCH] Starting simulator window...
echo.
"%~dp0main.exe"