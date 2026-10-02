# DAA Algorithm Test Suite

This directory contains standalone unit tests and test documentation for the DAA C++ Engine.

---

## 📋 Available Tests

### 1. `test_server.cpp`
- **Focus**: CDN Edge Server Unit Testing.
- **Validates**:
  - Initial configuration of Pune, Mumbai, Delhi, and Bangalore.
  - Utilization percentage calculations: $\frac{\text{Traffic}}{\text{Capacity}} \times 100\%$.
  - M/M/1 non-linear queuing latency model.
  - Server failure state: capacity clamped to 0, latency $\to \infty$, status `FAILED`.
  - Server recovery state: restoration to nominal capacity and latency.

### 2. `test_mcmf.cpp`
- **Focus**: Flow Network & Min-Cost Max-Flow Unit Testing.
- **Validates**:
  - Graph construction with 6 nodes (Source, 4 Edge Servers, Origin Sink).
  - Forward edges and reverse residual edges with anti-symmetric costs ($c(v, u) = -c(u, v)$).
  - Dijkstra with Johnson's Node Potentials yielding non-negative reduced costs ($c^\pi(u, v) \ge 0$).
  - Successive Shortest Path (SSP) augmenting paths and bottleneck capacity pushes.
  - Optimality verification: lowest-cost paths saturated first (Mumbai, then Bangalore, etc.).

---

## 🛠️ How to Compile & Run Individual Unit Tests

From the `DAA/` directory:

```cmd
:: Build and run Server Unit Tests
g++ -O2 -std=c++17 -Iinclude src/CDNServer.cpp tests/test_server.cpp -o bin/test_server.exe
bin\test_server.exe

:: Build and run MCMF Unit Tests
g++ -O2 -std=c++17 -Iinclude src/CDNServer.cpp src/Graph.cpp src/MinCostMaxFlow.cpp tests/test_mcmf.cpp -o bin/test_mcmf.exe
bin\test_mcmf.exe
```

---

## ⚡ Integrated 7-Scenario Verification Suite

For end-to-end integration and algorithmic stress-testing across dynamic workloads, run the main engine's test harness:

```cmd
bin\cdn_engine.exe --scenarios
```

This executes all 7 project test scenarios:
1. Normal Traffic Allocation
2. Traffic Spike Surge
3. Pune Queuing Congestion Adaptation
4. Mumbai Complete Node Failure (Failover)
5. Multiple Simultaneous Congestions
6. Total Fleet Overload (Safe Saturation & Unrouted Detection)
7. Fleet Recovery & Re-stabilization
