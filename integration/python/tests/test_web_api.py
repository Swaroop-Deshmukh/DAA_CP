import os
import sys

# Ensure dashboard and python packages are discoverable
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../../dashboard")))
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from fastapi.testclient import TestClient
from web_app import app


def test_web_routes():
    client = TestClient(app)
    
    # 1. Test index page
    res_index = client.get("/")
    assert res_index.status_code == 200
    assert "CDN Edge Server Simulator" in res_index.text

    # 2. Test snapshot API
    res_snap = client.get("/api/snapshot")
    assert res_snap.status_code == 200
    data = res_snap.json()
    assert len(data["servers"]) == 4
    names = [s["name"] for s in data["servers"]]
    assert "Pune" in names
    assert "Mumbai" in names
    assert "Delhi" in names
    assert "Bangalore" in names

    # 3. Test Spike API
    res_spike = client.post("/api/event/spike", json={"server": "Pune", "target_traffic": 4500.0})
    assert res_spike.status_code == 200

    # 4. Test Fail API
    res_fail = client.post("/api/event/fail", json={"server": "Mumbai"})
    assert res_fail.status_code == 200
    
    res_snap2 = client.get("/api/snapshot")
    mumbai_snap = next(s for s in res_snap2.json()["servers"] if s["name"] == "Mumbai")
    assert mumbai_snap["status"] == "DOWN"
    assert mumbai_snap["current_requests"] == 0.0

    # 5. Test Recover API
    res_rec = client.post("/api/event/recover", json={"server": "Mumbai"})
    assert res_rec.status_code == 200
    res_snap3 = client.get("/api/snapshot")
    mumbai_snap2 = next(s for s in res_snap3.json()["servers"] if s["name"] == "Mumbai")
    assert mumbai_snap2["status"] == "HEALTHY"
