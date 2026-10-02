"""
Unit and Integration Tests for Phase 1 CDN Simulator.
"""

import math
import pytest
from cdn_sim.server import EdgeServer, ServerStatus
from cdn_sim.simulator import CDNSimulator


def test_server_initialization():
    """Verify server initializes with proper defaults and physics."""
    pune = EdgeServer(
        name="Pune",
        capacity=5000.0,
        base_latency=25.0,
        bandwidth_cost=8.0,
        initial_requests=3200.0
    )
    assert pune.name == "Pune"
    assert pune.capacity == 5000.0
    assert pune.current_requests == 3200.0
    assert pune.latency == 25.0
    assert pune.cpu_utilization == 64.0
    assert pune.bandwidth_usage == 600.0  # 3200 * 0.1875
    assert pune.bandwidth_cost == 8.0
    assert pune.status == ServerStatus.HEALTHY


def test_normal_traffic_variation():
    """Verify normal traffic fluctuations: 3000 -> 3100 -> 2950 -> 3200."""
    pune = EdgeServer("Pune", 5000.0, 25.0, 8.0, 3000.0)
    normal_steps = [3000.0, 3100.0, 2950.0, 3200.0]

    for req in normal_steps:
        pune.set_traffic(req)
        assert pune.current_requests == req
        assert pune.status == ServerStatus.HEALTHY
        assert pune.latency == 25.0
        assert 58.0 <= pune.cpu_utilization <= 65.0
        assert pune.bandwidth_usage > 500.0


def test_traffic_spike():
    """Verify traffic spike: 3200 -> 4500 -> 4900."""
    pune = EdgeServer("Pune", 5000.0, 25.0, 8.0, 3200.0)
    
    # Spike to 4500
    pune.set_traffic(4500.0)
    assert pune.current_requests == 4500.0
    assert pune.cpu_utilization >= 85.0
    assert pune.bandwidth_usage > 800.0
    assert pune.status == ServerStatus.CONGESTED

    # Spike to 4900
    pune.set_traffic(4900.0)
    assert pune.current_requests == 4900.0
    assert pune.cpu_utilization >= 95.0
    assert pune.latency >= 115.0
    assert pune.status == ServerStatus.CONGESTED


def test_congestion_latency_progression():
    """Verify queuing latency steps: 25ms -> 40ms -> 80ms -> 120ms."""
    pune = EdgeServer("Pune", 5000.0, 25.0, 8.0, 3200.0)

    # 1. 25ms
    pune.set_traffic(3200.0)
    assert math.isclose(pune.latency, 25.0, abs_tol=1.0)
    assert pune.status == ServerStatus.HEALTHY

    # 2. 40ms
    pune.set_traffic(4100.0)
    assert math.isclose(pune.latency, 40.0, abs_tol=1.5)

    # 3. 80ms
    pune.set_traffic(4650.0)
    assert math.isclose(pune.latency, 80.0, abs_tol=2.0)
    assert pune.status == ServerStatus.CONGESTED

    # 4. 120ms
    pune.set_traffic(4900.0)
    assert math.isclose(pune.latency, 120.0, abs_tol=2.0)
    assert pune.status == ServerStatus.CONGESTED


def test_server_failure_and_recovery():
    """Verify failure: Mumbai -> HEALTHY -> Mumbai -> DOWN, and recovery."""
    sim = CDNSimulator()
    mumbai = sim.get_server("Mumbai")

    # Initial state
    assert mumbai.status == ServerStatus.HEALTHY
    assert mumbai.current_requests > 0
    assert not math.isinf(mumbai.latency)

    # Trigger failure
    sim.fail_server("Mumbai")
    assert mumbai.status == ServerStatus.DOWN
    assert mumbai.current_requests == 0.0
    assert math.isinf(mumbai.latency)
    assert mumbai.cpu_utilization == 0.0
    assert mumbai.bandwidth_usage == 0.0

    # Snapshot reflects failure
    snap = sim.get_snapshot()
    assert snap["down_count"] == 1
    assert snap["active_count"] == 3
    assert snap["fleet_health"] == "DEGRADED"

    # Trigger recovery
    sim.recover_server("Mumbai")
    assert mumbai.status == ServerStatus.HEALTHY
    assert mumbai.current_requests > 0
    assert not math.isinf(mumbai.latency)
