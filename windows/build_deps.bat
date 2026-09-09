@echo off
setlocal EnableDelayedExpansion

REM usage: build_deps.bat [x64|arm64]  (no argument builds both arches)

set "ARCH=%~1"
if "%ARCH%"=="" set "ARCH=both"

set "ROOT_DIR=%~dp0.."
set "CLANG_DIR=C:\Program Files\LLVM\bin"
set "PATH=C:\Program Files\CMake\bin;%CLANG_DIR%;%PATH%"
set "JSON_C_VERSION=0.19-20260627"
set "PCRE2_VERSION=10.48"
set "JSON_C_SHA1=257a038fc4d3504d30db173eadb2603e084beb71"
set "PCRE2_SHA1=f1802e727c52f64d3b76349b81b86b4d5054249b"

where cmake.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake.exe not found in PATH.
    exit /b 1
)
where clang.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] clang.exe not found in PATH.
    exit /b 1
)

if not "%ARCH%"=="both" if /i not "%ARCH%"=="x64" if /i not "%ARCH%"=="arm64" (
    echo [ERROR] Invalid arch "%ARCH%". Use x64 or arm64.
    exit /b 1
)

REM Download and extract each source archive once; both architectures share it.
set "JSON_SOURCE=%~dp0json-c-json-c-%JSON_C_VERSION%"
set "PCRE_SOURCE=%~dp0pcre2-%PCRE2_VERSION%"
if exist "%JSON_SOURCE%\" rmdir /s /q "%JSON_SOURCE%"
if exist "%PCRE_SOURCE%\" rmdir /s /q "%PCRE_SOURCE%"

curl -L --output "%~dp0json-c.zip" https://github.com/json-c/json-c/archive/refs/tags/json-c-%JSON_C_VERSION%.zip || exit /b 1
call :verify_sha1 "%~dp0json-c.zip" "%JSON_C_SHA1%"
if errorlevel 1 exit /b 1
tar --exclude=*/tests/*.test -xf "%~dp0json-c.zip" -C "%~dp0." || exit /b 1
del "%~dp0json-c.zip"

curl -L --output "%~dp0pcre2.zip" https://github.com/PCRE2Project/pcre2/releases/download/pcre2-%PCRE2_VERSION%/pcre2-%PCRE2_VERSION%.zip || exit /b 1
call :verify_sha1 "%~dp0pcre2.zip" "%PCRE2_SHA1%"
if errorlevel 1 exit /b 1
tar -xf "%~dp0pcre2.zip" -C "%~dp0." || exit /b 1
del "%~dp0pcre2.zip"

if /i "%ARCH%"=="both" (
    for %%A in (x64 arm64) do (
        call :build_arch %%A
        if errorlevel 1 (
            echo [ERROR] deps build failed for %%A
            exit /b 1
        )
    )
) else (
    call :build_arch %ARCH%
    if errorlevel 1 (
        echo [ERROR] deps build failed for %ARCH%
        exit /b 1
    )
)

if exist "%JSON_SOURCE%\" rmdir /s /q "%JSON_SOURCE%"
if exist "%PCRE_SOURCE%\" rmdir /s /q "%PCRE_SOURCE%"

echo [OK] deps (%ARCH%) built.
exit /b 0

:build_arch
set "BUILD_ARCH=%~1"
set "ARCH_BIN_DIR=%ROOT_DIR%\bin\%BUILD_ARCH%"
set "JSON_BUILD=%~dp0deps-build-%BUILD_ARCH%-json"
set "PCRE_BUILD=%~dp0deps-build-%BUILD_ARCH%-pcre2"

if exist "%JSON_BUILD%\" rmdir /s /q "%JSON_BUILD%"
if exist "%PCRE_BUILD%\" rmdir /s /q "%PCRE_BUILD%"
if not exist "%ARCH_BIN_DIR%\" mkdir "%ARCH_BIN_DIR%"

if /i "%BUILD_ARCH%"=="x64" (
    set "EXTRA_ARGS="
) else (
    set "EXTRA_ARGS=-DCMAKE_C_COMPILER_TARGET=aarch64-pc-windows-msvc -DCMAKE_SYSTEM_PROCESSOR=ARM64"
)

cmake.exe -S "%JSON_SOURCE%" -B "%JSON_BUILD%" -G Ninja -DCMAKE_C_COMPILER="%CLANG_DIR%\clang.exe" -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=OFF -DBUILD_TESTING=OFF -DBUILD_APPS=OFF -DDISABLE_WERROR=ON -DSIZEOF_SSIZE_T=8 %EXTRA_ARGS% || (
    echo [ERROR] Failed to configure json-c for %BUILD_ARCH%.
    exit /b 1
)
cmake.exe --build "%JSON_BUILD%" --target json-c || (
    echo [ERROR] Failed to build json-c for %BUILD_ARCH%.
    exit /b 1
)

if not exist "%ROOT_DIR%\include\json-c\" mkdir "%ROOT_DIR%\include\json-c"
xcopy /y "%JSON_SOURCE%\*.h" "%ROOT_DIR%\include\json-c" >nul || exit /b 1
xcopy /y "%JSON_BUILD%\*.h" "%ROOT_DIR%\include\json-c" >nul || exit /b 1
copy /y "%JSON_BUILD%\json-c.dll" "%ARCH_BIN_DIR%" >nul || exit /b 1
copy /y "%JSON_BUILD%\json-c.lib" "%ARCH_BIN_DIR%" >nul || exit /b 1

if exist "%JSON_BUILD%\" rmdir /s /q "%JSON_BUILD%"

cmake.exe -S "%PCRE_SOURCE%" -B "%PCRE_BUILD%" -G Ninja -DCMAKE_C_COMPILER="%CLANG_DIR%\clang.exe" -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=OFF -DBUILD_SHARED_LIBS=ON -DPCRE2_BUILD_PCRE2_8=ON -DPCRE2_BUILD_PCRE2_16=ON -DPCRE2_BUILD_PCRE2_32=ON -DPCRE2_BUILD_PCRE2GREP=OFF -DPCRE2_BUILD_TESTS=OFF %EXTRA_ARGS% || (
    echo [ERROR] Failed to configure PCRE2 for %BUILD_ARCH%.
    exit /b 1
)
cmake.exe --build "%PCRE_BUILD%" --target pcre2-8-shared pcre2-16-shared pcre2-32-shared pcre2-posix-shared || (
    echo [ERROR] Failed to build PCRE2 for %BUILD_ARCH%.
    exit /b 1
)

if not exist "%ROOT_DIR%\include\" mkdir "%ROOT_DIR%\include"
xcopy /y "%PCRE_SOURCE%\src\*.h" "%ROOT_DIR%\include" >nul || exit /b 1
for %%F in (pcre2-8 pcre2-16 pcre2-32 pcre2-posix) do (
    copy /y "%PCRE_BUILD%\%%F.dll" "%ARCH_BIN_DIR%" >nul || exit /b 1
    copy /y "%PCRE_BUILD%\%%F.lib" "%ARCH_BIN_DIR%" >nul || exit /b 1
)

if exist "%PCRE_BUILD%\" rmdir /s /q "%PCRE_BUILD%"
exit /b 0

:verify_sha1
set "LOCAL_FILE=%~1"
set "EXPECTED_HASH=%~2"
set "HASH_VALUE="
for /f "skip=1 delims=" %%L in ('certutil -hashfile "%LOCAL_FILE%" SHA1 2^>nul') do (
    if not defined HASH_VALUE set "HASH_VALUE=%%L"
)
if not defined HASH_VALUE (
    echo [ERROR] Could not compute SHA1 of %LOCAL_FILE%.
    exit /b 1
)
set "HASH_VALUE=%HASH_VALUE: =%"
if /i not "%HASH_VALUE%"=="%EXPECTED_HASH%" (
    echo [ERROR] SHA1 mismatch for %LOCAL_FILE%.
    echo         Expected: %EXPECTED_HASH%
    echo         Got:      %HASH_VALUE%
    exit /b 1
)
echo [OK] SHA1 checksum verified for %LOCAL_FILE%.
exit /b 0
