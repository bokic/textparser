@echo off

SET JSON_C_VERSION=0.18-20240915
SET PCRE2_VERSION=10.47

rmdir /s /q json-c
rmdir /s /q pcre2
rmdir /s /q build

curl -L --output json-c.zip https://github.com/json-c/json-c/archive/refs/tags/json-c-%JSON_C_VERSION%.zip
tar -xf json-c.zip
del json-c.zip

cmake.exe -S json-c-json-c-%JSON_C_VERSION% -B build -DBUILD_STATIC_LIBS=OFF
cmake.exe --build build --config Release

mkdir ..\include\json-c
mkdir ..\bin

xcopy /y json-c\*.h ..\include\json-c
xcopy /y build\*.h ..\include\json-c

xcopy /y /e build\Release\json-c.dll ..\bin
xcopy /y /e build\Release\json-c.lib ..\bin

rmdir /s /q json-c-json-c-%JSON_C_VERSION%
rmdir /s /q build

curl -L --output pcre2.zip https://github.com/PCRE2Project/pcre2/releases/download/pcre2-%PCRE2_VERSION%/pcre2-%PCRE2_VERSION%.zip
tar -xf pcre2.zip
del pcre2.zip

cmake.exe -S pcre2-%PCRE2_VERSION% -B build -DBUILD_STATIC_LIBS=OFF -DBUILD_SHARED_LIBS=ON -DPCRE2_BUILD_PCRE2_8=ON -DPCRE2_BUILD_PCRE2_16=ON -DPCRE2_BUILD_PCRE2_32=ON
cmake.exe --build build --config Release

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
