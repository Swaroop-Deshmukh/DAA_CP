# CDN Traffic Router & DAA Engine Data Contract (Schema v2)

This document formalizes the canonical data contract between the native **C++17 DAA Optimization Core** and all external consumers, including the **FastAPI Web Gateway**, WebSocket telemetry streams, and frontend visualization dashboards.

---

## 1. Architectural Principles & Guarantees

1. **Explicit Schema Versioning**:
   Every exported JSON payload and IPC response includes `"schema_version": 2` at the root.
2. **Atomic Write Guarantee**:
   The engine guarantees that readers never observe partially written or corrupt JSON files.
   - Implementation: Payloads are written to a temporary staging file (`data/routing_decision.json.tmp`), flushed to disk, and atomically swapped into place via `MoveFileExA(..., MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` on Windows and `rename(2)` on POSIX systems.
3. **Single Source of Truth**:
   The native C++ simulation engine acts as the sole source of truth for:
   - Request generation (Poisson arrivals + 24-hour diurnal sine cycles + stochastic spikes)
   - Dynamic queuing latency ($M/M/1$ piecewise convex curves)
   - Hardware health states (`HEALTHY`, `CONGESTED`, `DOWN`)
   - Min-Cost Max-Flow (MCMF) routing optimization
4. **Dual Latency Reporting**:
   Native algorithm solve time (`engine_solve_time_us`) is strictly isolated and measured separately from transport/process end-to-end latency (`e2e_latency_ms`).

---

## 2. Canonical File Contract: `data/routing_decision.json`

### Root Structure
```json
{
  "schema_version": 2,
  "status": "ok",
  "cmd": "tick",
  "cycle_id": 42,
  "active_strategy": "MCMF",
  "engine_solve_time_us": 70.9,
  "execution_time_ms": 0.071,
  "e2e_engine_latency_ms": 0.085,
  "telemetry": { ... },
  "routing": { ... }
}
```

### Root Fields

| Field | Type | Description |
|---|---|---|
| `schema_version` | `integer` | Contract version indicator (currently `2`). |
| `status` | `string` | Execution status: `"ok"` or `"error"`. |
| `cmd` | `string` | Triggering command (e.g., `"tick"`, `"spike"`, `"fail"`, `"run_scenario"`). |
| `cycle_id` | `integer` | Monotonically increasing engine execution cycle counter. |
| `active_strategy` | `string` | Active dispatch strategy: `"MCMF"`, `"ROUND_ROBIN"`, `"NEAREST"`, or `"GREEDY"`. |
| `engine_solve_time_us` | `number` | Pure C++ algorithm solve time in microseconds ($\mu$s). |
| `execution_time_ms` | `number` | Pure C++ algorithm solve time in milliseconds ($= \text{engine\_solve\_time\_us} / 1000$). |
| `e2e_engine_latency_ms` | `number` | Internal C++ command processing time including serialization. |
| `telemetry` | `object` | Fleet-wide telemetry and server health snapshot. |
| `routing` | `object` | Optimal flow allocation decision computed by the DAA solver. |

---

### `telemetry` Object Schema

```json
{
  "tick": 42,
  "timestamp": "14:32:42",
  "fleet_health": "OPTIMAL",
  "total_capacity": 21500.0,
  "total_traffic": 13500.0,
  "total_bandwidth": 2531.2,
  "avg_latency": 24.8,
  "avg_cpu": 62.8,
  "active_count": 4,
  "down_count": 0,
  "congested_count": 0,
  "redis_connected": false,
  "diurnal_multiplier": 0.859,
  "servers": [ ... ],
  "recent_events": [ ... ]
}
```

| Field | Type | Description |
|---|---|---|
| `tick` | `integer` | Discrete simulation clock tick. |
| `timestamp` | `string` | Local wall-clock time (`HH:MM:SS`). |
| `fleet_health` | `string` | Operational state: `"OPTIMAL"`, `"CONGESTED"`, `"DEGRADED"`, or `"CRITICAL"`. |
| `total_capacity` | `number` | Sum of capacities of non-failed edge servers (req/s). |
| `total_traffic` | `number` | Current aggregate incoming request rate across the fleet (req/s). |
| `total_bandwidth` | `number` | Total egress throughput in Megabits per second (Mbps). |
| `avg_latency` | `number` | Traffic-weighted average queuing response latency (ms). |
| `avg_cpu` | `number` | Average CPU compute saturation percentage (0.0% to 100.0%). |
| `active_count` | `integer` | Number of edge servers currently online and accepting traffic. |
| `down_count` | `integer` | Number of failed/offline servers. |
| `congested_count` | `integer` | Number of servers operating above 95% capacity or >80ms latency. |
| `diurnal_multiplier` | `number` | 24-hour sinusoidal wave factor ($0.80 \le M(t) \le 1.20$). |
| `servers` | `array[object]` | Array of individual Edge Server condition records. |
| `recent_events` | `array[object]` | Rolling buffer of recent operational state transitions. |

#### `servers[]` Item Schema
| Field | Type | Description |
|---|---|---|
| `id` | `integer` | Numerical identifier of the edge node. |
| `name` | `string` | Geographical city / node name (e.g., `"Pune"`, `"Mumbai"`). |
| `location` | `string` | Regional geographic designation. |
| `capacity` | `number` | Sustainable request throughput ceiling (req/s). |
| `current_requests` | `number` | Unrouted incoming client request volume arriving at this node (req/s). |
| `assigned_traffic` | `number` | Optimal traffic routed to this server by the MCMF engine (req/s). |
| `available_capacity`| `number` | Remaining headroom before saturation (req/s). |
| `base_latency` | `number` | Geographical baseline propagation latency (ms). |
| `latency` | `number` | Dynamic response time under queuing load ($M/M/1$) (ms). |
| `bandwidth_cost` | `number` | Transit financial cost (₹/GB). |
| `cpu_utilization` | `number` | Dynamic compute load percentage. |
| `bandwidth_usage` | `number` | Current network egress rate (Mbps). |
| `status` | `string` | Operational status: `"HEALTHY"`, `"CONGESTED"`, or `"DOWN"`. |
| `is_failed` | `boolean` | Flag indicating complete hardware failure / outage. |
| `is_congested` | `boolean` | Flag indicating severe queuing congestion. |

---

### `routing` Object Schema

```json
{
  "schema_version": 2,
  "cycle_id": 42,
  "timestamp": "2026-10-01T22:45:00Z",
  "requested_traffic": 11591,
  "routed_traffic": 11591,
  "unrouted_traffic": 0,
  "total_cost_rs": 457186.00,
  "routing_status": "OPTIMAL",
  "change_detected": true,
  "trigger_reason": "Traffic demand changed: 11343 -> 11591 req/s",
  "engine_solve_time_us": 70.9,
  "execution_time_ms": 0.071,
  "previous_allocation": { "Pune": 343, "Mumbai": 6000, "Delhi": 0, "Bangalore": 5000 },
  "new_allocation": { "Pune": 591, "Mumbai": 6000, "Delhi": 0, "Bangalore": 5000 },
  "allocation_diff": { "Pune": 248, "Mumbai": 0, "Delhi": 0, "Bangalore": 0 },
  "server_details": [ ... ]
}
```

| Field | Type | Description |
|---|---|---|
| `cycle_id` | `integer` | Routing decision iteration number. |
| `requested_traffic` | `integer` | Total ingress demand submitted for routing (req/s). |
| `routed_traffic` | `integer` | Flow successfully routed across active edge servers (req/s). |
| `unrouted_traffic` | `integer` | Excess traffic exceeding total fleet capacity (req/s), routed via origin fallback. |
| `total_cost_rs` | `number` | Weighted composite cost objective value: $W = \sum f_e \cdot c_e$, where $c_e = \alpha \cdot L_e + \beta \cdot B_e$ (with latency weight $\alpha$ and bandwidth cost weight $\beta$). Unless explicit regional currency multipliers are calibrated in config, this metric is treated as a unitless weighted routing penalty ("Weighted cost"). |
| `routing_status` | `string` | Status badge: `"OPTIMAL"`, `"CONGESTED_REROUTE"`, `"FAILOVER"`, or `"INSUFFICIENT_CAPACITY"`. |
| `change_detected` | `boolean` | `true` if conditions changed requiring MCMF recomputation; `false` if cached. |
| `trigger_reason` | `string` | Plain-language reason for rerouting (e.g. demand shift, node failure). |
| `previous_allocation`| `object` | Key-value map of server name to prior routed flow. |
| `new_allocation` | `object` | Key-value map of server name to newly assigned flow. |
| `allocation_diff` | `object` | Shift in assigned requests ($\text{new} - \text{prev}$). |
| `server_details` | `array[object]` | Per-node financial and utilization breakdown. |

---

### Phase 5 Algorithm Trace Extension (`trace[]`)

When generated via `--trace` or `{"cmd": "run_trace"}`, the payload includes the complete iteration trace:

```json
{
  "iteration": 1,
  "dist": [0, 0, 0, 0, 2000, 2800, 3400, 37000, 2500],
  "potentials_before": [0, 0, 0, 0, 0, 0, 0, 0, 0],
  "potentials_after": [0, 0, 0, 0, 2000, 2500, 2500, 2500, 2500],
  "chosen_path_nodes": [0, 1, 4, 8],
  "chosen_path_names": ["Source (User Ingress)", "Reg:Pune_Reg", "Edge:Alpha_S1", "Sink (Terminal)"],
  "reduced_path_cost": 2500,
  "real_path_cost": 2500,
  "bottleneck_delta": 100,
  "flow_pushed": 100,
  "cumulative_flow": 100,
  "cumulative_cost": 250000,
  "residual_graph": [
    {
      "from": 0,
      "to": 1,
      "from_name": "Source (User Ingress)",
      "to_name": "Reg:Pune_Reg",
      "capacity": 100,
      "flow": 100,
      "residual_capacity": 0,
      "cost": 0
    }
  ]
}
```

---

## 3. IPC Protocol Specification (Newline-Delimited JSON)

The engine daemon runs in interactive IPC mode (`cdn_engine --ipc`).
Communication occurs over `stdin` (client commands) and `stdout` (engine responses).

### Framing Rules
1. Every command sent to `stdin` must be a **single line of valid JSON** terminated by `\n`.
2. Every response emitted to `stdout` is guaranteed to be a **single line of valid JSON** terminated by `\n`.
3. Standard error (`stderr`) is reserved exclusively for debug diagnostics.

### Supported IPC Commands

| Command (`"cmd"`) | Parameters | Description |
|---|---|---|
| `"tick"` | `{"drift": bool}` | Advances simulation clock by 1 tick, applies Poisson noise + diurnal cycle, and recomputes routing. |
| `"run_scenario"` | `{"scenario": string}` | Executes one of the 7 validation scenarios (`"normal"`, `"spike"`, `"pune"`, `"mumbai"`, `"multi"`, `"overload"`, `"recovery"`). |
| `"spike"` | `{"server": string, "target_traffic": number}` | Injects an abrupt demand surge targeting the specified node. |
| `"fail"` | `{"server": string}` | Forces catastrophic server failure (transitions status to `DOWN`, reroutes via MCMF). |
| `"recover"` | `{"server": string, "initial_traffic": number}` | Restores failed server back online (`HEALTHY`). |
| `"set_traffic"` | `{"server": string, "traffic": number}` | Explicitly assigns incoming traffic rate to a server. |
| `"set_strategy"`| `{"strategy": string}` | Switches dispatch strategy (`"MCMF"`, `"ROUND_ROBIN"`, `"NEAREST"`, `"GREEDY"`). |
| `"run_trace"` | `{"topology": "3x3"}` | Computes full hand-verifiable algorithm step trace. |
| `"snapshot"` | None | Returns the current state without advancing the simulation clock. |
| `"ping"` | None | Health-check command, returns `{"status": "ok", "pong": true}`. |
| `"quit"` | None | Shuts down the engine process cleanly with exit code 0. |

---

## 4. Fallback Mode Specification

For testing, backward compatibility, or containerized environments without named pipes:
- **Flag**: `use_subprocess_fallback = True` (or `POST /api/engine/mode {"mode": "subprocess"}`).
- **Behavior**: Instead of keeping `cdn_engine` resident in memory, Python invokes `cdn_engine.exe --scenario <arg>` per event via `subprocess.run()`.
- **Performance Trade-Off**:
  - **Long-Lived IPC**: End-to-end event latency $\approx \mathbf{2.68\text{ ms}}$ (Pipe transmission + JSON parse).
  - **Subprocess Fallback**: End-to-end event latency $\approx \mathbf{52.5\text{ ms}}$ ($\approx 20\times$ slower due to Windows OS `CreateProcess`, DLL resolution, and process teardown).
  - In both modes, the C++ MCMF solve time itself remains unchanged at $\approx \mathbf{70.9\ \mu\text{s}}$.
