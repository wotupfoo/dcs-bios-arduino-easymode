@echo off
setlocal EnableExtensions

pushd "%~dp0" >nul || exit /b 1

set "REMOVE_DIST=0"
if /I "%~1"=="--all" set "REMOVE_DIST=1"

echo Cleaning repo-local temporary files...

for %%P in (
    ".tmp-*"
    ".tmp*"
) do (
    for /D %%D in (%%~P) do (
        if exist "%%~fD" (
            echo Removing directory %%~nxD
            rmdir /S /Q "%%~fD"
        )
    )

    for %%F in (%%~P) do (
        if exist "%%~fF" (
            if not exist "%%~fF\" (
                echo Removing file %%~nxF
                del /F /Q "%%~fF"
            )
        )
    )
)

if "%REMOVE_DIST%"=="1" (
    if exist "dist" (
        echo Removing directory dist
        rmdir /S /Q "dist"
    )
)

echo Cleanup complete.

popd
exit /b 0
