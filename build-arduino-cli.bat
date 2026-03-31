@echo off
setlocal EnableExtensions

if "%ARDUINO_CLI%"=="" set ARDUINO_CLI=arduino-cli
if "%ARDUINO_CONFIG_FILE%"=="" set ARDUINO_CONFIG_FILE=".\arduino-cli.yaml"
if "%~2"=="" (
    set "ARDUINO_FQBN=arduino:avr:uno"
) else (
    set "ARDUINO_FQBN=%~2"
)

rem handling the FQBN in firmware\sketch.yaml
rem %ARDUINO_CLI% --config-file %ARDUINO_CONFIG_FILE% compile --fqbn Arduino_STM32:STM32F1:genericSTM32F103C:upload_method=STLinkMethod firmware
%ARDUINO_CLI% --config-file %ARDUINO_CONFIG_FILE% compile --fqbn "%ARDUINO_FQBN%" --verbose %1
