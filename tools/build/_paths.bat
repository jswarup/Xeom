@echo off
:: Shared by build.bat/run.bat/test.bat — sets TOOLS_BUILD_DIR and PROJ_DIR. Call, don't run directly.
pushd "%~dp0"
set "TOOLS_BUILD_DIR=%CD%"
cd ..\..
set "PROJ_DIR=%CD%"
popd
