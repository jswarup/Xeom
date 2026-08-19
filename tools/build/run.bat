@echo off
setlocal

pushd "%~dp0"
set "TOOLS_BUILD_DIR=%CD%"
cd ..
cd ..
set "PROJ_DIR=%CD%"
popd

set "EXE_RELEASE=%PROJ_DIR%\out\clang-ninja-release\bin\xeom.exe"
set "EXE_DEBUG=%PROJ_DIR%\out\clang-ninja-debug\bin\xeom.exe"
set "EXE_CLANG_CL=%PROJ_DIR%\out\clang-cl-release\bin\xeom.exe"

if exist "%EXE_RELEASE%" goto run_release
if exist "%EXE_DEBUG%" goto run_debug
if exist "%EXE_CLANG_CL%" goto run_clang_cl

echo [WARN] No built executable found. Building first...
call "%TOOLS_BUILD_DIR%\build.bat" --release
if errorlevel 1 exit /b 1

if exist "%EXE_RELEASE%" goto run_release
exit /b 1

:run_release
echo [RUN] Executing Xeom - Release
"%EXE_RELEASE%" %*
exit /b %ERRORLEVEL%

:run_debug
echo [RUN] Executing Xeom - Debug
"%EXE_DEBUG%" %*
exit /b %ERRORLEVEL%

:run_clang_cl
echo [RUN] Executing Xeom - Clang-CL Release
"%EXE_CLANG_CL%" %*
exit /b %ERRORLEVEL%
