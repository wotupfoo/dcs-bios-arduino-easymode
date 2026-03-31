@echo off
setlocal EnableExtensions EnableDelayedExpansion

pushd "%~dp0" >nul || exit /b 1

set "BUILD_SCRIPT=%CD%\build-all-examples.bat"
if not exist "%BUILD_SCRIPT%" (
    echo build-all-examples.bat was not found beside this script.
    popd
    exit /b 1
)

set "CORE_LIST_FILE=%TEMP%\dcsbios-easymode-core-list-%RANDOM%-%RANDOM%.txt"
call :resolve_arduino_cli
call :resolve_sketchbook_dir

"%RESOLVED_ARDUINO_CLI%" --config-file "%RESOLVED_ARDUINO_CONFIG_FILE%" core list > "%CORE_LIST_FILE%"
if errorlevel 1 (
    echo Failed to query installed Arduino cores.
    if exist "%CORE_LIST_FILE%" del /f /q "%CORE_LIST_FILE%" >nul 2>&1
    popd
    exit /b 1
)

set /a TESTED_COUNT=0
set /a SKIPPED_COUNT=0
set /a FAILED_COUNT=0

call :test_board "Arduino Uno" "arduino:avr:uno" "arduino:avr"
call :test_board "Arduino Mega 2560" "arduino:avr:mega" "arduino:avr"
call :test_board "Arduino Nano" "arduino:avr:nano:cpu=atmega328" "arduino:avr"
call :test_board "STM32F103 Generic" "Arduino_STM32:STM32F1:genericSTM32F103C:upload_method=STLinkMethod" "Arduino_STM32:STM32F1" "rogerclark"

echo(
echo === Matrix Summary ===
echo Tested: !TESTED_COUNT!
echo Skipped: !SKIPPED_COUNT!
echo Failed: !FAILED_COUNT!

if exist "%CORE_LIST_FILE%" del /f /q "%CORE_LIST_FILE%" >nul 2>&1
popd

if !FAILED_COUNT! NEQ 0 (
    echo OVERALL RESULT: FAIL
    exit /b 1
)

echo OVERALL RESULT: PASS
exit /b 0

:resolve_arduino_cli
set "RESOLVED_ARDUINO_CLI=%ARDUINO_CLI%"
set "RESOLVED_ARDUINO_CONFIG_FILE=%ARDUINO_CONFIG_FILE%"

if "%RESOLVED_ARDUINO_CLI%"=="" call :find_arduino_cli
if "%RESOLVED_ARDUINO_CLI%"=="" set "RESOLVED_ARDUINO_CLI=arduino-cli"
if "%RESOLVED_ARDUINO_CONFIG_FILE%"=="" set "RESOLVED_ARDUINO_CONFIG_FILE=.\arduino-cli.yaml"

set "RESOLVED_ARDUINO_CLI=%RESOLVED_ARDUINO_CLI:"=%"
set "RESOLVED_ARDUINO_CONFIG_FILE=%RESOLVED_ARDUINO_CONFIG_FILE:"=%"
goto :eof

:find_arduino_cli
for %%P in (
    "%LocalAppData%\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "%ProgramFiles%\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "%ProgramFiles(x86)%\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "E:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
) do (
    if exist "%%~P" (
        set "RESOLVED_ARDUINO_CLI=%%~P"
        goto :eof
    )
)
goto :eof

:test_board
set "BOARD_NAME=%~1"
set "BOARD_FQBN=%~2"
set "BOARD_CORE=%~3"
set "BOARD_DETECT=%~4"

if /I "%BOARD_DETECT%"=="rogerclark" (
    if not exist "%SKETCHBOOK_DIR%\hardware\Arduino_STM32\STM32F1" (
        echo(
        echo === Skipping %BOARD_NAME% ===
        echo Missing Roger Clark core: %SKETCHBOOK_DIR%\hardware\Arduino_STM32\STM32F1
        set /a SKIPPED_COUNT+=1
        goto :eof
    )
    goto test_board_run
)

findstr /b /c:"%BOARD_CORE% " "%CORE_LIST_FILE%" >nul
if errorlevel 1 (
    echo(
    echo === Skipping %BOARD_NAME% ===
    echo Missing installed core: %BOARD_CORE%
    set /a SKIPPED_COUNT+=1
    goto :eof
)

:test_board_run
echo(
echo === Testing %BOARD_NAME% ===
echo FQBN: %BOARD_FQBN%
set /a TESTED_COUNT+=1
call "%BUILD_SCRIPT%" "%BOARD_FQBN%"
if errorlevel 1 (
    echo RESULT: FAIL - %BOARD_NAME%
    set /a FAILED_COUNT+=1
    goto :eof
)

echo RESULT: PASS - %BOARD_NAME%
goto :eof

:resolve_sketchbook_dir
set "SKETCHBOOK_DIR="
for /f "usebackq tokens=1,* delims=:" %%A in ("%RESOLVED_ARDUINO_CONFIG_FILE%") do (
    if /I "%%~A"=="user" set "SKETCHBOOK_DIR=%%~B"
)

if not "%SKETCHBOOK_DIR%"=="" goto trim_sketchbook

for /f "usebackq tokens=1,* delims==" %%A in ("%RESOLVED_ARDUINO_CONFIG_FILE%") do (
    if /I "%%~A"=="directories.user" set "SKETCHBOOK_DIR=%%~B"
)

if "%SKETCHBOOK_DIR%"=="" goto :eof

:trim_sketchbook
for /f "tokens=* delims= " %%A in ("!SKETCHBOOK_DIR!") do set "SKETCHBOOK_DIR=%%~A"
set "SKETCHBOOK_DIR=!SKETCHBOOK_DIR:"=!"
goto :eof
