"""
CDN Simulator Engine.
Orchestrates edge servers, drives simulation ticks, logs state transitions,
and handles event injection (spikes, congestion, failure/recovery).
"""

from typing import Dict, List, Optional, Any, Callable
from datetime import datetime
from .server import EdgeServer, ServerStatus
from .redis_syncer import RedisSyncer


class CDNSimulator:
    """
    Main CDN Simulation Engine managing Edge Servers across regions.
    Syncs real-time conditions to Redis Hashes every tick.
    """

    def __init__(
        self,
        enable_redis: bool = True,
        redis_host: str = "localhost",
        redis_port: int = 6379,
        redis_db: int = 0
    ):
        self.servers: Dict[str, EdgeServer] = {}
        self.tick_count: int = 0
        self.events_log: List[Dict[str, Any]] = []
        self._prev_statuses: Dict[str, ServerStatus] = {}

        # Redis state synchronizer (Phase 2)
        self.redis_syncer = RedisSyncer(
            host=redis_host,
            port=redis_port,
            db=redis_db,
            enabled=enable_redis
        )

        # Initialize the 4 edge servers as defined in specifications
        self._init_default_servers()

        # Perform initial sync to Redis
        if self.redis_syncer.is_connected:
            self.redis_syncer.sync_all(self.servers)
            self.log_event("REDIS", "Synced initial server states to Redis Hashes", "SUCCESS")

    def _init_default_servers(self):
        """Sets up default Indian edge locations with realistic parameters."""
        default_configs = [
            {
                "name": "Pune",
                "capacity": 5000.0,
                "base_latency": 25.0,
                "bandwidth_cost": 8.0,
                "initial_requests": 3200.0,
            },
            {
                "name": "Mumbai",
                "capacity": 6000.0,
                "base_latency": 18.0,
                "bandwidth_cost": 6.0,
                "initial_requests": 3800.0,
            },
            {
                "name": "Delhi",
                "capacity": 5500.0,
                "base_latency": 32.0,
                "bandwidth_cost": 9.0,
                "initial_requests": 3400.0,
            },
            {
                "name": "Bangalore",
                "capacity": 5000.0,
                "base_latency": 24.0,
                "bandwidth_cost": 7.5,
                "initial_requests": 3100.0,
            },
        ]

        for cfg in default_configs:
            server = EdgeServer(**cfg)
            self.servers[server.name] = server
            self._prev_statuses[server.name] = server.status

        self.log_event("SYSTEM", "Initialized 4 Edge Servers: Pune, Mumbai, Delhi, Bangalore")

    def get_server(self, name: str) -> EdgeServer:
        """Fetch server instance by city name."""
        server = self.servers.get(name)
        if not server:
            raise KeyError(f"Server '{name}' not found. Available: {list(self.servers.keys())}")
        return server

    def log_event(self, source: str, message: str, level: str = "INFO"):
        """Record an event in the simulation log."""
        entry = {
            "tick": self.tick_count,
            "timestamp": datetime.now().strftime("%H:%M:%S"),
            "source": source,
            "message": message,
            "level": level
        }
        self.events_log.append(entry)
        if len(self.events_log) > 100:
            self.events_log.pop(0)

    def trigger_spike(self, server_name: str, target_traffic: float):
        """Inject an abrupt traffic spike on a server."""
        server = self.get_server(server_name)
        server.trigger_spike(target_traffic)
        self.log_event(
            server_name,
            f"Traffic spike triggered: targeting {target_traffic:.0f} req/s",
            level="WARNING"
        )

    def fail_server(self, server_name: str):
        """Simulate unexpected server outage."""
        server = self.get_server(server_name)
        server.fail()
        self.log_event(
            server_name,
            f"SERVER OUTAGE: {server_name} went DOWN!",
            level="CRITICAL"
        )

    def recover_server(self, server_name: str, initial_traffic: Optional[float] = None):
        """Restore a failed server back online."""
        server = self.get_server(server_name)
        server.recover(initial_traffic=initial_traffic)
        self.log_event(
            server_name,
            f"Server recovered: {server_name} is back online (HEALTHY)",
            level="SUCCESS"
        )

    def set_traffic(self, server_name: str, requests: float, smooth: bool = False):
        """Set traffic on a specific server."""
        server = self.get_server(server_name)
        prev = server.current_requests
        server.set_traffic(requests, smooth_target=smooth)
        self.log_event(
            server_name,
            f"Traffic adjusted: {prev:.0f} -> {requests:.0f} req/s"
        )

    def tick(self, stochastic_drift: bool = True) -> Dict[str, Any]:
        """
        Advance the simulation clock by 1 tick.
        Updates all servers, records transitions, and produces a system snapshot.
        """
        self.tick_count += 1

        for name, server in self.servers.items():
            prev_status = self._prev_statuses[name]
            server.tick(stochastic_drift=stochastic_drift)
            curr_status = server.status

            # Detect state transition
            if curr_status != prev_status:
                lvl = "WARNING" if curr_status == ServerStatus.CONGESTED else (
                    "CRITICAL" if curr_status == ServerStatus.DOWN else "SUCCESS"
                )
                self.log_event(
                    name,
                    f"Status changed: {prev_status.value} -> {curr_status.value} "
                    f"(Traffic: {server.current_requests:.0f} req/s, Latency: {server.latency}ms)",
                    level=lvl
                )
                self._prev_statuses[name] = curr_status

        # Sync all updated server conditions to Redis Hashes (Phase 2)
        if self.redis_syncer.enabled:
            synced = self.redis_syncer.sync_all(self.servers)
            if synced and self.tick_count % 10 == 0:
                self.log_event("REDIS", f"State sync tick #{self.tick_count} written to Redis hashes", "INFO")

        return self.get_snapshot()

    def get_snapshot(self) -> Dict[str, Any]:
        """Returns comprehensive JSON-serializable state of entire CDN fleet."""
        server_snapshots = [s.to_dict() for s in self.servers.values()]
        active_servers = [s for s in self.servers.values() if not s.is_failed]

        total_capacity = sum(s.capacity for s in self.servers.values())
        total_traffic = sum(s.current_requests for s in self.servers.values())
        total_bandwidth = sum(s.bandwidth_usage for s in self.servers.values())
        
        avg_latency = (
            sum(s.latency for s in active_servers) / len(active_servers)
            if active_servers else None
        )
        
        avg_cpu = (
            sum(s.cpu_utilization for s in active_servers) / len(active_servers)
            if active_servers else 0.0
        )

        down_count = sum(1 for s in self.servers.values() if s.status == ServerStatus.DOWN)
        congested_count = sum(1 for s in self.servers.values() if s.status == ServerStatus.CONGESTED)

        if down_count > 0:
            fleet_health = "DEGRADED" if down_count < len(self.servers) else "CRITICAL"
        elif congested_count > 0:
            fleet_health = "CONGESTED"
        else:
            fleet_health = "OPTIMAL"

        return {
            "tick": self.tick_count,
            "timestamp": datetime.now().strftime("%H:%M:%S"),
            "fleet_health": fleet_health,
            "total_capacity": round(total_capacity, 1),
            "total_traffic": round(total_traffic, 1),
            "total_bandwidth": round(total_bandwidth, 1),
            "avg_latency": round(avg_latency, 1) if avg_latency is not None else "INF",
            "avg_cpu": round(avg_cpu, 1),
            "active_count": len(active_servers),
            "down_count": down_count,
            "congested_count": congested_count,
            "redis_connected": self.redis_syncer.is_connected,
            "servers": server_snapshots,
            "recent_events": self.events_log[-8:]
        }
