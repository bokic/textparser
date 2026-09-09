@echo off
setlocal EnableDelayedExpansion

REM usage: build.bat [x64|arm64]  (no argument builds both arches)

set "ARCH=%~1"
if "%ARCH%"=="" set "ARCH=both"

set "ROOT_DIR=%~dp0.."
set "BIN_DIR=%ROOT_DIR%\bin"
set "CLANG_DIR=C:\Program Files\LLVM\bin"

set "PATH=C:\Program Files\CMake\bin;%CLANG_DIR%;%LOCALAPPDATA%\Microsoft\WinGet\Links;%LOCALAPPDATA%\Programs\Python\Python313;%PATH%"

if /i "%ARCH%"=="both" (
    for %%A in (x64 arm64) do (
        call :build_arch %%A
        if errorlevel 1 (
            echo [ERROR] build failed for %%A
            exit /b 1
        )
    )
) else (
    if /i "%ARCH%"=="x64" (
        call :build_arch x64
    ) else if /i "%ARCH%"=="arm64" (
        call :build_arch arm64
    ) else (
        echo [ERROR] Invalid arch "%ARCH%". Use x64 or arm64.
        exit /b 1
    )
    if errorlevel 1 (
        echo [ERROR] build failed for %ARCH%
        exit /b 1
    )
)

echo [OK] build (%ARCH%) done.
exit /b 0

:build_arch
set "BUILD_ARCH=%~1"
set "BUILD_DIR=%~dp0build-%BUILD_ARCH%"
set "ARCH_BIN_DIR=%BIN_DIR%\%BUILD_ARCH%"

if exist "%BUILD_DIR%\" rmdir /s /q "%BUILD_DIR%"
if not exist "%ARCH_BIN_DIR%\" mkdir "%ARCH_BIN_DIR%"

REM CMake's Windows targets link against dependency files in the shared bin
REM directory. Stage the selected architecture's dependencies before building.
copy /y "%ARCH_BIN_DIR%\json-c.dll" "%BIN_DIR%" >nul || (
    echo [ERROR] Missing %BUILD_ARCH% dependency json-c.dll. Run build_deps.bat %BUILD_ARCH% first.
    exit /b 1
)
copy /y "%ARCH_BIN_DIR%\json-c.lib" "%BIN_DIR%" >nul || exit /b 1
for %%F in (pcre2-8 pcre2-16 pcre2-32 pcre2-posix) do (
    copy /y "%ARCH_BIN_DIR%\%%F.dll" "%BIN_DIR%" >nul || exit /b 1
    copy /y "%ARCH_BIN_DIR%\%%F.lib" "%BIN_DIR%" >nul || exit /b 1
)

if /i "%BUILD_ARCH%"=="x64" (
    set "EXTRA_ARGS="
) else (
    set "EXTRA_ARGS=-DCMAKE_C_COMPILER_TARGET=aarch64-pc-windows-msvc -DCMAKE_SYSTEM_PROCESSOR=ARM64"
)

cmake.exe -S "%ROOT_DIR%" -B "%BUILD_DIR%" -G "Ninja" -DCMAKE_C_COMPILER="%CLANG_DIR%\clang.exe" -DCMAKE_CXX_COMPILER="%CLANG_DIR%\clang++.exe" -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release %EXTRA_ARGS% || (
    echo [ERROR] Failed to configure textparser for %BUILD_ARCH%.
    exit /b 1
)

cmake.exe --build "%BUILD_DIR%" --config Release || (
    echo [ERROR] Failed to build textparser for %BUILD_ARCH%.
    exit /b 1
)

for %%F in (ccat.exe textparser.exe textparser.dll textparser.lib textparser-json.dll textparser-json.lib) do (
    copy /y "%BIN_DIR%\%%F" "%ARCH_BIN_DIR%" >nul || (
        echo [ERROR] Missing build output %%F for %BUILD_ARCH%.
        exit /b 1
    )
)

copy /y "%BUILD_DIR%\compile_commands.json" "%ROOT_DIR%" >nul
if exist "%BUILD_DIR%\" rmdir /s /q "%BUILD_DIR%"

exit /b 0
