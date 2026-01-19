@echo off

rmdir /s /q build

cmake.exe -S .. -B build -T "ClangCL"
cmake --build build --config Release

rmdir /s /q build
