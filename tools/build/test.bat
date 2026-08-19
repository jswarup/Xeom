@echo off
setlocal

pushd "%~dp0"
set "TOOLS_BUILD_DIR=%CD%"
cd ..
cd ..
set "PROJ_DIR=%CD%"
popd

set "TEST_EXE_RELEASE=%PROJ_DIR%\out\clang-ninja-release\bin\xeom_tests.exe"
set "TEST_EXE_DEBUG=%PROJ_DIR%\out\clang-ninja-debug\bin\xeom_tests.exe"
set "TEST_EXE_CLANG_CL=%PROJ_DIR%\out\clang-cl-release\bin\xeom_tests.exe"

if exist "%TEST_EXE_RELEASE%" goto test_release
if exist "%TEST_EXE_DEBUG%" goto test_debug
if exist "%TEST_EXE_CLANG_CL%" goto test_clang_cl

echo [WARN] No built test binary found. Building with tests...
call "%TOOLS_BUILD_DIR%\build.bat" --release --test
exit /b %ERRORLEVEL%

:test_release
echo [TEST] Executing Xeom Tests - Release
"%TEST_EXE_RELEASE%" %*
exit /b %ERRORLEVEL%

:test_debug
echo [TEST] Executing Xeom Tests - Debug
"%TEST_EXE_DEBUG%" %*
exit /b %ERRORLEVEL%

:test_clang_cl
echo [TEST] Executing Xeom Tests - Clang-CL Release
"%TEST_EXE_CLANG_CL%" %*
exit /b %ERRORLEVEL%
