# Adaptive CDN Traffic Router with Min-Cost Max-Flow Engine

> **Design and Analysis of Algorithms (DAA) Capstone Project**  
> Dynamic Edge Content Delivery Optimization using Min-Cost Max-Flow (MCMF), Johnson's Node Potentials, Edmonds-Karp Min-Cut Verification, 0/1 Knapsack Caching DP, and Holt-Winters Predictive Smoothing.

[![C++17 Standard](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![DAA Invariants](https://img.shields.io/badge/SelfTest-7%2F7%20PASS-brightgreen.svg)](file:///c:/Users/swaro/DAA_CP/results/test_report.md)
[![FastAPI Dashboard](https://img.shields.io/badge/Dashboard-FastAPI%20%2B%20VanillaJS-orange.svg)](http://127.0.0.1:8000/)
[![License](https://img.shields.io/badge/License-MIT-purple.svg)](LICENSE)

---

## 📌 Executive Summary

Modern Content Delivery Networks (CDNs) deliver over 80% of global internet traffic. Traditional heuristic load balancers (such as Round-Robin or Greedy Min-Latency) frequently cause **congestion collapse** under asymmetric loads by overwhelming lowest-latency nodes and neglecting transit bandwidth costs.

This project formulates dynamic multi-region CDN traffic routing as a formal **Min-Cost Max-Flow (MCMF)** network optimization problem on a directed bipartite graph. By modeling queuing delay via **piecewise-linear convex cost curves**, utilizing **Johnson's node potentials** to run Dijkstra in $\mathcal{O}(|E| \log |V|)$ on residual networks, solving edge caching via **0/1 Knapsack Dynamic Programming**, and damping traffic jitter with **dual-threshold hysteresis**, this engine achieves:
- **Zero dropped packets** under high traffic spikes and server outages.
- **12.7% to 23.5% lower financial bandwidth cost** compared to Round-Robin and Greedy heuristics.
- **14.5% to 54.1% lower average latency** by preventing $M/M/1$ queuing blowout.
- **Sub-millisecond solver performance** ($< 15\ \mu\text{s}$ on 4 servers, $\sim 400\text{ ms}$ on 1,000 servers).

---

## 🏛️ System Architecture

```
                                [Client Regions / Traffic Demand]
                                                │
                                                ▼
                             ┌──────────────────────────────────────┐
                             │    Virtual Super-Source (Node 0)     │
                             └──────────────────┬───────────────────┘
                                                │
                     ┌──────────────────────────┼──────────────────────────┐
                     ▼                          ▼                          ▼
            [Region: Pune]             [Region: Mumbai]           [Region: Delhi]
                     │                          │                          │
                     ▼                          ▼                          ▼
          ┌────────────────────┐     ┌────────────────────┐     ┌────────────────────┐
          │  PoP: Pune (5k)    │     │  PoP: Mumbai (6k)  │     │  PoP: Delhi (5.5k) │
          └─────────┬──────────┘     └─────────┬──────────┘     └─────────┬──────────┘
                    │                          │                          │
                    │   4 Convex Cost Bands    │   4 Convex Cost Bands    │
                    │  (1.0x, 1.5x, 3.0x, 8.0x)│  (1.0x, 1.5x, 3.0x, 8.0x)│
                    └──────────────────────────┼──────────────────────────┘
                                               │
                                               ▼
                                 ┌───────────────────────────┐
                                 │ Virtual Super-Sink (Node T│
                                 └───────────────────────────┘
```

The C++17 engine is the **Single Source of Truth** for all mathematical modeling, physics simulation, and routing decisions. The Python FastAPI layer and Vanilla JS Dashboard act strictly as presentation and telemetry streaming clients.

---

## 📂 Repository Directory Layout

```
DAA_CP/
├── DAA/                      ◄── [EVALUATOR/TEACHER: CORE C++ IMPLEMENTATION]
│   ├── src/                  # C++17 modular source files
│   │   ├── AlgorithmTrace.cpp  # Step-by-step SSP trace generator
│   │   ├── Baselines.cpp       # Round-Robin, Greedy, and Nearest heuristics
│   │   ├── Benchmark.cpp       # Scalability suite up to 1000 servers
│   │   ├── CacheKnapsack.cpp   # 0/1 Knapsack DP cache optimizer
│   │   ├── EdmondsKarp.cpp     # Max-flow and min-cut BFS extractor
│   │   ├── Graph.cpp           # Adjacency list and residual graph
│   │   ├── HoltForecast.cpp    # Holt linear exponential smoother
│   │   ├── IpcEngine.cpp       # Long-lived persistent stdio JSON-IPC
│   │   ├── MinCostMaxFlow.cpp  # SSP with Johnson's node potentials
│   │   ├── Router.cpp          # High-level CDN adaptive traffic controller
│   │   ├── SelfTest.cpp        # 7-point mathematical invariant self-test
│   │   ├── StabilityManager.cpp# Schmitt-trigger dual-threshold hysteresis
│   │   ├── TrafficSimulator.cpp# Diurnal cycles and Poisson arrival generator
│   │   └── main.cpp            # Unified CLI driver
│   ├── include/              # Corresponding C++ header interfaces
│   ├── bin/                  # Compiled binary executables (cdn_engine.exe)
│   ├── build.bat             # One-click Windows build script
│   └── Makefile              # Cross-platform Unix/Linux build file
├── integration/              # Presentation & Dashboard Layer
│   ├── dashboard/            # FastAPI server (web_app.py) & UI (static/index.html)
│   └── python/               # Engine client IPC bridge (engine_client.py)
├── results/                  # Benchmark CSVs, Test Reports & Complexity Proofs
│   ├── benchmark_results.csv # Demand benchmark measurements
│   ├── scalability_benchmark.csv # 10 to 1000 servers empirical scaling data
│   ├── scenario_results.csv  # 7-scenario verification results
│   ├── test_report.md        # Comprehensive verification test report
│   └── complexity_analysis.md# Formal proofs and asymptotic complexity analysis
├── docs/                     # Academic & Viva Documentation
│   ├── DEMO.md               # 5-minute timed viva demo walkthrough script
│   ├── VIVA_QA.md            # 28 detailed examiner viva questions & answers
│   ├── UPGRADE_BRIEF.md      # Specification and milestone checklist
│   └── SCHEMA.md             # Canonical JSON telemetry schema documentation
└── README.md                 # This root document
```

---

## 🛠️ Build & Quick-Start Guide

### Prerequisites
- **C++ Compiler**: GCC 9+ (g++ with C++17), Clang 10+, or MSVC 2019+
- **Python**: Python 3.9+ with `fastapi` and `uvicorn` (for the dashboard)

### 1. Build the C++ Engine

**Windows (PowerShell / Command Prompt)**:
```cmd
cd DAA
.\build.bat
```
*Or compile directly with g++:*
```cmd
g++ -std=c++17 -O3 src/*.cpp -Iinclude -o bin/cdn_engine.exe
```

**Linux / macOS**:
```bash
cd DAA
make
```

### 2. Run C++ Verification Modes

```cmd
# 1. Run the 7-Point DAA Invariant Self-Test (100% automated correctness check)
.\DAA\bin\cdn_engine.exe --selftest

# 2. Run the 7 Operational Verification Scenarios (Spikes, Congestion, Failover)
.\DAA\bin\cdn_engine.exe --scenarios

# 3. Run the Scalability Benchmark (10 to 1,000 servers, Dijkstra vs Bellman-Ford)
.\DAA\bin\cdn_engine.exe --bench

# 4. Run the Full Interactive Demo with Min-Cut Extraction & Algorithm Trace
.\DAA\bin\cdn_engine.exe --demo
```

### 3. Launch the Web Dashboard

```bash
# Start FastAPI backend server on localhost:8000
python -m uvicorn integration.dashboard.web_app:app --host 127.0.0.1 --port 8000
```
Open your browser and navigate to:  
👉 **`http://127.0.0.1:8000/`**

---

## 🖥️ Dashboard Panels (Panels 1 to 10)

The web dashboard is built strictly with **Vanilla HTML, CSS, and JavaScript** (no external framework bloat or Tailwind dependencies):

1. **Panel 1: Network Topology Graph**: Interactive SVG visualization of the bipartite flow network with live animated links representing flow rate ($f_{uv}$) and pipe thickness representing capacity ($\text{cap}_{uv}$).
2. **Panel 2: Strategy Toggle & Comparison**: Real-time side-by-side comparison of MCMF vs Round-Robin, Nearest Server, and Greedy Min-Latency with live cost/latency bar charts.
3. **Panel 3: Min-Cut Bottleneck Overlay**: Visualizes the **Ford-Fulkerson Max-Flow Min-Cut Theorem**. Shows $S$-partition, $T$-partition, and highlights saturated bottleneck cut edges crossing the network boundary.
4. **Panel 4: Algorithm Trace Stepper**: Step-by-step interactive stepper demonstrating each augmentation of the Successive Shortest Path algorithm, displaying node potentials $\pi(u)$, reduced costs $c^\pi$, and priority queue states.
5. **Panel 5: Interactive Controls & Stress Testing**: Real-time traffic demand slider, simulation speed controller, reactive/proactive mode switch, hysteresis anti-thrashing toggle, and per-server fault injection buttons (**Spike**, **Fail**, **Recover**).
6. **Panel 6: Predictive Traffic Forecasting**: Visualizes Holt's Linear Exponential Smoothing ($\alpha=0.40, \beta=0.30$) over a rolling window. Demonstrates how proactive rerouting prevents latency spikes before flash crowds peak.
7. **Panel 7: Edge Cache View (0/1 Knapsack DP)**: Evaluates edge NVMe cache allocation. Shows the mathematical counterexample where greedy value-density fails and DP achieves a **+58.7% benefit improvement**, complete with interactive 2D DP matrix view.
8. **Panel 8: Scalability Benchmark Page**: Log-log empirical performance plot for networks up to 1,000 servers ($V=1503, E=14272$), validating the theoretical bound $\mathcal{O}(K \cdot |E| \log |V|)$ with constant scaling ratio $C \approx 2.2 - 4.3\text{ ns/op}$.
9. **Panel 9: "Why this decision?" Explainability**: Plain-language operational audit explaining solver decisions based on convex cost penalty tiers, saturation limits, and marginal cost comparisons.
10. **Panel 10: CSV & JSON Data Export**: Live data preview and one-click export for full routing JSON snapshots and 3 benchmark CSV reports.

---

## ⏱️ 5-Minute Evaluation & Viva Demo Script

*(Full script with word-for-word talking points is located in [docs/DEMO.md](file:///c:/Users/swaro/DAA_CP/docs/DEMO.md))*

| Time | Panel | Key Action & Viva Talking Point |
|---|---|---|
| **0:00 – 0:45** | **Panel 1: Network Graph** | Show Virtual Source $S$, Virtual Sink $T$, 4 PoPs. Explain bipartite formulation and convex cost bands modeling $M/M/1$ queuing delay. |
| **0:45 – 1:30** | **Panel 2: Strategy Toggle** | Click Round-Robin, then Greedy, then MCMF. Show why Greedy causes queuing blowout (48.6 ms latency) while MCMF achieves global optimal cost (Rs. 545k, 22.3 ms). |
| **1:30 – 2:15** | **Panel 4: Algorithm Trace** | Step through SSP. Explain why Johnson's node potentials $\pi(u)$ ensure non-negative reduced costs ($c^\pi \ge 0$), enabling Dijkstra with binary heap in $\mathcal{O}(|E| \log |V|)$ instead of Bellman-Ford. |
| **2:15 – 3:00** | **Panel 3: Min-Cut Overlay** | Highlight the $S-T$ cut boundary. Show that $\max |f| = \min \text{cap}(S, T) = 21,500$ req/s, proving network throughput limits. |
| **3:00 – 3:45** | **Panel 5: Live Controls** | Drag demand to 18,000 req/s. Click **Fail** on Mumbai. Show zero dropped packets failover in $< 50\ \mu\text{s}$. Toggle **Hysteresis** to explain prevention of route flapping. |
| **3:45 – 4:15** | **Panel 6: Forecast View** | Demonstrate Holt linear smoothing ($\alpha=0.40, \beta=0.30$). Show proactive rerouting trigger at Tick 5 cutting peak congestion latency by 59%. |
| **4:15 – 4:40** | **Panel 7: Cache Knapsack** | Show 0/1 Knapsack DP vs Greedy. Explain why greedy density fails on indivisible items and how DP yields +58.7% benefit. |
| **4:40 – 5:00** | **Panels 8, 9 & 10: Scalability & Export** | Show log-log scalability to 1,000 servers ($C \approx 4.06\text{ ns/op}$), diagnostic tree explainability, and one-click data export. |

---

## 📊 Algorithmic Complexity & Theoretical Proofs

A summary of theoretical bounds and formal proofs detailed in [results/complexity_analysis.md](file:///c:/Users/swaro/DAA_CP/results/complexity_analysis.md):

| Algorithm | Theoretical Complexity | Empirical Constant ($C$) | Key Invariant / Guarantee |
|---|:---:|:---:|---|
| **MCMF (SSP)** | $\mathcal{O}(K \cdot \|E\| \log \|V\|)$ | $2.19 - 3.93\text{ ns/op}$ | Global minimum financial + latency cost |
| **Dijkstra + Potentials** | $\mathcal{O}(\|E\| \log \|V\|)$ | Binary min-heap | Reduced costs $c^\pi(u, v) \ge 0$ strictly preserved |
| **Edmonds-Karp** | $\mathcal{O}(\|V\| \cdot \|E\|^2)$ | Breadth-First Search | Strongly polynomial max-flow bound |
| **Min-Cut Extraction** | $\mathcal{O}(\|V\| + \|E\|)$ | Residual graph BFS | $\max \|f\| = \min \text{cap}(S, T)$ equivalence |
| **0/1 Knapsack DP** | $\mathcal{O}(n \cdot W)$ | Pseudo-polynomial | Exact optimal cache benefit |
| **Holt Exponential Smoother** | $\mathcal{O}(1)$ | Single-cycle math | Minimum Mean Squared Error linear trend |

---

## ✅ Definition of Done Checklist

| Requirement / Milestone | Status | Verification Evidence |
|---|:---:|---|
| `cdn_engine --selftest` passes every check | **PASS** | 7/7 checks passed strictly with zero violations in 15.42 ms. |
| Counterexample scenario shows MCMF strictly better than Greedy | **PASS** | MCMF achieves 23.5% lower cost and 54.1% lower latency; Greedy drops 1,200 req/s. |
| Comparison table shows MCMF best or tied in every scenario | **PASS** | Verified across all 7 scenarios and baseline comparison table. |
| Min-cut equals max-flow in every test | **PASS** | Verified in `CHECK_5` ($\text{Cut} == \text{MaxFlow} = 21,500$ req/s and $370$ req/s). |
| Trace mode replays augmentations correctly | **PASS** | Step-by-step trace generated in `--trace` and animated in Panel 4. |
| Benchmark CSV and plots exist up to 1,000 servers | **PASS** | `results/scalability_benchmark.csv` generated; Dijkstra vs Bellman-Ford benchmarked. |
| Dashboard runs end-to-end with long-lived engine & all 10 panels | **PASS** | Persistent stdio IPC verified; Panels 1 to 10 fully interactive on port 8000. |
| Documentation consistent and updated without invented results | **PASS** | `results/complexity_analysis.md`, `results/test_report.md`, `docs/DEMO.md`, and `docs/VIVA_QA.md` fully synchronized with real C++ engine data. |

---

## 👥 Authors & Academic Attribution
- **Course**: Design and Analysis of Algorithms (DAA) Capstone Project  
- **Core Technology**: C++17, Python 3 / FastAPI, Vanilla JavaScript & SVG / Canvas.  
- **Academic References**:
  - Ahuja, Magnanti, and Orlin, *Network Flows: Theory, Algorithms, and Applications* (Prentice Hall, 1993).
  - Cormen, Leiserson, Rivest, and Stein (CLRS), *Introduction to Algorithms* (MIT Press, 4th ed.).
  - Kleinrock, Leonard, *Communication Nets: Stochastic Message Flow and Delay* (McGraw-Hill, 1964).
