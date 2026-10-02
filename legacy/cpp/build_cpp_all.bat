@echo off
echo ========================================================
echo Compiling C++ DAA Adaptive CDN Traffic Router Engine...
echo ========================================================

cd /d "%~dp0"
g++ -O2 -std=c++17 -Wall -Wextra ^
    cpp\CDNServer.cpp ^
    cpp\TrafficSimulator.cpp ^
    cpp\Graph.cpp ^
    cpp\MinCostMaxFlow.cpp ^
    cpp\Router.cpp ^
    cpp\SimulationScenario.cpp ^
    cpp\Benchmark.cpp ^
    cpp\main.cpp ^
    -o cdn_engine.exe

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] cdn_engine.exe built successfully!
    echo Running cdn_engine.exe...
    echo ========================================================
    cdn_engine.exe
) else (
    echo [ERROR] Compilation failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
