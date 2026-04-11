@echo off
setlocal EnableExtensions

for %%I in ("%~dp0.") do set "WORKSPACE_LIBRARY_ROOT=%%~fI"
if "%ARDUINO_CLI%"=="" for %%P in (
    "%LocalAppData%\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "%ProgramFiles%\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "%ProgramFiles(x86)%\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
) do if not defined ARDUINO_CLI if exist "%%~P" set "ARDUINO_CLI=%%~P"
if "%ARDUINO_CLI%"=="" for /f "usebackq delims=" %%P in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "$drives = [System.IO.DriveInfo]::GetDrives() | Where-Object { $_.DriveType -eq [System.IO.DriveType]::Fixed -and $_.IsReady }; foreach ($drive in $drives) { foreach ($dir in @('Program Files', 'Program Files (x86)')) { $candidate = Join-Path $drive.Name ($dir + '\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'); if (Test-Path -LiteralPath $candidate) { Write-Output $candidate; exit 0 } } }"`) do if not defined ARDUINO_CLI set "ARDUINO_CLI=%%~P"
if exist "%LocalAppData%\Arduino15\libraries" set "ARDUINO_IDE_LIBRARIES=%LocalAppData%\Arduino15\libraries"
if "%ARDUINO_CLI%"=="" set ARDUINO_CLI=arduino-cli
if "%ARDUINO_CONFIG_FILE%"=="" set ARDUINO_CONFIG_FILE=.\arduino-cli.yaml
set "ARDUINO_CLI=%ARDUINO_CLI:"=%"
set "ARDUINO_CONFIG_FILE=%ARDUINO_CONFIG_FILE:"=%"
set "WORKSPACE_LIBRARY_ROOT=%WORKSPACE_LIBRARY_ROOT:"=%"
set "ARDUINO_IDE_LIBRARIES=%ARDUINO_IDE_LIBRARIES:"=%"
if "%~2"=="" (
    set "ARDUINO_FQBN=arduino:avr:uno"
) else (
    set "ARDUINO_FQBN=%~2"
)

echo Arduino CLI: "%ARDUINO_CLI%"
echo Arduino CLI config: "%ARDUINO_CONFIG_FILE%"
echo Workspace library: "%WORKSPACE_LIBRARY_ROOT%"
if defined ARDUINO_IDE_LIBRARIES echo Arduino IDE libraries: "%ARDUINO_IDE_LIBRARIES%"
echo FQBN: %ARDUINO_FQBN%

rem handling the FQBN in firmware\sketch.yaml
rem %ARDUINO_CLI% --config-file %ARDUINO_CONFIG_FILE% compile --fqbn Arduino_STM32:STM32F1:genericSTM32F103C:upload_method=STLinkMethod firmware
if defined ARDUINO_IDE_LIBRARIES (
    "%ARDUINO_CLI%" --config-file "%ARDUINO_CONFIG_FILE%" compile --library "%WORKSPACE_LIBRARY_ROOT%" --libraries "%ARDUINO_IDE_LIBRARIES%" --fqbn "%ARDUINO_FQBN%" --verbose "%~1"
) else (
    "%ARDUINO_CLI%" --config-file "%ARDUINO_CONFIG_FILE%" compile --library "%WORKSPACE_LIBRARY_ROOT%" --fqbn "%ARDUINO_FQBN%" --verbose "%~1"
)
goto :eof
