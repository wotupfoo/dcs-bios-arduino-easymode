@echo off
setlocal EnableExtensions

pushd "%~dp0" >nul || exit /b 1

set "REMOVE_DIST=0"
if /I "%~1"=="--all" set "REMOVE_DIST=1"

echo Cleaning repo-local temporary files...
set "POWERSHELL_EXE=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
"%POWERSHELL_EXE%" -NoProfile -Command ^
  "Get-ChildItem -Force -LiteralPath '.' | Where-Object { $_.Name -like '.tmp*' } | ForEach-Object { if ($_.PSIsContainer) { Write-Host ('Removing directory ' + $_.Name); Remove-Item -LiteralPath $_.FullName -Recurse -Force } else { Write-Host ('Removing file ' + $_.Name); Remove-Item -LiteralPath $_.FullName -Force } }"

if "%REMOVE_DIST%"=="1" (
    if exist "dist" (
        echo Removing directory dist
        rmdir /S /Q "dist"
    )
)

echo Cleanup complete.

popd
exit /b 0
