@echo off
setlocal enabledelayedexpansion

echo ====================================================================
echo   Compiling DAA Min-Cost Max-Flow CDN Traffic Router (C++17)
echo ====================================================================

:: Navigate to the directory containing this batch script (DAA/)
cd /d "%~dp0"

:: Ensure output directories exist
if not exist "bin" mkdir bin
if not exist "build" mkdir build

:: Release file locks if cdn_engine.exe is running in background
taskkill /F /IM cdn_engine.exe >nul 2>&1

echo [1/2] Compiling C++ source files from src/ with headers from include/...


g++ -O2 -std=c++17 -Wall -Wextra -Iinclude ^
    src\GraphConfig.cpp ^
    src\CDNServer.cpp ^
    src\TrafficSimulator.cpp ^
    src\Graph.cpp ^
    src\MinCostMaxFlow.cpp ^
    src\EdmondsKarp.cpp ^
    src\Baselines.cpp ^
    src\CacheKnapsack.cpp ^
    src\HoltForecast.cpp ^
    src\StabilityManager.cpp ^
    src\SelfTest.cpp ^
    src\Router.cpp ^
    src\SimulationScenario.cpp ^
    src\AlgorithmTrace.cpp ^
    src\Benchmark.cpp ^
    src\IpcEngine.cpp ^
    src\main.cpp ^
    -o bin\cdn_engine.exe

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Compilation failed with exit code %ERRORLEVEL%!
    exit /b %ERRORLEVEL%
)

echo [2/2] Build successful: bin\cdn_engine.exe
echo ====================================================================
echo Running DAA Engine:
echo ====================================================================

bin\cdn_engine.exe %*
