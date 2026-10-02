"""
Scenario Definitions and Walkthroughs for Phase 1 CDN Simulation.
Provides deterministic executions for verifying:
  1. Normal traffic variations: 3000 -> 3100 -> 2950 -> 3200
  2. Traffic spikes: 3200 -> 4500 -> 4900
  3. Congestion & Queuing Latency: 25ms -> 40ms -> 80ms -> 120ms
  4. Failure injection: Mumbai (HEALTHY -> DOWN) and recovery.
"""

from typing import List, Dict, Any
from .simulator import CDNSimulator


def run_phase1_demonstration(simulator: CDNSimulator) -> List[Dict[str, Any]]:
    """
    Executes a structured timeline demonstrating all 4 behaviors requested in Phase 1.
    Returns step-by-step state snapshots for reporting and logging.
    """
    timeline_records = []

    # Reset Pune to start of demo
    pune = simulator.get_server("Pune")
    pune.set_traffic(3000.0)

    # -------------------------------------------------------------
    # 1. Normal Traffic Sequence: 3000 -> 3100 -> 2950 -> 3200
    # -------------------------------------------------------------
    simulator.log_event("SCENARIO", ">>> Starting Scenario 1: Normal Traffic Fluctuation (Pune)", "INFO")
    normal_steps = [3000.0, 3100.0, 2950.0, 3200.0]
    for traffic in normal_steps:
        pune.set_traffic(traffic)
        snap = simulator.tick(stochastic_drift=False)
        snap["phase"] = "Normal Traffic"
        snap["note"] = f"Pune traffic set to {traffic} req/s"
        timeline_records.append(snap)

    # -------------------------------------------------------------
    # 2. Traffic Spike Sequence: 3200 -> 4500 -> 4900
    # -------------------------------------------------------------
    simulator.log_event("SCENARIO", ">>> Starting Scenario 2: Traffic Spike Surge (Pune)", "WARNING")
    spike_steps = [4500.0, 4900.0]
    for traffic in spike_steps:
        pune.set_traffic(traffic)
        snap = simulator.tick(stochastic_drift=False)
        snap["phase"] = "Traffic Spike"
        snap["note"] = f"Spike surge to {traffic} req/s"
        timeline_records.append(snap)

    # -------------------------------------------------------------
    # 3. Congestion Sequence: Latency 25ms -> 40ms -> 80ms -> 120ms
    # -------------------------------------------------------------
    simulator.log_event("SCENARIO", ">>> Starting Scenario 3: Progressive Queuing Congestion", "WARNING")
    congestion_traffic_levels = [
        (3200.0, "Nominal baseline (~64%) -> Latency 25.0ms (HEALTHY)"),
        (4100.0, "Queue buildup (~82%) -> Latency 40.0ms"),
        (4650.0, "Severe queuing (~93%) -> Latency 80.0ms (CONGESTED)"),
        (4900.0, "Critical saturation (~98%) -> Latency 120.0ms (CONGESTED)"),
    ]
    for traffic, desc in congestion_traffic_levels:
        pune.set_traffic(traffic)
        snap = simulator.tick(stochastic_drift=False)
        snap["phase"] = "Congestion & Latency Ramp"
        snap["note"] = f"Pune: {desc}"
        timeline_records.append(snap)

    # -------------------------------------------------------------
    # 4. Server Failure: Mumbai -> HEALTHY -> Mumbai -> DOWN
    # -------------------------------------------------------------
    simulator.log_event("SCENARIO", ">>> Starting Scenario 4: Server Outage (Mumbai)", "CRITICAL")
    mumbai = simulator.get_server("Mumbai")
    
    # Pre-failure state snapshot
    snap_healthy = simulator.tick(stochastic_drift=False)
    snap_healthy["phase"] = "Server Health Check"
    snap_healthy["note"] = f"Mumbai Status: {mumbai.status.value}"
    timeline_records.append(snap_healthy)

    # Fail Mumbai
    simulator.fail_server("Mumbai")
    snap_down = simulator.tick(stochastic_drift=False)
    snap_down["phase"] = "Server Failure"
    snap_down["note"] = f"Mumbai Status: {mumbai.status.value} (Traffic dropped to 0, Latency: INF)"
    timeline_records.append(snap_down)

    # Optional Recovery demonstration
    simulator.recover_server("Mumbai", initial_traffic=3500.0)
    snap_recovered = simulator.tick(stochastic_drift=False)
    snap_recovered["phase"] = "Server Recovery"
    snap_recovered["note"] = f"Mumbai Status: {mumbai.status.value} (Restored to service)"
    timeline_records.append(snap_recovered)

    return timeline_records
