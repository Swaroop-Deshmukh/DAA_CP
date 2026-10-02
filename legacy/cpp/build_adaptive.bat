@echo off
echo ===================================================
echo   Compiling Phase 5 Adaptive Router (C++ DAA Code)
echo ===================================================
g++ -O2 -std=c++11 main_adaptive.cpp -o main_adaptive.exe
if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Build completed: main_adaptive.exe
    echo.
    echo Running Phase 5 Closed-Loop Adaptive Router:
    echo ---------------------------------------------------
    main_adaptive.exe
) else (
    echo [ERROR] Compilation failed! Check g++ compiler.
)
