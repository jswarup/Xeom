@echo off
:: Shared by run.bat/test.bat — sets XEOM_EXE to the first built xeom.exe found across presets. Call, don't run directly.
call "%~dp0_paths.bat"

set "XEOM_EXE="
for %%P in (clang-ninja-release clang-ninja-debug clang-cl-release) do (
    if exist "%PROJ_DIR%\out\%%P\bin\xeom.exe" (
        set "XEOM_EXE=%PROJ_DIR%\out\%%P\bin\xeom.exe"
        goto :found
    )
)
:found
