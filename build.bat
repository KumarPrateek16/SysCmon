@echo off
setlocal enabledelayedexpansion
title Building SysCmon Suite...

cd /d "%~dp0"
echo ==============================================================
echo              SysCmon - Build Pipeline (MinGW GCC)
echo ==============================================================

echo [1/3] Compiling Application Resources...
windres -I"src" -I"assets" "src\resource.rc" -O coff -o "src\resource.o"
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Resource compilation failed!
    exit /b %ERRORLEVEL%
)

echo [2/3] Compiling SysCmon.exe...
gcc -O3 -mwindows -Wall src\main.c src\metrics.c src\render.c src\resource.o -o SysCmon.exe -luser32 -lgdi32 -lmsimg32 -ldwmapi -lcomctl32 -liphlpapi -lpsapi -lm
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] SysCmon.exe compilation failed!
    exit /b %ERRORLEVEL%
)
echo [OK] SysCmon.exe built successfully.

if exist "installer\installer.c" (
    echo [3/3] Compiling Standalone Installer SysCmon_Setup.exe...
    gcc -O2 -mwindows -Wall installer\installer.c src\resource.o -o SysCmon_Setup.exe -luser32 -lgdi32 -lshell32 -lole32 -luuid -lcomctl32 -ldwmapi
    if %ERRORLEVEL% NEQ 0 (
        echo [WARNING] Installer compilation warning, continuing...
    ) else (
        echo [OK] SysCmon_Setup.exe built successfully.
    )
)

echo ==============================================================
echo                   Build Completed Successfully!
echo ==============================================================
exit /b 0
