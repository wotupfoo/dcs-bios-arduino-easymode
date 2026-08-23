@echo off
setlocal EnableExtensions

node "%~dp0scripts\package-firmware-release.mjs" %*
