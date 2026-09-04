@echo off
title TFT180 Simulator

cd /d "%~dp0"
if not exist "%~dp0main.exe" (
    echo [INFO] main.exe not found, building now...
    call "%~dp0build.bat"
    if %errorlevel% neq 0 exit /b %errorlevel%
)

"%~dp0main.exe"