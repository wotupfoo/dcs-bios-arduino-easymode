@echo off
if "%1"=="" echo Provide the name of the sub-directory you want to Compile and Upload
if "%ARDUINO_CLI%"=="" call :find_arduino_cli
call :find_arduino_ide_libraries
if "%ARDUINO_CLI%"=="" set ARDUINO_CLI=arduino-cli
if "%ARDUINO_CONFIG_FILE%"=="" set ARDUINO_CONFIG_FILE=.\arduino-cli.yaml
set "ARDUINO_CLI=%ARDUINO_CLI:"=%"
set "ARDUINO_CONFIG_FILE=%ARDUINO_CONFIG_FILE:"=%"
set "ARDUINO_IDE_LIBRARIES=%ARDUINO_IDE_LIBRARIES:"=%"

rem handling the FQBN in firmware\sketch.yaml
rem %ARDUINO_CLI% --config-file %ARDUINO_CONFIG_FILE% compile --fqbn Arduino_STM32:STM32F1:genericSTM32F103C:upload_method=STLinkMethod firmware
echo Arduino CLI: "%ARDUINO_CLI%"
echo Arduino CLI config: "%ARDUINO_CONFIG_FILE%"
if defined ARDUINO_IDE_LIBRARIES echo Arduino IDE libraries: "%ARDUINO_IDE_LIBRARIES%"
echo Uploading %1
"%ARDUINO_CLI%" --config-file "%ARDUINO_CONFIG_FILE%" upload --verbose "%~1" -b "%~2" -p "%~3"
echo Done
goto :eof

:find_arduino_cli
for %%P in (
    "%LocalAppData%\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "%ProgramFiles%\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "%ProgramFiles(x86)%\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "E:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
) do (
    if exist "%%~P" (
        set "ARDUINO_CLI=%%~P"
        goto :eof
    )
)
goto :eof

:find_arduino_ide_libraries
if exist "%LocalAppData%\Arduino15\libraries" (
    set "ARDUINO_IDE_LIBRARIES=%LocalAppData%\Arduino15\libraries"
)
goto :eof
