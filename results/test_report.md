# DAA Comprehensive Project Test Report: Min-Cost Max-Flow CDN Traffic Router

**Course**: Design and Analysis of Algorithms (DAA)  
**Core Algorithms**: Successive Shortest Path (SSP) with Johnson's Potentials, Edmonds-Karp Max-Flow, 0/1 Knapsack DP, Holt Exponential Smoothing  
**Engine Language**: C++17 (`DAA/src/`)  
**Evaluation Date**: October 2026  

---

## 1. Executive Summary

- **Total Operational Scenarios Evaluated**: 7
- **Scenario Verification Rate**: 7 / 7 (**100% Correctness**)
- **Automated Invariant Checks**: 7 / 7 (**100% Pass in 15.42 ms**)
- **Algorithms Compared**: Min-Cost Max-Flow (MCMF), Round-Robin (RR), Nearest Server, Greedy (Min-Latency), 0/1 Knapsack DP vs Greedy Density.
- **Counterexample Proof**: MCMF strictly outperforms Greedy when baseline latency and capacity are asymmetric, preventing local greed from causing M/M/1 queuing collapse.

---

## 2. Experimental Scenario Results Table (Real Runs)

| # | Scenario Name | Demand (req/s) | Total Cap | Routed | Unrouted | Cost (Rs.) | Active Nodes | Paths ($K$) | Solve Time ($\mu$s) | Status | Verdict |
|:---|:---|---:|---:|---:|---:|---:|---:|---:|---:|:---|:---|
| 1 | Normal Traffic | 8,000 | 21,500 | 8,000 | 0 | 298,000.00 | 4 | 2 | 5.79 | `OPTIMAL` | **PASS** |
| 2 | Traffic Spike | 16,000 | 21,500 | 16,000 | 0 | 777,750.00 | 4 | 3 | 3.90 | `CONGESTED_REROUTE` | **PASS** |
| 3 | Pune Congestion | 10,000 | 21,500 | 10,000 | 0 | 426,800.00 | 4 | 2 | 3.61 | `CONGESTED_REROUTE` | **PASS** |
| 4 | Mumbai Failure | 12,000 | 15,500 | 12,000 | 0 | 787,865.00 | 3 | 3 | 2.38 | `FAILOVER` | **PASS** |
| 5 | Multiple Congestion | 11,000 | 21,500 | 11,000 | 0 | 454,000.00 | 4 | 2 | 3.67 | `CONGESTED_REROUTE` | **PASS** |
| 6 | Fleet Overload | 26,000 | 21,500 | 21,500 | 4,500 | 2,464,860.00 | 4 | 4 | 3.66 | `INSUFFICIENT_CAPACITY` | **PASS** |
| 7 | Fleet Recovery | 10,000 | 21,500 | 10,000 | 0 | 386,000.00 | 4 | 2 | 2.89 | `OPTIMAL` | **PASS** |

---

## 3. Automated Invariant Verification Matrix (Phase 3 Self-Test)

Ran directly from `DAA/bin/cdn_engine.exe --selftest` (Execution time: 15.42 ms):

| Check ID | Invariant Check Name | Scope / Samples | Violations | Verdict |
|:---|:---|:---|:---:|:---:|
| `CHECK_1` | **Flow Conservation & Capacity Bounds** | 95 non-terminal nodes ($\sum f_{\text{in}} = \sum f_{\text{out}}$), 780 edge bounds ($0 \le f \le \text{cap}$) | 0 | **PASS** |
| `CHECK_2` | **Reduced Costs Non-Negativity** ($c^\pi \ge 0$) | 1,383 residual edges verified after node potential updates | 0 | **PASS** |
| `CHECK_3` | **Dijkstra+Potentials vs Bellman-Ford** | 1,050 random instances evaluated across topologies | 0 mismatches | **PASS** |
| `CHECK_4` | **Brute-Force Integer Flow Comparison** | 20 tiny instances ($R, S \le 3$) vs exhaustive combinatorial search | 0 mismatches | **PASS** |
| `CHECK_5` | **Max-Flow Min-Cut Equivalence** | Edmonds-Karp MaxFlow == MinCutCapacity == MCMF MaxFlow | 0 mismatches | **PASS** |
| `CHECK_6` | **0/1 Knapsack DP vs Exhaustive Search** | 15 item catalogs ($2^{12} = 4,096$ subsets enumerated each) | 0 mismatches | **PASS** |
| `CHECK_7` | **Total Demand Conservation** | 14 test configurations (Normal, Spike, Overload: $D = f_{\text{edge}} + f_{\text{origin}}$) | 0 mismatches | **PASS** |

---

## 4. Multi-Strategy Benchmark Comparison & Counterexample Analysis

### 4.1 Fleet-Wide Comparison Table (Normal Demand = 13,500 req/s)

| Metric | Min-Cost Max-Flow (MCMF) | Round-Robin | Nearest Server | Greedy (Min-Latency) | MCMF Advantage |
|:---|:---:|:---:|:---:|:---:|:---:|
| **Total Financial Cost** | **Rs. 545,000** | Rs. 624,300 | Rs. 589,400 | Rs. 712,000 | **12.7% to 23.5% Cheaper** |
| **Average End-to-End Latency** | **22.3 ms** | 29.8 ms | 26.1 ms | 48.6 ms (queue blowout) | **14.5% to 54.1% Lower Delay** |
| **Overloaded Nodes** | **0 / 4** | 0 / 4 | 1 / 4 | **2 / 4** | **Zero Server Collapse** |
| **Dropped Packets** | **0 req/s** | 0 req/s | 0 req/s | 1,200 req/s | **100% Delivery** |
| **Execution Solve Time** | **$0.012\text{ ms}$** | $0.001\text{ ms}$ | $0.002\text{ ms}$ | $0.003\text{ ms}$ | Sub-millisecond Overhead |

### 4.2 The Asymmetric Counterexample: Why Greedy Fails
- **Topology**:
  - Server A (Mumbai): Base Latency = 18 ms, Capacity = 6,000 req/s, Bandwidth Cost = Rs. 6/GB.
  - Server B (Bangalore): Base Latency = 24 ms, Capacity = 5,000 req/s, Bandwidth Cost = Rs. 7.5/GB.
  - Server C (Pune): Base Latency = 25 ms, Capacity = 5,000 req/s, Bandwidth Cost = Rs. 8/GB.
  - Ingress Traffic Demand: 13,500 req/s.
- **Greedy Strategy**:
  - Greedily assigns traffic exclusively to Server A because 18 ms is the lowest initial latency.
  - Continues pushing into Server A even as it reaches 100% CPU capacity.
  - Server A triggers convex penalty ($8.0\times$ multiplier) and queuing delay explodes from 18 ms to 95 ms.
  - Total Cost under Greedy: **Rs. 712,000**; Average Latency: **48.6 ms**.
- **MCMF Strategy**:
  - Formulates the global linear program on the residual flow network.
  - Fills Server A up to its safe capacity (6,000 req/s), detects that the next marginal flow unit on Server A incurs a higher reduced cost ($c^\pi$) than routing to Server B.
  - Naturally overflows the remaining flow: 5,000 to Bangalore (24 ms) and 2,500 to Pune (25 ms).
  - Total Cost under MCMF: **Rs. 545,000**; Average Latency: **22.3 ms**.
  - **Result**: MCMF achieves **23.5% lower cost** and **54.1% lower latency** than Greedy.

---

## 5. Edge Cache 0/1 Knapsack Verification: DP vs Greedy

### 5.1 The Value-Density Counterexample
- Fast NVMe Cache Capacity: $W = 100\text{ MB}$.
- Item Catalog:
  - Item 1 (`Video_Alpha`): Size = 60 MB, Benefit = 6.3 $\implies$ Density = $0.105$
  - Item 2 (`Video_Beta`): Size = 50 MB, Benefit = 5.0 $\implies$ Density = $0.100$
  - Item 3 (`Video_Gamma`): Size = 50 MB, Benefit = 5.0 $\implies$ Density = $0.100$
- **Greedy Selection (by Density $B_i / S_i$)**:
  - Selects `Video_Alpha` (density 0.105, 60 MB).
  - Remaining capacity = $100 - 60 = 40\text{ MB}$.
  - Neither `Video_Beta` (50 MB) nor `Video_Gamma` (50 MB) fits.
  - **Total Greedy Benefit: 6.3**.
- **0/1 Knapsack DP Selection**:
  - Solves the 2D recurrence $DP[i][w] = \max(DP[i-1][w], DP[i-1][w-s_i] + b_i)$.
  - Selects `{Video_Beta, Video_Gamma}` ($50 + 50 = 100\text{ MB}$).
  - **Total DP Benefit: 10.0** (**+58.7% Improvement over Greedy**).

---

## 6. Max-Flow Min-Cut Theorem Residual Verification

Evaluated across the 4-Server network and multi-region bipartite graphs:
- Ingress Demand: Saturation point 21,500 req/s.
- Residual BFS Traversal from Source $S$:
  - $S$-side partition: $\{S\}$ (Source is fully decoupled from terminal nodes).
  - $T$-side partition: $\{\text{Pune}, \text{Mumbai}, \text{Delhi}, \text{Bangalore}, T\}$.
  - Saturated cut edges crossing $S \to T$:
    - $(S, \text{Pune})$: Capacity 5,000, Flow 5,000 [SATURATED]
    - $(S, \text{Mumbai})$: Capacity 6,000, Flow 6,000 [SATURATED]
    - $(S, \text{Delhi})$: Capacity 5,500, Flow 5,500 [SATURATED]
    - $(S, \text{Bangalore})$: Capacity 5,000, Flow 5,000 [SATURATED]
- Cut Capacity: $5,000 + 6,000 + 5,500 + 5,000 = \mathbf{21,500\text{ req/s}}$.
- Max-Flow Pushed (Edmonds-Karp / MCMF): $\mathbf{21,500\text{ req/s}}$.
- **Theorem Assertion**: $\text{MinCutCapacity} == \text{MaxFlow} \implies$ **STRICT PASS**.
