@echo off
echo ===================================================
echo   Compiling Phase 4 MCMF Engine (C++ DAA Code)
echo ===================================================
g++ -O2 -std=c++11 main_mcmf.cpp -o main_mcmf.exe
if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Build completed: main_mcmf.exe
    echo.
    echo Running Phase 4 Min-Cost Max-Flow Engine:
    echo ---------------------------------------------------
    main_mcmf.exe
) else (
    echo [ERROR] Compilation failed! Check g++ compiler.
)
