@echo off

if exist build\ (
    rmdir /s /q build
)

SET "PATH=C:\Program Files\CMake\bin;C:\Program Files\LLVm\bin;%PATH%"
SET "CC=C:\Program Files\LLVm\bin\clang.exe"
SET "CXX=C:\Program Files\LLVm\bin\clang++.exe"

cmake.exe -S .. -B build -G "Ninja"
cmake --build build --config Release

copy build\compile_commands.json ..

rmdir /s /q build
