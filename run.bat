@echo off
setlocal
cd /d "%~dp0"

echo Starting SysCmon System Suite...
if not exist "SysCmon.exe" (
    echo SysCmon.exe not detected. Building now...
    call build.bat
    if errorlevel 1 (
        echo [ERROR] Failed to build SysCmon.
        pause
        exit /b 1
    )
)

start "" "%~dp0SysCmon.exe"
exit /b 0
