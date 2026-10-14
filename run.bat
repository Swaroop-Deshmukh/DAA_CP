@echo off
setlocal

echo ====================================================================
echo   CDN Traffic Operations Console (NetOps) — Launch Script
echo ====================================================================

:: Navigate to root directory
cd /d "%~dp0"

:: 1. Ensure C++ Engine is built
if not exist "DAA\bin\cdn_engine.exe" (
    echo [1/3] Building C++ optimization core...
    cd DAA
    call build.bat
    cd ..
) else (
    echo [1/3] C++ engine binary verified: DAA\bin\cdn_engine.exe
)

:: 2. Ensure benchmark results exist
if not exist "results\scalability_benchmark.csv" (
    echo [2/3] Generating benchmark dataset...
    DAA\bin\cdn_engine.exe --bench
) else (
    echo [2/3] Scalability dataset verified: results\scalability_benchmark.csv
)

:: 3. Launch FastAPI Operations Dashboard
echo [3/3] Starting NetOps Console at http://127.0.0.1:8000/ ...
start "" http://127.0.0.1:8000/
python -m uvicorn integration.dashboard.web_app:app --host 127.0.0.1 --port 8000 --reload
