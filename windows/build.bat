@echo off

if exist build\ (
    rmdir /s /q build
)

SETX PATH "C:\Program Files\CMake\bin;C:\Program Files\LLVm\bin;%PATH%"

cmake.exe -S .. -B build -G "Ninja"
cmake --build build --config Release

copy build\compile_commands.json ..

rmdir /s /q build
