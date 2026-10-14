# UPGRADE BRIEF: Adaptive CDN Traffic Router (DAA Capstone)

This file is the single reference for upgrading the project. Re-read it whenever you are unsure about scope, rules, or what "done" means.

---

## 1. Context

This is a Design and Analysis of Algorithms (DAA) capstone project by a 3rd-year B.Tech student. The goal is a real-time CDN traffic router that models the CDN as a flow network and uses Min-Cost Max-Flow (MCMF) to route traffic to edge servers while minimizing latency and cost and respecting capacity.

### What already exists in the repo

- `DAA/`: C++17 engine
  - Graph and Edge (adjacency list with reverse residual edges)
  - MinCostMaxFlow: Successive Shortest Path with Dijkstra and Johnson potentials
  - Router: threshold/debounce drift detector and allocation diffing
  - CDNServer and TrafficSimulator
  - SimulationScenarioRunner with 7 scenarios (Normal Traffic, Traffic Spike, Pune Congestion, Mumbai Failure, Multiple Congestion, Total Fleet Overload, Fleet Recovery)
  - BenchmarkSuite
  - `build.bat`, producing `bin/cdn_engine.exe` with modes `--scenarios`, `--bench`, `--loop`, `--demo`
- `integration/`: Python and web tier
  - Python physics simulator (M/M/1 queuing) in `cdn_sim/`
  - FastAPI and WebSocket server (`dashboard/web_app.py`) that runs the C++ binary as a subprocess
  - Redis syncer (`redis/redis_syncer.py`)
  - Vanilla HTML/CSS/JS glassmorphism dashboard (`dashboard/static/index.html`)
- `data/routing_decision.json`: the JSON contract between C++, Python and the UI

File names come from the project report. Explore the repo first and adapt to the real structure.

### The core weakness to fix

The current topology is `source -> 4 edge servers (Pune, Mumbai, Delhi, Bangalore) -> sink`. There is no real routing choice, so MCMF degenerates into "sort servers by cost and fill the cheapest first". The number of augmentations is at most 4, and a solve takes about 0.003 ms. This does not showcase the algorithm.

---

## 2. Goals (in priority order)

1. The C++ core must showcase real DAA concepts (network flow, min-cut theorem, greedy/Dijkstra, dynamic programming, asymptotic analysis), not just wrap a trivial problem.
2. The demo must make the algorithms visible and prove they beat simpler approaches.
3. Everything must stay correct, deterministic and explainable in a viva.

---

## 3. Hard rules

- Core algorithms stay **pure C++17, standard library only, written from scratch**. No Boost, LEMON or OR-tools. A small single-header HTTP library is allowed for transport only (Phase 6).
- Keep the Windows build working (`build.bat`) and add a Makefile or CMake for Linux and macOS.
- Use **seeded RNG** everywhere so every demo and test is reproducible.
- Keep **fixed-point integer costs** (`cost = round(100 * value)`) to avoid floating-point issues.
- Do not delete existing scenarios or features. If a result changes, update the expected values and explain why.
- Do not add dependencies or rewrite unrelated code. Stay inside the current phase.
- Prefer clear, commented, readable code over clever code. The author must be able to explain every line in a viva.

## 4. How to work

- Work **one phase at a time**, only the phase requested in the current message.
- Before any coding, read the existing code and write a short audit (what is good, what is redundant, what will change).
- After each phase: build, run all tests, summarise what changed, list the files touched, then **stop and wait for my go-ahead**.
- If something is ambiguous but not blocking, make a reasonable assumption, state it, and continue. Ask a question only if you are truly blocked.
- If something fails, find the root cause. Never weaken a test to make it pass.

### Phase map

| Phase | Topic |
|---|---|
| 1 | Non-trivial flow network (regions, origin fallback, convex edges, counterexample) |
| 2a | Edmonds-Karp, min-cut, Bellman-Ford reference solver, baselines |
| 2b | Cache DP, Holt forecast, hysteresis |
| 3 | `--selftest` verification mode |
| 4 | Scalability benchmarks on random graphs |
| 5 | Algorithm trace mode |
| 6 | Architecture cleanup |
| 7 | Dashboard (7a: panels 1-4, 7b: panels 5-10) |
| 8 | Documentation and viva readiness |

---

## PHASE 1: Make the flow problem non-trivial

1. **Config-driven graph.** Generalize Graph so it is built from a config file (JSON or simple text): N user regions, M edge servers, per-region demand, per-link capacity and latency, per-server capacity and bandwidth cost.

2. **New network:**

   ```
   S -> Region_i      (cap = demand_i, cost 0)
   Region_i -> Edge_j (cap = link bandwidth, cost = alpha*latency_ij + beta*bandwidthCost_j, scaled)
   Edge_j -> T        (capacity split into convex utilization bands, see item 3)
   Region_i -> Origin -> T  (cap = INF, cost = very large)
   ```

   The origin edge makes the problem always feasible. Flow on it means traffic not served by the edge fleet (a cache miss goes to origin). Remove the special-case "unrouted" logic where the origin edge replaces it, and keep the `INSUFFICIENT_CAPACITY` status derived from origin flow.

   Default config: 8 regions (Pune, Mumbai, Delhi, Bangalore, Chennai, Hyderabad, Kolkata, Ahmedabad) and 5 to 6 edge servers.

3. **Convex load cost.** Replace each `Edge_j -> T` edge with parallel edges for utilization bands, for example:

   | Utilization band | Cost multiplier |
   |---|---|
   | 0-50% | 1.0 x c |
   | 50-80% | 1.5 x c |
   | 80-95% | 3.0 x c |
   | 95-100% | 8.0 x c |

   Because band costs are increasing, SSP fills cheaper bands first, so M/M/1-style convex delay is modelled exactly with a linear solver. Make band boundaries and multipliers configurable.

4. **Preset for the old topology.** Keep the original 4-server topology available as a preset so the 7 existing scenarios still run and pass.

5. **Counterexample scenario.** Add a small instance where "each region picks its nearest or cheapest server in order" (greedy) is strictly worse than MCMF. Guide: Region A can use server X (cost 1) or Y (cost 2); Region B can only use X; X has limited capacity. Greedy in region order lets A fill X, pushing B to origin or a very expensive path. MCMF sends A to Y and B to X. Print both results side by side with total cost.

**Acceptance:** old scenarios still pass, the counterexample shows MCMF strictly better, flow conservation holds on the new network.

---

## PHASE 2a: Max-flow, min-cut, reference solver, baselines

1. **Edmonds-Karp** (BFS augmenting paths) max-flow, from scratch.

2. **Min-cut extraction.** After max-flow, BFS the residual graph from S. Output the S-side set and the saturated edges crossing the cut. Assert `cut capacity == max-flow value` (max-flow min-cut theorem). Run it on the network **without** the origin fallback to answer: how much demand can the edge fleet serve, and which servers or links are the bottleneck. Output a capacity-upgrade recommendation (for example "add capacity at server X").

3. **Reference solver.** SSP using Bellman-Ford or SPFA (no potentials), behind a flag. It is used for verification and for benchmarking against Dijkstra with potentials.

4. **Baselines**, using the same input and output format as MCMF:
   - Round-robin
   - Nearest-geographic
   - Greedy cheapest-first (per region, in region order)

   Report per strategy: total cost, average latency, p95 latency (demand-weighted), number of overloaded servers (above 90% utilization), and unserved/origin traffic.

**Acceptance:** one comparison table over all scenarios and all strategies; min-cut output shown for the Total Fleet Overload scenario.

---

## PHASE 2b: Cache DP, forecast, stability

5. **Cache placement DP.** Per edge server, a 0/1 knapsack over a content catalog (Zipf popularity, item sizes, storage limit).
   - `benefit = popularity * (originLatency - edgeLatency)`
   - Backtrack to list the chosen items.
   - `hitRatio_j = cached popularity / total popularity`
   - Feed it into the edge cost: `cost += (1 - hitRatio_j) * originFetchCost`
   - Also implement greedy-by-density and show instances where it is worse than the DP (this demonstrates why 0/1 knapsack needs DP).
   - Re-run the DP only every K ticks, not every tick.

6. **Holt's linear smoothing** per region (level and trend, k-step-ahead forecast, O(1) update). The router has two modes: REACTIVE (current demand) and PROACTIVE (forecast demand). Add a spike scenario comparing both: peak overload, time to recover, total cost.

   ```
   level = a*x + (1-a)*(level + trend)
   trend = b*(level - prevLevel) + (1-b)*trend
   forecast_k = level + k*trend
   ```

7. **Router stability.** Add hysteresis (only reroute if the improvement exceeds a threshold) and an optional migration penalty (extra cost for moving traffic away from the previous allocation). Report route changes per 100 ticks with and without hysteresis.

**Acceptance:** a case where DP beats greedy-by-density; proactive vs reactive numbers; fewer route changes with hysteresis.

---

## PHASE 3: Correctness and verification (`--selftest`)

Implement a self-test mode that needs no UI and prints PASS or FAIL for each check, plus a summary table:

- Flow conservation at every non-terminal node, and `0 <= flow <= capacity` on every edge.
- Reduced costs non-negative after every potential update (assert inside the solver in debug builds).
- Dijkstra+potentials total cost equals Bellman-Ford SSP total cost on 1000+ random instances.
- On tiny instances (at most 3 regions, 3 servers, small integer capacities), compare against brute-force enumeration of all integer flows.
- Edmonds-Karp max-flow value equals MCMF flow value (with origin fallback removed) and equals min-cut capacity.
- Knapsack DP result equals brute-force subset enumeration on small catalogs.
- Total demand equals edge-served traffic plus origin traffic.

---

## PHASE 4: Benchmarks that measure the right thing

1. Seeded random network generator for 10, 50, 100, 500 and 1000 servers, with proportional numbers of regions.
2. Measure: graph build time, solve time, number of augmentations K, edges E, vertices V. Run each size several times and report the median.
3. Compare Dijkstra+potentials against Bellman-Ford SSP on the same instances.
4. Output CSV and overlay the theoretical `O(K * E log V)` curve. Report the ratio measured/theoretical to show it is roughly constant.
5. Remove or relabel the old "1k-50k req/s demand" benchmark. Runtime does not depend on demand, only on V, E and K, so state that in the report.

---

## PHASE 5: Algorithm trace mode (most important for the demo)

Add a `--trace` flag and a `trace` array in the JSON. For every SSP augmentation record:

- iteration number
- `dist[]` under reduced costs
- potentials `pi[]` before and after
- chosen path (node list)
- reduced cost and real cost of the path
- bottleneck delta and flow pushed
- cumulative flow and cumulative cost
- compact residual-graph snapshot (edge, residual capacity, cost)

Also trace Edmonds-Karp (BFS layers, path, bottleneck) and the min-cut BFS.

**Acceptance:** on a small 3-region, 3-server example, the trace can be verified by hand.

---

## PHASE 6: Architecture cleanup

1. Spawning the `.exe` per event costs more than the solve itself. Replace it with a long-lived engine process, using either:
   - (a) stdin/stdout newline-delimited JSON commands (`tick`, `spike`, `fail`, `recover`, `set_strategy`, `run_scenario`, `run_trace`, `set_config`), or
   - (b) an embedded single-header HTTP server (for example cpp-httplib), used for transport only.

   Keep the old subprocess path as a fallback flag.
2. Make the **C++ simulator the single source of truth** for demand generation (Poisson + diurnal sine + random spikes), server health and latency. Python becomes a thin UI gateway (FastAPI and WebSocket) with no duplicate physics. Keep Redis as an optional sink only and document it as an extension.
3. Keep `data/routing_decision.json` as the contract, but version it (`"schema_version": 2`) and write it atomically (write a temp file, then rename). Document the schema in `docs/SCHEMA.md`.
4. Report engine solve time (microseconds) separately from end-to-end latency in the UI.

---

## PHASE 7: Dashboard

Keep vanilla JS and CSS in the existing glassmorphism style, no heavy frameworks. SVG or Canvas is fine.

**7a (panels 1-4):**

1. **Network graph** (main screen): regions on the left, edge servers in the middle, origin on the right. Edge thickness is flow, color is utilization (green, yellow, red), dashed edges for the origin fallback, failed servers greyed out.
2. **Strategy toggle:** Round-Robin / Nearest / Greedy / MCMF on the same live traffic, with a side-by-side comparison table and rolling charts (average latency, p95, overloaded servers, total cost).
3. **Min-cut overlay** toggle: highlight the cut and show "max-flow = min-cut = X" with a bottleneck/upgrade message.
4. **Algorithm trace stepper:** play, pause, next and previous through augmentations. Highlight the current path on the graph and show a side table of dist, potentials, bottleneck, running flow and cost. Include a note on why potentials are needed (Bellman-Ford vs Dijkstra).

**7b (panels 5-10):**

5. **Controls:** spike per region, fail and recover a server, demand slider, speed slider, scenario dropdown (the 7 old ones plus the counterexample and forecast spike), reactive vs proactive toggle, hysteresis toggle.
6. **Forecast panel:** actual vs Holt forecast per region, with a marker where proactive rerouting triggered.
7. **Cache panel:** click a server to see the DP-chosen content, its hit ratio, and the DP vs greedy-by-density comparison.
8. **Scalability page:** a button that runs the benchmark sizes and plots measured runtime against the theoretical curve, and Dijkstra vs Bellman-Ford.
9. **"Why this decision?" panel:** plain-language explanation of the current routing (for example "Mumbai is at 82%, so the next unit costs 3x; Pune's traffic was shifted to Bangalore").
10. **Export** current results as CSV and JSON for the report.

Also write `docs/DEMO.md`: a 5-minute demo script that walks through the panels in the best order.

---

## PHASE 8: Documentation and viva readiness

1. Update the technical report with the new model, convex edges, all algorithms and their complexity. State the general bound (O(F) augmentations for integer capacities) plus the **measured** K. Include proofs: non-negativity of reduced costs with Johnson potentials, the max-flow min-cut theorem, and optimal substructure for the knapsack. Use real numbers from actual runs; never invent results.
2. Fix known inaccuracies:
   - Say "negative residual **edges**", not cycles.
   - Be consistent about custom heap vs `std::priority_queue`.
   - Use `cap(u,v)` for capacity, not `u(u,v)`.
   - Complete the complexity table.
   - The old `K <= 4` argument applied only to the 6-node topology; say so.
3. Write a README with build and run steps and the 5-minute demo script.
4. Write `docs/VIVA_QA.md` with at least 25 likely examiner questions and answers, for example: why Dijkstra needs potentials, why K was at most 4 before and what changed, why convex edges, why DP and not greedy for caching, what the min-cut tells us, and the limits of the model (single-commodity flow vs content-level routing).
5. Add a limitations and future work section: multi-commodity flow is NP-hard for integer flows, real CDNs use DNS/anycast and approximate heuristics, and the production path is Go/Rust with gRPC and Kubernetes.

---

## Definition of done

- [x] `cdn_engine --selftest` passes every check.
- [x] The counterexample scenario shows MCMF strictly better than greedy, and the comparison table shows MCMF best or tied on cost, latency and overload in every scenario.
- [x] Min-cut equals max-flow in every test; trace mode replays augmentations correctly.
- [x] Benchmark CSV and plots exist for up to 1000 servers, including Dijkstra vs Bellman-Ford.
- [x] The dashboard runs end-to-end with the long-lived engine and all panels work.
- [x] Docs (report, README, SCHEMA, DEMO, VIVA_QA) are updated and consistent with the code.

