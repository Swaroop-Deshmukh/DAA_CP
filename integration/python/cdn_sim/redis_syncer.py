"""
Redis Real-Time State Syncer for CDN Simulator (Phase 2).
Handles persisting edge server conditions to Redis Hashes with robust connection error handling.
"""

import logging
from datetime import datetime, timezone
from typing import Dict, Any, Optional
import redis
from redis.exceptions import ConnectionError, TimeoutError, RedisError

logger = logging.getLogger("cdn_sim.redis")


class RedisSyncer:
    """
    Manages synchronization of CDN Edge Server states to Redis.
    Uses Redis Hashes:
        server:pune
        server:mumbai
        server:delhi
        server:bangalore
    """

    def __init__(
        self,
        host: str = "localhost",
        port: int = 6379,
        db: int = 0,
        password: Optional[str] = None,
        enabled: bool = True,
        connect_timeout: float = 0.1
    ):
        self.host = host
        self.port = port
        self.db = db
        self.enabled = enabled
        self.is_connected = False
        self._last_error_log = 0.0

        if not self.enabled:
            self.client = None
            return

        try:
            self.client = redis.Redis(
                host=self.host,
                port=self.port,
                db=self.db,
                password=password,
                decode_responses=True,
                socket_connect_timeout=connect_timeout,
                socket_timeout=connect_timeout
            )
            # Verify connectivity with Ping
            self.client.ping()
            self.is_connected = True
            logger.info("Successfully connected to Redis at %s:%d (db %d)", self.host, self.port, self.db)
        except (ConnectionError, TimeoutError, RedisError) as e:
            self.is_connected = False
            logger.warning("Redis not reachable at %s:%d (%s). Simulation will run in standalone mode.", self.host, self.port, e)

    def check_connection(self) -> bool:
        """Attempts a quick ping if disconnected, returns current status."""
        if not self.enabled or self.client is None:
            return False
        try:
            self.client.ping()
            self.is_connected = True
            return True
        except (ConnectionError, TimeoutError, RedisError):
            self.is_connected = False
            return False

    def sync_server(self, server) -> bool:
        """
        Writes the latest state of a single EdgeServer to its Redis Hash.
        Key: server:<name_lower> (e.g. server:pune)
        """
        if not self.enabled or not self.client:
            return False

        key = f"server:{server.name.lower()}"
        now_ts = datetime.now(timezone.utc).isoformat()
        
        # Prepare hash mapping
        mapping = {
            "capacity": f"{server.capacity:.1f}",
            "current_requests": f"{server.current_requests:.1f}",
            "latency": "INF" if server.is_failed else f"{server.latency:.1f}",
            "cpu": f"{server.cpu_utilization:.1f}",
            "bandwidth_usage": f"{server.bandwidth_usage:.1f}",
            "bandwidth_capacity": f"{server.bandwidth_capacity:.1f}",
            "bandwidth_cost": f"{server.bandwidth_cost:.2f}",
            "status": server.status.value,
            "timestamp": now_ts
        }

        try:
            self.client.hset(key, mapping=mapping)
            self.is_connected = True
            return True
        except (ConnectionError, TimeoutError, RedisError) as e:
            self.is_connected = False
            logger.debug("Failed to write to Redis for %s: %s", key, e)
            return False

    def sync_all(self, servers: Dict[str, Any]) -> bool:
        """
        Syncs all edge servers in a single pipelined operation for optimal performance.
        """
        if not self.enabled or not self.client:
            return False

        now_ts = datetime.now(timezone.utc).isoformat()

        try:
            pipeline = self.client.pipeline(transaction=False)
            for server in servers.values():
                key = f"server:{server.name.lower()}"
                mapping = {
                    "capacity": f"{server.capacity:.1f}",
                    "current_requests": f"{server.current_requests:.1f}",
                    "latency": "INF" if server.is_failed else f"{server.latency:.1f}",
                    "cpu": f"{server.cpu_utilization:.1f}",
                    "bandwidth_usage": f"{server.bandwidth_usage:.1f}",
                    "bandwidth_capacity": f"{server.bandwidth_capacity:.1f}",
                    "bandwidth_cost": f"{server.bandwidth_cost:.2f}",
                    "status": server.status.value,
                    "timestamp": now_ts
                }
                pipeline.hset(key, mapping=mapping)
            pipeline.execute()
            self.is_connected = True
            return True
        except (ConnectionError, TimeoutError, RedisError) as e:
            self.is_connected = False
            return False

    def get_server_state(self, server_name: str) -> Optional[Dict[str, str]]:
        """Reads back the hash for a given server from Redis."""
        if not self.enabled or not self.client or not self.is_connected:
            return None
        try:
            key = f"server:{server_name.lower()}"
            return self.client.hgetall(key)
        except (ConnectionError, TimeoutError, RedisError):
            self.is_connected = False
            return None

    def get_all_servers(self) -> Dict[str, Dict[str, str]]:
        """Reads all server hashes from Redis."""
        if not self.enabled or not self.client or not self.is_connected:
            return {}
        try:
            result = {}
            for name in ["pune", "mumbai", "delhi", "bangalore"]:
                key = f"server:{name}"
                data = self.client.hgetall(key)
                if data:
                    result[name] = data
            return result
        except (ConnectionError, TimeoutError, RedisError):
            self.is_connected = False
            return {}
