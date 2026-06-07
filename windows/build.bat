@echo off

if exist build\ (
    rmdir /s /q build
)

SET "PATH=C:\Program Files\CMake\bin;C:\Program Files\LLVm\bin;%PATH%"
SET "CC=C:\Program Files\LLVm\bin\clang.exe"
SET "CXX=C:\Program Files\LLVm\bin\clang++.exe"

cmake.exe -S .. -B build -G "Ninja" || exit /b 1
cmake --build build --config Release || exit /b 1

copy build\compile_commands.json ..

rmdir /s /q build
