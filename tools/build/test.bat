@echo off
setlocal

call "%~dp0_locate_exe.bat"
if not defined XEOM_EXE (
    echo [WARN] No built executable found. Building with tests...
    call "%TOOLS_BUILD_DIR%\build.bat" --release --test
    exit /b %ERRORLEVEL%
)

echo [TEST] Executing Xeom Jeeves Tests
"%XEOM_EXE%" --test %*
exit /b %ERRORLEVEL%
