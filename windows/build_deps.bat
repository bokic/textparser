@echo off

SET JSON_C_VERSION=0.19-20260627
SET PCRE2_VERSION=10.48

REM "Visual Studio 18 2026" generator requires CMake >= 4.2
where cmake.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake.exe not found in PATH.
    exit /b 1
)

for /f "tokens=3" %%v in ('cmake.exe --version') do (
    set CMAKE_VERSION=%%v
    goto cmake_version_parsed
)
:cmake_version_parsed
for /f "tokens=1,2 delims=." %%a in ("%CMAKE_VERSION%") do (
    set CMAKE_VERSION_MAJOR=%%a
    set CMAKE_VERSION_MINOR=%%b
)

if %CMAKE_VERSION_MAJOR% LSS 4 goto cmake_too_old
if %CMAKE_VERSION_MAJOR% EQU 4 if %CMAKE_VERSION_MINOR% LSS 2 goto cmake_too_old
echo [OK] Using CMake %CMAKE_VERSION%
goto cmake_done

:cmake_too_old
echo [ERROR] CMake %CMAKE_VERSION% detected. The "Visual Studio 18 2026" generator requires CMake 4.2 or newer.
echo         Upgrade with: winget install Kitware.CMake
exit /b 1

:cmake_done

rmdir /s /q json-c
rmdir /s /q pcre2
rmdir /s /q build

curl -L --output json-c.zip https://github.com/json-c/json-c/archive/refs/tags/json-c-%JSON_C_VERSION%.zip
tar -xf json-c.zip
del json-c.zip

cmake.exe -S json-c-json-c-%JSON_C_VERSION% -B build -G "Visual Studio 18 2026" -A x64 -DBUILD_STATIC_LIBS=OFF -DBUILD_TESTING=OFF -DBUILD_APPS=OFF
cmake.exe --build build --config Release --target json-c

mkdir ..\include\json-c
mkdir ..\bin

xcopy /y json-c-json-c-%JSON_C_VERSION%\*.h ..\include\json-c
xcopy /y build\*.h ..\include\json-c

xcopy /y /e build\Release\json-c.dll ..\bin
xcopy /y /e build\Release\json-c.lib ..\bin

rmdir /s /q json-c-json-c-%JSON_C_VERSION%
rmdir /s /q build

curl -L --output pcre2.zip https://github.com/PCRE2Project/pcre2/releases/download/pcre2-%PCRE2_VERSION%/pcre2-%PCRE2_VERSION%.zip
tar -xf pcre2.zip
del pcre2.zip

cmake.exe -S pcre2-%PCRE2_VERSION% -B build -G "Visual Studio 18 2026" -A x64 -DBUILD_STATIC_LIBS=OFF -DBUILD_SHARED_LIBS=ON -DPCRE2_BUILD_PCRE2_8=ON -DPCRE2_BUILD_PCRE2_16=ON -DPCRE2_BUILD_PCRE2_32=ON -DPCRE2_BUILD_PCRE2GREP=OFF -DPCRE2_BUILD_TESTS=OFF
cmake.exe --build build --config Release --target pcre2-8-shared pcre2-16-shared pcre2-32-shared pcre2-posix-shared

mkdir ..\bin

xcopy /y /e build\interface ..\include
xcopy /y /e build\Release\pcre2-8.dll ..\bin
xcopy /y /e build\Release\pcre2-8.lib ..\bin
xcopy /y /e build\Release\pcre2-16.dll ..\bin
xcopy /y /e build\Release\pcre2-16.lib ..\bin
xcopy /y /e build\Release\pcre2-32.dll ..\bin
xcopy /y /e build\Release\pcre2-32.lib ..\bin
xcopy /y /e build\Release\pcre2-posix.dll ..\bin
xcopy /y /e build\Release\pcre2-posix.lib ..\bin

rmdir /s /q pcre2-%PCRE2_VERSION%
rmdir /s /q build
