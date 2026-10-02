"""
Edge Server Data Model and Dynamic Physics Simulator.
Represents an individual CDN edge node with realistic performance characteristics.
"""

from enum import Enum
import math
import random
from typing import Optional, List, Dict, Any


class ServerStatus(str, Enum):
    HEALTHY = "HEALTHY"
    CONGESTED = "CONGESTED"
    DOWN = "DOWN"


class EdgeServer:
    """
    Simulates a CDN Edge Server.
    
    Attributes:
        name: City / Identifier (e.g., "Pune", "Mumbai", "Delhi", "Bangalore")
        capacity: Maximum requests/second server can sustainably process (req/s)
        current_requests: Actual incoming request traffic (req/s)
        base_latency: Baseline network/processing latency without load (ms)
        latency: Effective dynamic latency under current queue load (ms)
        cpu_utilization: Percentage CPU usage (0 - 100%)
        bandwidth_usage: Network throughput used (Mbps)
        bandwidth_cost: Data transfer cost (₹/GB)
        status: Current health status (HEALTHY, CONGESTED, DOWN)
    """

    def __init__(
        self,
        name: str,
        capacity: float,
        base_latency: float,
        bandwidth_cost: float,
        initial_requests: Optional[float] = None,
        bandwidth_capacity: Optional[float] = None,
        mbits_per_request: float = 0.1875  # ~23.4 KB per request -> 3200 req/s ≈ 600 Mbps
    ):
        self.name = name
        self.capacity = float(capacity)
        self.base_latency = float(base_latency)
        self.bandwidth_cost = float(bandwidth_cost)
        self.mbits_per_request = mbits_per_request
        self.bandwidth_capacity = float(bandwidth_capacity) if bandwidth_capacity is not None else round(self.capacity * 0.20, 1)

        # Initial baseline load (default ~64% nominal load)
        if initial_requests is not None:
            self.current_requests = float(initial_requests)
        else:
            self.current_requests = round(self.capacity * 0.64, 1)

        self.target_requests = self.current_requests
        self.is_failed = False
        self.is_artificially_congested = False

        # Derived dynamic properties
        self.latency = self.base_latency
        self.cpu_utilization = 0.0
        self.bandwidth_usage = 0.0
        self.status = ServerStatus.HEALTHY

        # History tracking (last 30 ticks for telemetry)
        self.history: List[Dict[str, Any]] = []

        # Run initial calculation
        self._recalculate_metrics(noise=False)

    def set_traffic(self, requests: float, smooth_target: bool = False):
        """Directly set or set target for incoming traffic."""
        if smooth_target:
            self.target_requests = max(0.0, float(requests))
        else:
            self.current_requests = max(0.0, float(requests))
            self.target_requests = self.current_requests
            self._recalculate_metrics(noise=False)

    def trigger_spike(self, target_traffic: float):
        """Surge traffic to a high spike value."""
        self.target_requests = float(target_traffic)

    def fail(self):
        """Simulate catastrophic server failure (e.g. power/link down)."""
        self.is_failed = True
        self.status = ServerStatus.DOWN
        self.current_requests = 0.0
        self.target_requests = 0.0
        self.cpu_utilization = 0.0
        self.bandwidth_usage = 0.0
        self.latency = float("inf")

    def recover(self, initial_traffic: Optional[float] = None):
        """Recover a failed server back to operational state."""
        self.is_failed = False
        if initial_traffic is not None:
            self.current_requests = float(initial_traffic)
        else:
            self.current_requests = round(self.capacity * 0.60, 1)
        self.target_requests = self.current_requests
        self._recalculate_metrics(noise=False)

    def set_congestion_override(self, congested: bool):
        """Manually induce severe internal processing congestion/queuing."""
        self.is_artificially_congested = congested
        self._recalculate_metrics(noise=False)

    def tick(self, stochastic_drift: bool = True, drift_magnitude: float = 0.02):
        """
        Advance server state by 1 simulation tick.
        Simulates natural traffic drift or approaches target traffic smoothly.
        """
        if self.is_failed:
            self.current_requests = 0.0
            self.latency = float("inf")
            self.cpu_utilization = 0.0
            self.bandwidth_usage = 0.0
            self.status = ServerStatus.DOWN
            self._record_history()
            return

        # Smoothly interpolate towards target requests if different
        diff = self.target_requests - self.current_requests
        if abs(diff) > 1.0:
            # Step 50% of the remaining gap per tick for smooth surge progression
            step = diff * 0.55
            self.current_requests = round(self.current_requests + step, 1)
        else:
            self.current_requests = self.target_requests

        # Apply realistic stochastic micro-variation if in nominal steady-state
        if stochastic_drift and abs(diff) <= 5.0:
            jitter_pct = random.uniform(-drift_magnitude, drift_magnitude)
            self.current_requests = max(
                100.0,
                round(self.current_requests * (1.0 + jitter_pct), 1)
            )
            self.target_requests = self.current_requests

        self._recalculate_metrics(noise=stochastic_drift)
        self._record_history()

    def _recalculate_metrics(self, noise: bool = True):
        """
        Calculates CPU, bandwidth, dynamic queuing latency, and server status.
        Uses M/M/1 queuing approximation for realistic congestion curves.
        """
        if self.is_failed:
            self.latency = float("inf")
            self.cpu_utilization = 0.0
            self.bandwidth_usage = 0.0
            self.status = ServerStatus.DOWN
            return

        # Load factor rho = current_requests / capacity
        rho = self.current_requests / max(1.0, self.capacity)

        # 1. Bandwidth usage (Mbps)
        raw_bw = self.current_requests * self.mbits_per_request
        bw_noise = random.uniform(-5.0, 5.0) if noise else 0.0
        self.bandwidth_usage = max(0.0, round(raw_bw + bw_noise, 1))

        # 2. CPU Utilization (%)
        # Scales linearly with load up to 70%, then non-linear queuing overhead kicks in
        if rho <= 0.70:
            cpu_val = rho * 100.0
        elif rho <= 0.90:
            # Steeper ramp from 70% to ~88%
            cpu_val = 70.0 + ((rho - 0.70) / 0.20) * 18.0
        elif rho <= 1.0:
            # Saturated: 88% to ~98%
            cpu_val = 88.0 + ((rho - 0.90) / 0.10) * 10.0
        else:
            # Overloaded
            cpu_val = min(100.0, 98.0 + (rho - 1.0) * 10.0)

        if self.is_artificially_congested:
            cpu_val = max(cpu_val, 94.0)

        cpu_jitter = random.uniform(-1.0, 1.0) if noise else 0.0
        self.cpu_utilization = round(min(100.0, max(0.0, cpu_val + cpu_jitter)), 1)

        # 3. Dynamic Latency (ms)
        # Baseline latency + queuing delay curve calibrated to:
        # rho <= 0.64 (e.g. 3200/5000): base latency (25.0 ms)
        # rho ~ 0.82 (e.g. 4100/5000): ~1.6x base (40.0 ms)
        # rho ~ 0.93 (e.g. 4650/5000): ~3.2x base (80.0 ms)
        # rho >= 0.98 (e.g. 4900/5000): ~4.8x base (120.0 ms+)
        if rho <= 0.64:
            calculated_latency = self.base_latency
        elif rho <= 0.82:
            progress = (rho - 0.64) / (0.82 - 0.64)
            calculated_latency = self.base_latency + progress * (self.base_latency * 0.60)  # 25 -> 40ms
        elif rho <= 0.93:
            progress = (rho - 0.82) / (0.93 - 0.82)
            calculated_latency = (self.base_latency * 1.60) + progress * (self.base_latency * 1.60)  # 40 -> 80ms
        elif rho <= 0.98:
            progress = (rho - 0.93) / (0.98 - 0.93)
            calculated_latency = (self.base_latency * 3.20) + progress * (self.base_latency * 1.60)  # 80 -> 120ms
        else:
            # Overloaded
            excess = rho - 0.98
            calculated_latency = (self.base_latency * 4.80) + excess * (self.base_latency * 20.0)

        if self.is_artificially_congested:
            calculated_latency = max(calculated_latency, self.base_latency * 4.80)

        lat_jitter = random.uniform(-0.5, 0.5) if noise else 0.0
        self.latency = round(max(self.base_latency, calculated_latency + lat_jitter), 1)

        # 4. Status determination
        if self.cpu_utilization >= 85.0 or rho >= 0.88 or self.latency >= (self.base_latency * 2.5):
            self.status = ServerStatus.CONGESTED
        else:
            self.status = ServerStatus.HEALTHY

    def _record_history(self):
        """Append latest snapshot to rolling history window."""
        self.history.append({
            "traffic": self.current_requests,
            "latency": 999.0 if math.isinf(self.latency) else self.latency,
            "cpu": self.cpu_utilization,
            "bandwidth": self.bandwidth_usage,
            "status": self.status.value
        })
        if len(self.history) > 60:
            self.history.pop(0)

    def to_dict(self) -> Dict[str, Any]:
        """Convert server state to JSON-serializable dictionary."""
        return {
            "name": self.name,
            "capacity": round(self.capacity, 1),
            "current_requests": round(self.current_requests, 1),
            "latency": "INF" if math.isinf(self.latency) else round(self.latency, 1),
            "latency_ms": None if math.isinf(self.latency) else round(self.latency, 1),
            "cpu": round(self.cpu_utilization, 1),
            "cpu_utilization": round(self.cpu_utilization, 1),
            "bandwidth_usage": round(self.bandwidth_usage, 1),
            "bandwidth_capacity": round(self.bandwidth_capacity, 1),
            "bandwidth_cost": round(self.bandwidth_cost, 2),
            "status": self.status.value,
            "load_ratio": 0.0 if self.is_failed else round(self.current_requests / self.capacity, 3)
        }

    def __repr__(self) -> str:
        lat_str = "INF" if math.isinf(self.latency) else f"{self.latency}ms"
        return (
            f"EdgeServer({self.name} | Status: {self.status.value} | "
            f"Traffic: {self.current_requests}/{self.capacity} req/s | "
            f"Latency: {lat_str} | CPU: {self.cpu_utilization}% | "
            f"BW: {self.bandwidth_usage}Mbps | Cost: ₹{self.bandwidth_cost}/GB)"
        )
