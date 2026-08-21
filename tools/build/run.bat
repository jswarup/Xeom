@echo off
setlocal

call "%~dp0_locate_exe.bat"
if not defined XEOM_EXE (
    echo [WARN] No built executable found. Building first...
    call "%TOOLS_BUILD_DIR%\build.bat" --release
    if errorlevel 1 exit /b 1
    call "%~dp0_locate_exe.bat"
)
if not defined XEOM_EXE (
    echo [ERROR] Build succeeded but xeom.exe was not found.
    exit /b 1
)

echo [RUN] Executing Xeom
"%XEOM_EXE%" %*
exit /b %ERRORLEVEL%
