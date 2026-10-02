@echo off
echo ====================================================================
echo   Compiling Adaptive CDN Traffic Router C++ Engine (DAA Project)
echo ====================================================================

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

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Compilation failed!
    exit /b %ERRORLEVEL%
)

echo [SUCCESS] Build completed successfully: cdn_engine.exe
echo.
echo Running CDN Engine:
echo ====================================================================
cdn_engine.exe %*
