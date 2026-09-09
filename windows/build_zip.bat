@echo off
setlocal EnableDelayedExpansion

REM usage: build_zip.bat [x64|arm64]  (no argument builds zips for both arches)

set "ARCH=%~1"
if "%ARCH%"=="" set "ARCH=both"

if /i "%ARCH%"=="both" (
    for %%A in (x64 arm64) do (
        call :make_zip %%A
        if errorlevel 1 (
            echo [ERROR] zip failed for %%A
            exit /b 1
        )
    )
) else (
    if /i "%ARCH%"=="x64" (
        call :make_zip x64
    ) else if /i "%ARCH%"=="arm64" (
        call :make_zip arm64
    ) else (
        echo [ERROR] Invalid arch "%ARCH%". Use x64 or arm64.
        exit /b 1
    )
    if errorlevel 1 (
        echo [ERROR] zip failed for %ARCH%
        exit /b 1
    )
)

echo [OK] zip (%ARCH%) done.
exit /b 0

:make_zip
set "ZIP_ARCH=%~1"
set "ROOT_DIR=%~dp0.."
set "ARCH_BIN_DIR=%ROOT_DIR%\bin\%ZIP_ARCH%"
set "PACKAGE_DIR=%~dp0textparser-%ZIP_ARCH%"

if not exist "%ARCH_BIN_DIR%\textparser.exe" (
    echo [ERROR] Missing %ZIP_ARCH% build outputs. Run build.bat %ZIP_ARCH% first.
    exit /b 1
)

if exist "%PACKAGE_DIR%\" rmdir /s /q "%PACKAGE_DIR%"
mkdir "%PACKAGE_DIR%\include"

for %%F in (ccat.exe textparser.exe textparser.dll textparser-json.dll textparser.lib textparser-json.lib json-c.dll pcre2-8.dll pcre2-16.dll pcre2-32.dll pcre2-posix.dll) do (
    copy /y "%ARCH_BIN_DIR%\%%F" "%PACKAGE_DIR%" >nul || exit /b 1
)
copy /y "%ROOT_DIR%\include\textparser.h" "%PACKAGE_DIR%\include" >nul || exit /b 1
copy /y "%ROOT_DIR%\include\textparser-json.h" "%PACKAGE_DIR%\include" >nul || exit /b 1

for /f "delims=" %%i in ('git -C "%ROOT_DIR%" describe --tags --dirty') do set "git_describe=%%i"
tar -a -c -f "%~dp0textparser-%git_describe%-%ZIP_ARCH%.zip" -C "%PACKAGE_DIR%" ccat.exe textparser.exe textparser.dll textparser-json.dll textparser.lib textparser-json.lib json-c.dll pcre2-8.dll pcre2-16.dll pcre2-32.dll pcre2-posix.dll include\textparser.h include\textparser-json.h
if errorlevel 1 exit /b 1

rmdir /s /q "%PACKAGE_DIR%"
exit /b 0
