@echo off
title TFT180 Simulator Build

cd /d "%~dp0"
echo ==================================================
echo         Building TFT180 Simulator...
echo ==================================================

if exist "%~dp0compiler\bin\gcc.exe" (
    set "PATH=%~dp0compiler\bin;%PATH%"
    set "GCC_CMD=%~dp0compiler\bin\gcc.exe"
) else (
    set "GCC_CMD=gcc"
)

"%GCC_CMD%" -fdiagnostics-color=always -g -O2 -Wno-misleading-indentation -I. -Dsimulator main.c image.c zf_device_tft180.c zf_common_font.c zf_common_function.c avilib.c read_bmp.c -o main.exe -lgdi32 -luser32

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build failed! Check errors above.
    pause
    exit /b %errorlevel%
)

echo.
echo [SUCCESS] main.exe built successfully!
echo ==================================================