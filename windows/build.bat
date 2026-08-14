@echo off

if exist build\ (
    rmdir /s /q build
)

SET "PATH=C:\Program Files\CMake\bin;C:\Program Files\LLVM\bin;%LOCALAPPDATA%\Microsoft\WinGet\Links;%LOCALAPPDATA%\Programs\Python\Python313;%PATH%"

cmake.exe -S .. -B build -G "Ninja" "-DCMAKE_C_COMPILER=C:/Program Files/LLVM/bin/clang.exe" "-DCMAKE_CXX_COMPILER=C:/Program Files/LLVM/bin/clang++.exe" -DBUILD_TESTS=OFF || exit /b 1
cmake --build build --config Release || exit /b 1

copy build\compile_commands.json ..

rmdir /s /q build
