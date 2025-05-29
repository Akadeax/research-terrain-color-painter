@echo off
setlocal EnableDelayedExpansion

REM =============================
REM Step 1: Locate UE5.5 Installation Directory via Registry
REM =============================
set "UE_INSTALL_DIR="
for /f "tokens=2*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.5" /v InstalledDirectory 2^>nul') do (
    set "UE_INSTALL_DIR=%%B"
)

if "%UE_INSTALL_DIR%"=="" (
    echo ERROR: Unreal Engine 5.5 installation not found in registry.
    pause
    exit /b 1
)

REM =============================
REM Step 2: Locate the .uproject File in the Current Folder
REM =============================
set "PROJECT_FILE="
for %%i in (*.uproject) do (
    set "PROJECT_FILE=%%i"
)
if not defined PROJECT_FILE (
    echo ERROR: No .uproject file found in the current directory.
    pause
    exit /b 1
)
echo Found project file: %PROJECT_FILE%

REM Extract project name from the .uproject filename (without extension)
for %%I in ("%PROJECT_FILE%") do set "PROJECT_NAME=%%~nI"
echo Project name: %PROJECT_NAME%

REM =============================
REM Set the Project Directory
REM =============================
set "PROJECT_DIR=%~dp0"

REM =============================
REM Build the Project's Editor Target
REM =============================
echo Building %PROJECT_NAME%...
call "%UE_INSTALL_DIR%\Engine\Build\BatchFiles\Build.bat" %PROJECT_NAME%Editor Win64 Development "%PROJECT_DIR%%PROJECT_FILE%" -waitmutex

if ERRORLEVEL 1 (
    echo ======== Build failed. Exiting. ======== 
    pause
    exit /b 1
)

REM =============================
REM Launch the Unreal Editor with the Project
REM =============================
echo Launching Unreal Editor...
start "" "%UE_INSTALL_DIR%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT_DIR%%PROJECT_FILE%"

endlocal