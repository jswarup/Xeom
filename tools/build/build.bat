@echo off
setlocal

echo ===================================================================
echo   Xeom Build Engine - Clang 20+ / CMake Toolchain
echo ===================================================================

:: Resolve directory paths
pushd "%~dp0"
set "TOOLS_BUILD_DIR=%CD%"
cd ..
cd ..
set "PROJ_DIR=%CD%"
popd

:: Default configurations
set BUILD_TYPE=Release
set PRESET=clang-ninja-release
set DO_CLEAN=0
set DO_TEST=0
set USE_CLANG_CL=0
set USE_VS=0

:: Parse arguments
:parse_args
if "%~1"=="" goto done_args
if /i "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    set PRESET=clang-ninja-debug
    shift
    goto parse_args
)
if /i "%~1"=="--release" (
    set BUILD_TYPE=Release
    set PRESET=clang-ninja-release
    shift
    goto parse_args
)
if /i "%~1"=="--clang-cl" (
    set USE_CLANG_CL=1
    shift
    goto parse_args
)
if /i "%~1"=="--vs" (
    set USE_VS=1
    set PRESET=vs2026-clang
    shift
    goto parse_args
)
if /i "%~1"=="--clean" (
    set DO_CLEAN=1
    shift
    goto parse_args
)
if /i "%~1"=="--test" (
    set DO_TEST=1
    shift
    goto parse_args
)
shift
goto parse_args
:done_args

if "%USE_CLANG_CL%"=="1" (
    if "%BUILD_TYPE%"=="Debug" (
        set PRESET=clang-cl-debug
    ) else (
        set PRESET=clang-cl-release
    )
)

:: Locate Visual Studio environment
set VSCMD_SKIP_SENDTELEMETRY=1
set VCVARS="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"

if not exist %VCVARS% (
    echo [ERROR] Visual Studio vcvarsall.bat not found at %VCVARS%
    exit /b 1
)

echo [INFO] Initializing Visual Studio x64 environment...
call %VCVARS% x64 > nul
if errorlevel 1 (
    echo [ERROR] Failed to initialize x64 environment.
    exit /b 1
)

:: Ensure CMake and Ninja are on PATH
set VS_CMAKE_DIR=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin
set VS_NINJA_DIR=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja
set VS_LLVM_DIR=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin

set "PATH=%VS_CMAKE_DIR%;%VS_NINJA_DIR%;%VS_LLVM_DIR%;%PATH%"

set "BUILD_DIR=%PROJ_DIR%\out\%PRESET%"

if "%DO_CLEAN%"=="1" (
    echo [INFO] Cleaning build directory: %BUILD_DIR%
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

echo [INFO] Toolchain Verification:
cmake --version | findstr /i "cmake version"
ninja --version
clang++ --version | findstr /i "clang version"

cd /d "%TOOLS_BUILD_DIR%"

echo.
echo [INFO] Configuring CMake with Preset '%PRESET%'...
cmake --preset %PRESET%
if errorlevel 1 (
    cd /d "%PROJ_DIR%"
    echo [ERROR] CMake configuration failed!
    exit /b 1
)

echo.
echo [INFO] Building targets (Build Type: %BUILD_TYPE%)...
cmake --build "%BUILD_DIR%"
if errorlevel 1 (
    cd /d "%PROJ_DIR%"
    echo [ERROR] Build failed!
    exit /b 1
)

cd /d "%PROJ_DIR%"

echo.
echo [SUCCESS] Build completed successfully!
echo [INFO] Executable available at: %BUILD_DIR%\bin\xeom.exe

if "%DO_TEST%"=="1" (
    echo.
    echo [INFO] Running test suite...
    ctest --test-dir "%BUILD_DIR%" --output-on-failure
)
