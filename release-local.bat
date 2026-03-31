@echo off
setlocal EnableExtensions

pushd "%~dp0" >nul || exit /b 1
npm run release:zip
set "EXIT_CODE=%ERRORLEVEL%"
popd

exit /b %EXIT_CODE%
