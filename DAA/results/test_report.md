# DAA Project Test Report: Min-Cost Max-Flow CDN Traffic Router

**Course**: Design and Analysis of Algorithms (DAA)
**Core Algorithm**: Successive Shortest Path (SSP) with Dijkstra and Johnson's Node Potentials
**Evaluation Date**: October 2026
**Engine Language**: C++17 (`DAA/src/`)

## 1. Executive Summary

- **Total Scenarios Evaluated**: 7
- **Scenarios Passed**: 7 / 7 (**100% Correctness**)
- **Network Invariants**: Flow conservation, capacity constraints, demand constraints, cost non-negativity, overload calculation, and failed-node isolation were verified automatically after every routing decision.

## 2. Experimental Scenario Results Table

| # | Scenario Name | Demand (req/s) | Total Cap | Routed | Unrouted | Cost (Rs.) | Active Nodes | Paths (K) | Time (us) | Status | Verdict |
|:---|:---|---:|---:|---:|---:|---:|---:|---:|---:|:---|:---|
| 1 | Normal Traffic | 8000 | 21500 | 8000 | 0 | 298000.00 | 4 | 2 | 4.00 | `OPTIMAL` | **PASS** |
| 2 | Traffic Spike | 16000 | 21500 | 16000 | 0 | 777750.00 | 4 | 3 | 4.36 | `CONGESTED_REROUTE` | **PASS** |
| 3 | Pune Congestion | 10000 | 21500 | 10000 | 0 | 426800.00 | 4 | 2 | 2.41 | `CONGESTED_REROUTE` | **PASS** |
| 4 | Mumbai Failure | 12000 | 15500 | 12000 | 0 | 787865.00 | 3 | 3 | 2.48 | `FAILOVER` | **PASS** |
| 5 | Multiple Congestion | 11000 | 21500 | 11000 | 0 | 454000.00 | 4 | 2 | 2.26 | `CONGESTED_REROUTE` | **PASS** |
| 6 | Fleet Overload | 26000 | 21500 | 21500 | 4500 | 2464860.00 | 4 | 4 | 2.60 | `INSUFFICIENT_CAPACITY` | **PASS** |
| 7 | Fleet Recovery | 10000 | 21500 | 10000 | 0 | 386000.00 | 4 | 2 | 2.35 | `OPTIMAL` | **PASS** |

## 3. Automated Invariant Verification Matrix

| # | Scenario | Flow Conservation | Capacity Bounds | Demand Bound | Cost Non-Negative | Overload Math | Failure Zero Flow | Feasibility |
|:---|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| 1 | Normal Traffic | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| 2 | Traffic Spike | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| 3 | Pune Congestion | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| 4 | Mumbai Failure | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| 5 | Multiple Congestion | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| 6 | Fleet Overload | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| 7 | Fleet Recovery | PASS | PASS | PASS | PASS | PASS | PASS | PASS |

## 4. Scenario-by-Scenario Evaluation

### Scenario 1: Normal Traffic
- **Objective**: Nominal conditions: MCMF routes via lowest-cost servers first
- **Demand / Capacity**: 8000 req/s / 21500 req/s
- **Routed / Unrouted**: 8000 req/s / 0 req/s
- **Routing Cost**: Rs. 298000.00
- **Augmenting Iterations (K)**: 2 iterations
- **Execution Time**: 4.00 microseconds
- **Outcome Summary**: MCMF prioritized lowest-cost nodes (Mumbai saturated at 6000, remainder 2000 to Bangalore). Zero drop.

### Scenario 2: Traffic Spike
- **Objective**: Surge from 8k to 16k req/s distributed across fleet without packet loss
- **Demand / Capacity**: 16000 req/s / 21500 req/s
- **Routed / Unrouted**: 16000 req/s / 0 req/s
- **Routing Cost**: Rs. 777750.00
- **Augmenting Iterations (K)**: 3 iterations
- **Execution Time**: 4.36 microseconds
- **Outcome Summary**: Traffic spike fully absorbed across Mumbai, Bangalore, and Pune. Zero packets dropped.

### Scenario 3: Pune Congestion
- **Objective**: Latency spike shifts edge cost, diverting traffic away from congested Pune node
- **Demand / Capacity**: 10000 req/s / 21500 req/s
- **Routed / Unrouted**: 10000 req/s / 0 req/s
- **Routing Cost**: Rs. 426800.00
- **Augmenting Iterations (K)**: 2 iterations
- **Execution Time**: 2.41 microseconds
- **Outcome Summary**: Pune's elevated latency increased its edge cost. MCMF routed 10k entirely through Mumbai (6k) & Bangalore (4k), completely bypassing Pune.

### Scenario 4: Mumbai Failure
- **Objective**: Node down: capacity clamped to 0, seamless failover to surviving fleet
- **Demand / Capacity**: 12000 req/s / 15500 req/s
- **Routed / Unrouted**: 12000 req/s / 0 req/s
- **Routing Cost**: Rs. 787865.00
- **Augmenting Iterations (K)**: 3 iterations
- **Execution Time**: 2.48 microseconds
- **Outcome Summary**: Failover successful: 0 traffic routed to Mumbai. 12,000 req/s redistributed across Pune, Bangalore, and Delhi.

### Scenario 5: Multiple Congestion
- **Objective**: Dual-node congestion: load shifted entirely to healthy servers (Mumbai & Bangalore)
- **Demand / Capacity**: 11000 req/s / 21500 req/s
- **Routed / Unrouted**: 11000 req/s / 0 req/s
- **Routing Cost**: Rs. 454000.00
- **Augmenting Iterations (K)**: 2 iterations
- **Execution Time**: 2.26 microseconds
- **Outcome Summary**: Adaptive router shielded congested nodes (Pune & Delhi), routing 100% of 11,000 req/s via healthy nodes (Mumbai & Bangalore).

### Scenario 6: Fleet Overload
- **Objective**: Demand exceeds fleet capacity: safe max flow saturation, exact unrouted detection
- **Demand / Capacity**: 26000 req/s / 21500 req/s
- **Routed / Unrouted**: 21500 req/s / 4500 req/s
- **Routing Cost**: Rs. 2464860.00
- **Augmenting Iterations (K)**: 4 iterations
- **Execution Time**: 2.60 microseconds
- **Outcome Summary**: Max flow correctly saturated fleet capacity at 21,500 req/s. Safely isolated 4,500 unrouted req/s without claiming impossible traffic.

### Scenario 7: Fleet Recovery
- **Objective**: Restoration of all nodes: optimal cost baseline re-established
- **Demand / Capacity**: 10000 req/s / 21500 req/s
- **Routed / Unrouted**: 10000 req/s / 0 req/s
- **Routing Cost**: Rs. 386000.00
- **Augmenting Iterations (K)**: 2 iterations
- **Execution Time**: 2.35 microseconds
- **Outcome Summary**: All servers fully recovered to HEALTHY. Optimal min-cost routing baseline re-established.

