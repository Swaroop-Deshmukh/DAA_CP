"""
Engine Client for C++17 DAA Min-Cost Max-Flow Engine (Phase 6).
Provides persistent IPC over stdin/stdout with automatic fallback to subprocess mode.

Single source of truth:
The C++ engine drives Poisson demand generation, diurnal 24h sine waves,
server queuing latency, and optimal MCMF routing decisions.
"""

import os
import sys
import json
import time
import subprocess
import threading
from typing import Dict, Any, Optional

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "../.."))
DAA_DIR = os.path.join(BASE_DIR, "DAA")
DAA_BIN = os.path.join(DAA_DIR, "bin", "cdn_engine.exe")
ROUTING_JSON = os.path.join(BASE_DIR, "data", "routing_decision.json")


class EngineClient:
    """
    Client connecting Python FastAPI Gateway to the C++ DAA Engine.
    Supports:
      1. Long-lived IPC Mode (default, ~0.1ms latency)
      2. Subprocess Spawning Fallback Mode (~20-30ms latency)
    """

    def __init__(self, use_subprocess_fallback: bool = False):
        self.use_fallback = use_subprocess_fallback
        self.process: Optional[subprocess.Popen] = None
        self.lock = threading.Lock()
        self.last_snapshot: Dict[str, Any] = {}
        self.last_e2e_ms: float = 0.0
        self.last_solve_us: float = 0.0

        if not self.use_fallback:
            self._start_ipc_daemon()

    def _start_ipc_daemon(self):
        """Starts the long-lived C++ cdn_engine daemon with --ipc flag."""
        if not os.path.exists(DAA_BIN):
            print(f"[EngineClient] Binary not found at {DAA_BIN}. Falling back to cached mode.")
            self.use_fallback = True
            return

        try:
            self.process = subprocess.Popen(
                [DAA_BIN, "--ipc"],
                cwd=DAA_DIR,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1  # line buffered
            )

            # Read initial ready confirmation
            ready_line = self.process.stdout.readline()
            if ready_line:
                ready_json = json.loads(ready_line.strip())
                print(f"[EngineClient] Long-lived IPC Engine started: {ready_json.get('engine')} ({ready_json.get('mode')})")
        except Exception as e:
            print(f"[EngineClient] Failed to start IPC daemon: {e}. Falling back to subprocess mode.")
            self.use_fallback = True
            self.process = None

    def close(self):
        """Gracefully shuts down the long-lived engine process."""
        with self.lock:
            if self.process:
                try:
                    self.process.stdin.write('{"cmd": "quit"}\n')
                    self.process.stdin.flush()
                    self.process.terminate()
                except Exception:
                    pass
                self.process = None

    def set_fallback_mode(self, fallback: bool):
        """Toggle between long-lived IPC mode and subprocess fallback."""
        with self.lock:
            if fallback == self.use_fallback:
                return
            self.use_fallback = fallback
            if fallback:
                if self.process:
                    try:
                        self.process.stdin.write('{"cmd": "quit"}\n')
                        self.process.stdin.flush()
                        self.process.terminate()
                    except Exception:
                        pass
                    self.process = None
                print("[EngineClient] Switched to Subprocess Fallback Mode")
            else:
                self._start_ipc_daemon()
                print("[EngineClient] Switched to Long-Lived IPC Mode")

    def _read_routing_json(self) -> Dict[str, Any]:
        """Reads latest canonical routing decision file."""
        if os.path.exists(ROUTING_JSON):
            try:
                with open(ROUTING_JSON, "r") as f:
                    return json.load(f)
            except Exception as e:
                print(f"[EngineClient] Error reading {ROUTING_JSON}: {e}")
        return {}

    def _execute_subprocess_fallback(self, scenario: str = "normal") -> Dict[str, Any]:
        """Executes cdn_engine per event via subprocess.run (Fallback path)."""
        t0 = time.perf_counter()
        cmd = [DAA_BIN, "--scenario", scenario]
        try:
            if os.path.exists(DAA_BIN):
                res = subprocess.run(
                    cmd,
                    cwd=DAA_DIR,
                    capture_output=True,
                    text=True,
                    timeout=10
                )
                if res.returncode != 0:
                    print(f"[EngineClient] Subprocess warning: {res.stderr}")
        except Exception as e:
            print(f"[EngineClient] Subprocess execution error: {e}")

        t1 = time.perf_counter()
        e2e_ms = (t1 - t0) * 1000.0

        decision = self._read_routing_json()
        if "routing" in decision and isinstance(decision["routing"], dict):
            inner = decision["routing"]
            solve_us = inner.get("engine_solve_time_us", inner.get("execution_time_ms", 0.0) * 1000.0)
        else:
            solve_us = decision.get("engine_solve_time_us", decision.get("execution_time_ms", 0.0) * 1000.0)

        snapshot = {
            "schema_version": 2,
            "status": "ok",
            "cmd": "fallback_subprocess",
            "ipc_mode": "subprocess_fallback",
            "engine_solve_time_us": solve_us,
            "execution_time_ms": solve_us / 1000.0,
            "e2e_latency_ms": e2e_ms,
            "routing": decision,
            "telemetry": decision.get("telemetry", {})
        }
        self.last_e2e_ms = e2e_ms
        self.last_solve_us = solve_us
        self.last_snapshot = snapshot
        return snapshot

    def send_command(self, cmd_dict: Dict[str, Any]) -> Dict[str, Any]:
        """
        Sends a command to the C++ engine and returns the response snapshot.
        Automatically uses subprocess fallback if IPC mode is disabled or fails.
        """
        if self.use_fallback or not self.process:
            scen = cmd_dict.get("scenario", cmd_dict.get("server", "normal"))
            return self._execute_subprocess_fallback(scen)

        with self.lock:
            t0 = time.perf_counter()
            cmd_json = json.dumps(cmd_dict) + "\n"

            try:
                self.process.stdin.write(cmd_json)
                self.process.stdin.flush()

                # Read JSON response line from stdout
                resp_line = self.process.stdout.readline()
                if not resp_line:
                    raise IOError("Engine process returned empty response (pipe closed)")

                t1 = time.perf_counter()
                e2e_ms = (t1 - t0) * 1000.0

                resp = json.loads(resp_line.strip())
                resp["e2e_latency_ms"] = e2e_ms
                resp["ipc_mode"] = "long-lived"

                self.last_e2e_ms = e2e_ms
                self.last_solve_us = resp.get("engine_solve_time_us", 0.0)
                self.last_snapshot = resp
                return resp

            except Exception as e:
                print(f"[EngineClient] IPC error ({e}). Restarting engine and falling back.")
                self.process = None
                self._start_ipc_daemon()
                # Run this request via subprocess fallback
                return self._execute_subprocess_fallback(cmd_dict.get("scenario", "normal"))

    # Convenience helper commands
    def tick(self, drift: bool = True) -> Dict[str, Any]:
        return self.send_command({"cmd": "tick", "drift": drift})

    def run_scenario(self, scenario_name: str) -> Dict[str, Any]:
        return self.send_command({"cmd": "run_scenario", "scenario": scenario_name})

    def trigger_spike(self, server_name: str, target_traffic: float = 4900.0) -> Dict[str, Any]:
        return self.send_command({"cmd": "spike", "server": server_name, "target_traffic": target_traffic})

    def fail_server(self, server_name: str) -> Dict[str, Any]:
        return self.send_command({"cmd": "fail", "server": server_name})

    def recover_server(self, server_name: str, initial_traffic: float = -1.0) -> Dict[str, Any]:
        return self.send_command({"cmd": "recover", "server": server_name, "initial_traffic": initial_traffic})

    def set_traffic(self, server_name: str, traffic: float) -> Dict[str, Any]:
        return self.send_command({"cmd": "set_traffic", "server": server_name, "traffic": traffic})

    def set_strategy(self, strategy: str) -> Dict[str, Any]:
        return self.send_command({"cmd": "set_strategy", "strategy": strategy})

    def run_trace(self, topology: str = "3x3") -> Dict[str, Any]:
        return self.send_command({"cmd": "run_trace", "topology": topology})

    def get_snapshot(self) -> Dict[str, Any]:
        if not self.last_snapshot:
            return self.send_command({"cmd": "snapshot"})
        return self.last_snapshot

    def benchmark_latency(self, n_samples: int = 10) -> Dict[str, Any]:
        """
        Runs n_samples in Long-Lived IPC mode and n_samples in Subprocess Fallback mode.
        Returns a comparative report showing End-to-End Latency vs Engine Solve Time.
        """
        ipc_e2e_times = []
        ipc_solve_times = []
        sub_e2e_times = []
        sub_solve_times = []

        # 1. Benchmark Long-Lived IPC
        prev_fallback = self.use_fallback
        self.set_fallback_mode(False)
        time.sleep(0.1)

        for _ in range(n_samples):
            resp = self.send_command({"cmd": "tick", "drift": True})
            ipc_e2e_times.append(resp.get("e2e_latency_ms", 0.0))
            ipc_solve_times.append(resp.get("engine_solve_time_us", 0.0))
            time.sleep(0.01)

        # 2. Benchmark Subprocess Fallback
        self.set_fallback_mode(True)
        for _ in range(n_samples):
            resp = self.send_command({"cmd": "run_scenario", "scenario": "normal"})
            sub_e2e_times.append(resp.get("e2e_latency_ms", 0.0))
            sub_solve_times.append(resp.get("engine_solve_time_us", 0.0))
            time.sleep(0.01)

        # Restore previous mode
        self.set_fallback_mode(prev_fallback)

        ipc_e2e_sorted = sorted(ipc_e2e_times)
        ipc_solve_sorted = sorted(ipc_solve_times)
        sub_e2e_sorted = sorted(sub_e2e_times)
        sub_solve_sorted = sorted(sub_solve_times)

        median_ipc_e2e = ipc_e2e_sorted[n_samples // 2]
        median_ipc_solve = ipc_solve_sorted[n_samples // 2]
        median_sub_e2e = sub_e2e_sorted[n_samples // 2]
        median_sub_solve = sub_solve_sorted[n_samples // 2]

        speedup = (median_sub_e2e / median_ipc_e2e) if median_ipc_e2e > 0 else 0.0

        return {
            "samples": n_samples,
            "ipc_median_e2e_ms": round(median_ipc_e2e, 3),
            "ipc_median_solve_us": round(median_ipc_solve, 1),
            "subprocess_median_e2e_ms": round(median_sub_e2e, 3),
            "subprocess_median_solve_us": round(median_sub_solve, 1),
            "e2e_speedup_factor": round(speedup, 1),
            "overhead_ratio_subprocess": round(median_sub_e2e / (median_sub_solve / 1000.0), 1) if median_sub_solve > 0 else 0.0,
            "overhead_ratio_ipc": round(median_ipc_e2e / (median_ipc_solve / 1000.0), 1) if median_ipc_solve > 0 else 0.0
        }
