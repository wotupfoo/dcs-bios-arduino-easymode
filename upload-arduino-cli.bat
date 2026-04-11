@echo off
setlocal EnableExtensions EnableDelayedExpansion

if "%~1"=="" (
    echo Provide the sketch directory to compile and upload.
    echo Usage: %~nx0 ^<sketch-dir^> [fqbn] ^<port^>
    exit /b 1
)

for %%I in ("%~dp0.") do set "WORKSPACE_LIBRARY_ROOT=%%~fI"
if "%ARDUINO_CLI%"=="" for %%P in (
    "%LocalAppData%\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "%ProgramFiles%\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "%ProgramFiles(x86)%\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
) do if not defined ARDUINO_CLI if exist "%%~P" set "ARDUINO_CLI=%%~P"
if "%ARDUINO_CLI%"=="" for /f "usebackq delims=" %%P in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "$drives = [System.IO.DriveInfo]::GetDrives() | Where-Object { $_.DriveType -eq [System.IO.DriveType]::Fixed -and $_.IsReady }; foreach ($drive in $drives) { foreach ($dir in @('Program Files', 'Program Files (x86)')) { $candidate = Join-Path $drive.Name ($dir + '\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'); if (Test-Path -LiteralPath $candidate) { Write-Output $candidate; exit 0 } } }"`) do if not defined ARDUINO_CLI set "ARDUINO_CLI=%%~P"
if exist "%LocalAppData%\Arduino15\libraries" set "ARDUINO_IDE_LIBRARIES=%LocalAppData%\Arduino15\libraries"
if "%ARDUINO_CLI%"=="" set "ARDUINO_CLI=arduino-cli"
if "%ARDUINO_CONFIG_FILE%"=="" set "ARDUINO_CONFIG_FILE=.\arduino-cli.yaml"
set "ARDUINO_CLI=%ARDUINO_CLI:"=%"
set "ARDUINO_CONFIG_FILE=%ARDUINO_CONFIG_FILE:"=%"
set "WORKSPACE_LIBRARY_ROOT=%WORKSPACE_LIBRARY_ROOT:"=%"
set "ARDUINO_IDE_LIBRARIES=%ARDUINO_IDE_LIBRARIES:"=%"
set "SKETCH_PATH=%~1"

call :normalize_arguments "%~2" "%~3" "%~4"
call :resolve_fqbn "%~1" "%NORMALIZED_FQBN%"
if not defined ARDUINO_FQBN (
    echo Unable to determine FQBN for "%~1".
    exit /b 1
)

call :resolve_port "%NORMALIZED_FQBN%" "%NORMALIZED_PORT%"
if not defined ARDUINO_PORT (
    echo Provide the serial port as the last argument.
    echo Usage: %~nx0 ^<sketch-dir^> [fqbn] ^<port^>
    exit /b 1
)

for %%I in ("%~1") do set "SKETCH_NAME=%%~nxI"
set "BUILD_ROOT=%WORKSPACE_LIBRARY_ROOT%\.arduino-build\%SKETCH_NAME%"
set "BUILD_PATH=%BUILD_ROOT%\upload-%RANDOM%%RANDOM%"
if not exist "%BUILD_ROOT%" mkdir "%BUILD_ROOT%" >nul 2>&1
if not exist "%BUILD_PATH%" mkdir "%BUILD_PATH%" >nul 2>&1

echo Arduino CLI: "%ARDUINO_CLI%"
echo Arduino CLI config: "%ARDUINO_CONFIG_FILE%"
echo Workspace library: "%WORKSPACE_LIBRARY_ROOT%"
if defined ARDUINO_IDE_LIBRARIES echo Arduino IDE libraries: "%ARDUINO_IDE_LIBRARIES%"
echo Sketch: "%SKETCH_PATH%"
echo FQBN: %ARDUINO_FQBN%
echo Port: %ARDUINO_PORT%
echo Build path: "%BUILD_PATH%"

call :compile_sketch
if errorlevel 1 exit /b %errorlevel%

call :upload_with_arduino_cli
if errorlevel 1 exit /b %errorlevel%

echo Done
goto :eof

:compile_sketch
if defined ARDUINO_IDE_LIBRARIES (
    "%ARDUINO_CLI%" --config-file "%ARDUINO_CONFIG_FILE%" compile --library "%WORKSPACE_LIBRARY_ROOT%" --libraries "%ARDUINO_IDE_LIBRARIES%" --build-path "%BUILD_PATH%" --fqbn "%ARDUINO_FQBN%" --verbose "%SKETCH_PATH%"
) else (
    "%ARDUINO_CLI%" --config-file "%ARDUINO_CONFIG_FILE%" compile --library "%WORKSPACE_LIBRARY_ROOT%" --build-path "%BUILD_PATH%" --fqbn "%ARDUINO_FQBN%" --verbose "%SKETCH_PATH%"
)
exit /b %errorlevel%

:upload_with_arduino_cli
echo Uploading with board-defined upload recipe via arduino-cli.
"%ARDUINO_CLI%" --config-file "%ARDUINO_CONFIG_FILE%" upload --input-dir "%BUILD_PATH%" --fqbn "%ARDUINO_FQBN%" -p "%ARDUINO_PORT%" --verbose "%SKETCH_PATH%"
exit /b %errorlevel%

:normalize_arguments
set "NORMALIZED_FQBN=%~1"
set "NORMALIZED_PORT=%~2"
if not "%~3"=="" (
    set "NORMALIZED_FQBN=%~1=%~2"
    set "NORMALIZED_PORT=%~3"
)
exit /b 0

:resolve_fqbn
set "ARDUINO_FQBN="
set "SECOND_ARG=%~2"
if not "%~2"=="" (
    echo(!SECOND_ARG!| findstr /C:":" >nul
    if not errorlevel 1 (
        set "ARDUINO_FQBN=%~2"
    )
)
if defined ARDUINO_FQBN exit /b 0

if exist "%~1\sketch.yaml" (
    for /f "usebackq tokens=1,* delims=:" %%A in (`findstr /B /C:"default_fqbn:" "%~1\sketch.yaml"`) do (
        set "ARDUINO_FQBN=%%B"
        set "ARDUINO_FQBN=!ARDUINO_FQBN: =!"
    )
)
if not defined ARDUINO_FQBN set "ARDUINO_FQBN=arduino:avr:uno"
exit /b 0

:resolve_port
set "ARDUINO_PORT="
set "SECOND_ARG=%~1"
if not "%~2"=="" (
    set "ARDUINO_PORT=%~2"
) else if not "%~1"=="" (
    echo(!SECOND_ARG!| findstr /C:":" >nul
    if errorlevel 1 set "ARDUINO_PORT=%~1"
)
exit /b 0
