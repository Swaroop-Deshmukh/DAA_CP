"""
FastAPI Web Dashboard for CDN Simulator & C++ DAA Min-Cost Max-Flow Engine.
Provides REST endpoints and WebSocket stream for real-time edge telemetry and C++ routing decisions.
"""

import asyncio
import json
import os
import subprocess
import sys
from typing import Optional
from contextlib import asynccontextmanager
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
from pydantic import BaseModel

# Ensure integration/python is on sys.path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../python")))

from cdn_sim.simulator import CDNSimulator

# Paths to DAA C++ Engine and Canonical Data
BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "../.."))
DAA_DIR = os.path.join(BASE_DIR, "DAA")
DAA_BIN = os.path.join(DAA_DIR, "bin", "cdn_engine.exe")
ROUTING_JSON = os.path.join(BASE_DIR, "data", "routing_decision.json")

# Shared simulation instance & state
simulator = CDNSimulator()
auto_drift_enabled = True
connected_clients = set()
current_routing_decision = {}


def read_routing_decision() -> dict:
    """Read the latest canonical routing decision written by the C++ DAA Engine."""
    if os.path.exists(ROUTING_JSON):
        try:
            with open(ROUTING_JSON, "r") as f:
                return json.load(f)
        except Exception as e:
            print(f"Error reading {ROUTING_JSON}: {e}")
    return {}


def run_cpp_engine(scenario_arg: str = "normal") -> dict:
    """Execute the native C++17 DAA engine to compute optimal MCMF routing decisions."""
    global current_routing_decision
    cmd = [DAA_BIN, "--scenario", scenario_arg]

    try:
        if os.path.exists(DAA_BIN):
            result = subprocess.run(
                cmd,
                cwd=DAA_DIR,
                capture_output=True,
                text=True,
                timeout=10
            )
            if result.returncode != 0:
                print(f"C++ Engine Warning: {result.stderr}")
        else:
            print(f"Warning: {DAA_BIN} not found. Using cached decision.")
    except Exception as e:
        print(f"Error executing C++ engine ({cmd}): {e}")

    current_routing_decision = read_routing_decision()
    return current_routing_decision


def get_combined_snapshot() -> dict:
    """Combine server telemetry with the C++ DAA Min-Cost Max-Flow decision."""
    telemetry = simulator.get_snapshot()
    routing = current_routing_decision or read_routing_decision()

    # Synchronize server assigned traffic from C++ MCMF engine into telemetry
    if routing and "server_details" in routing:
        server_allocations = {s["server"]: s for s in routing["server_details"]}
        for s in telemetry["servers"]:
            name = s["name"]
            if name in server_allocations:
                alloc = server_allocations[name]
                s["assigned_traffic"] = alloc.get("assigned_traffic", 0)
                s["routed_traffic"] = alloc.get("assigned_traffic", 0)
                s["unit_cost"] = alloc.get("unit_cost_rs", s.get("bandwidth_cost", 0.0))
                s["subtotal_cost"] = alloc.get("subtotal_cost_rs", 0.0)
                if alloc.get("status") == "FAILED" or alloc.get("status") == "DOWN":
                    s["status"] = "DOWN"
                elif alloc.get("status") == "CONGESTED":
                    s["status"] = "CONGESTED"

    res = dict(telemetry)
    res["telemetry"] = telemetry
    res["routing"] = routing
    return res


@asynccontextmanager
async def lifespan(app: FastAPI):
    # Startup: execute baseline C++ DAA routing decision
    print(">> Initializing C++ DAA Min-Cost Max-Flow Engine...")
    run_cpp_engine("normal")
    task = asyncio.create_task(telemetry_background_loop())
    yield
    # Shutdown
    task.cancel()


app = FastAPI(title="Adaptive CDN Traffic Router — DAA Dashboard", version="2.0.0", lifespan=lifespan)

# Request Models
class ScenarioRequest(BaseModel):
    scenario: str

class TrafficRequest(BaseModel):
    server: str
    traffic: float

class SpikeRequest(BaseModel):
    server: str
    target_traffic: float

class ServerActionRequest(BaseModel):
    server: str


# Static files
STATIC_DIR = os.path.join(os.path.dirname(__file__), "static")
os.makedirs(STATIC_DIR, exist_ok=True)
app.mount("/static", StaticFiles(directory=STATIC_DIR), name="static")


@app.get("/")
def get_index():
    return FileResponse(os.path.join(STATIC_DIR, "index.html"))


@app.get("/api/routing_decision")
def get_routing():
    """Return the latest C++ DAA routing decision."""
    decision = current_routing_decision or read_routing_decision()
    return decision


@app.get("/api/snapshot")
def get_snapshot():
    """Return combined telemetry and C++ MCMF routing decision."""
    return get_combined_snapshot()


@app.post("/api/scenario/{name}")
async def post_scenario(name: str):
    """
    Trigger a specific test scenario in the C++ DAA engine,
    recalculate Min-Cost Max-Flow, and broadcast the new decision.
    """
    scenario_map = {
        "normal": "normal",
        "spike": "spike",
        "pune_congestion": "pune",
        "pune": "pune",
        "mumbai_failure": "mumbai_fail",
        "mumbai": "mumbai_fail",
        "multiple_congestion": "multi",
        "multi": "multi",
        "fleet_overload": "overload",
        "overload": "overload",
        "fleet_recovery": "recovery",
        "recovery": "recovery",
        "reset": "reset"
    }

    arg = scenario_map.get(name.lower(), "normal")
    decision = run_cpp_engine(arg)

    # Sync simulator with the scenario
    if arg == "normal" or arg == "reset":
        simulator.recover_server("Mumbai")
        simulator.set_traffic("Pune", 3200)
        simulator.set_traffic("Mumbai", 3800)
        simulator.set_traffic("Delhi", 3400)
        simulator.set_traffic("Bangalore", 3100)
    elif arg == "spike":
        simulator.set_traffic("Pune", 4900)
        simulator.set_traffic("Mumbai", 4200)
    elif arg == "pune":
        simulator.trigger_spike("Pune", 4900)
    elif arg == "mumbai_fail":
        simulator.fail_server("Mumbai")
    elif arg == "multi":
        simulator.trigger_spike("Pune", 4900)
        simulator.trigger_spike("Delhi", 5400)
    elif arg == "overload":
        for s in simulator.servers.values():
            simulator.set_traffic(s.name, s.capacity * 0.95)
    elif arg == "recovery":
        simulator.recover_server("Mumbai")
        simulator.set_traffic("Pune", 3200)

    # Broadcast updated snapshot immediately to all open clients
    await broadcast_snapshot()
    return {"status": "ok", "scenario": name, "routing": decision}


@app.post("/api/event/spike")
async def post_spike(req: SpikeRequest):
    simulator.trigger_spike(req.server, req.target_traffic)
    if req.server.lower() == "pune":
        run_cpp_engine("pune")
    else:
        run_cpp_engine("spike")
    await broadcast_snapshot()
    return {"status": "ok", "message": f"Spike triggered on {req.server}"}


@app.post("/api/event/fail")
async def post_fail(req: ServerActionRequest):
    simulator.fail_server(req.server)
    if req.server.lower() == "mumbai":
        run_cpp_engine("mumbai_fail")
    await broadcast_snapshot()
    return {"status": "ok", "message": f"{req.server} marked DOWN"}


@app.post("/api/event/recover")
async def post_recover(req: ServerActionRequest):
    simulator.recover_server(req.server)
    run_cpp_engine("recovery")
    await broadcast_snapshot()
    return {"status": "ok", "message": f"{req.server} recovered"}


@app.post("/api/event/traffic")
async def post_traffic(req: TrafficRequest):
    simulator.set_traffic(req.server, req.traffic)
    await broadcast_snapshot()
    return {"status": "ok", "message": f"Traffic set on {req.server}"}


@app.post("/api/drift/toggle")
def toggle_drift():
    global auto_drift_enabled
    auto_drift_enabled = not auto_drift_enabled
    return {"status": "ok", "auto_drift": auto_drift_enabled}


@app.post("/api/reset")
async def post_reset():
    global simulator
    simulator = CDNSimulator()
    run_cpp_engine("normal")
    await broadcast_snapshot()
    return {"status": "ok", "message": "Simulator reset to default"}


@app.websocket("/ws/telemetry")
async def websocket_telemetry(websocket: WebSocket):
    await websocket.accept()
    connected_clients.add(websocket)
    try:
        # Send initial snapshot immediately upon connection
        await websocket.send_json(get_combined_snapshot())
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        connected_clients.remove(websocket)
    except Exception:
        connected_clients.discard(websocket)


async def broadcast_snapshot():
    """Broadcast the latest combined telemetry & DAA routing snapshot to all clients."""
    if not connected_clients:
        return
    snap = get_combined_snapshot()
    to_remove = set()
    for ws in list(connected_clients):
        try:
            await ws.send_json(snap)
        except Exception:
            to_remove.add(ws)
    connected_clients.difference_update(to_remove)


async def telemetry_background_loop():
    """Background task advancing simulation and broadcasting telemetry to web clients."""
    while True:
        await asyncio.sleep(1.5)
        if auto_drift_enabled:
            simulator.tick(stochastic_drift=True)
            await broadcast_snapshot()
