# Adaptive CDN Traffic Router with Min-Cost Max-Flow (DAA Implementation)

> **Course**: Design and Analysis of Algorithms (DAA)  
> **Topic**: Directed Flow Networks, Min-Cost Max-Flow (MCMF), Johnson's Node Potentials, and Adaptive Traffic Optimization  
> **Language**: 100% Modern C++ (C++17) using C++ Standard Library (`<vector>`, `<queue>`, `<chrono>`, `<map>`)  
> **External Libraries**: **None** (All graph representations, shortest path routines, potentials, and flow augmentation algorithms are implemented manually from first principles).

---

## 1. Project Title
**Adaptive CDN Traffic Router with Min-Cost Max-Flow**

---

## 2. Problem Statement
In distributed Content Delivery Networks (CDNs), client request demand fluctuates dynamically due to diurnal patterns, flash crowds, hardware failures, and network congestion. 

Given:
1. A dynamic global request demand $F$ (in requests per second, $\text{req/s}$).
2. A fleet of geographically distributed CDN edge servers (Pune, Mumbai, Delhi, Bangalore), each with:
   - Finite processing capacity $C_i$ ($\text{req/s}$)
   - Dynamic round-trip latency $L_i$ ($\text{ms}$)
   - Regional egress bandwidth transit cost $B_i$ ($₹/\text{GB}$)
   - Health status: `HEALTHY`, `CONGESTED`, or `FAILED`

**Optimization Goal**:
Determine the optimal traffic flow assignment $f_i$ across all edge servers $i \in \{1, 2, 3, 4\}$ such that:
1. **Flow Conservation & Demand Maximization**: $\sum_{i} f_i = \min\left(F, \sum_i C_i\right)$
2. **Capacity Constraints**: $0 \le f_i \le C_i$ for all servers $i$
3. **Cost Minimization**: The global multi-objective routing cost $\sum_i f_i \cdot (\alpha L_i + \beta B_i)$ is strictly minimized.
4. **Adaptive Reaction**: When server health or load changes, the system must automatically recalculate allocations without manual intervention.

---

## 3. Objective
- Model the CDN routing problem as a **Directed Flow Network** with residual capacities.
- Formulate the multi-objective cost function combining latency and monetary bandwidth cost.
- Implement the **Successive Shortest Path (SSP)** Min-Cost Max-Flow algorithm from scratch.
- Address the challenge of negative residual edge costs using **Dijkstra's Algorithm with Johnson's Node Potentials / Reduced Costs**.
- Implement closed-loop **Adaptive Rerouting** that detects fleet condition drift and reroutes traffic away from congested/failed nodes.
- Empirically benchmark execution times using `<chrono>` across varying traffic scales ($1\text{k} - 50\text{k}\text{ req/s}$) and verify algorithmic bounds.

---

## 4. Graph Model

We formulate the system as a directed flow network $G = (V, E)$ with:
- **Vertices** $V = \{s, v_1, v_2, v_3, v_4, t\}$ where $|V| = 6$:
  - $s = 0$: Global Traffic Ingress Source
  - $v_1 = 1$: Pune Edge Server
  - $v_2 = 2$: Mumbai Edge Server
  - $v_3 = 3$: Delhi Edge Server
  - $v_4 = 4$: Bangalore Edge Server
  - $t = 5$: Origin Data Center Sink

```
             [ User Ingress Traffic: F req/s ]
                             │
                             ▼
                     ┌───────────────┐
                     │   Source (0)  │
                     └───────┬───────┘
                             │
         ┌───────────────────┼───────────────────┬───────────────────┐
         │ (c1, w1)          │ (c2, w2)          │ (c3, w3)          │ (c4, w4)
         ▼                   ▼                   ▼                   ▼
   ┌───────────┐       ┌───────────┐       ┌───────────┐       ┌───────────┐
   │ Pune (1)  │       │Mumbai (2) │       │ Delhi (3) │       │Bangalore(4)│
   └─────┬─────┘       └─────┬─────┘       └─────┬─────┘       └─────┬─────┘
         │ (c1, 0)           │ (c2, 0)           │ (c3, 0)           │ (c4, 0)
         └───────────────────┼───────────────────┴───────────────────┘
                             │
                             ▼
                     ┌───────────────┐
                     │   Sink (5)    │
                     └───────────────┘
```

### Directed Edges and Data Structure:
Each directed edge in the adjacency list stores:
```cpp
struct Edge {
    int to;             // Destination vertex
    int reverseIndex;   // Index of reverse residual edge in adj[to] for O(1) residual lookup
    int capacity;       // Maximum flow capacity (req/s)
    int cost;           // Scaled integer routing cost per unit of flow
    int flow;           // Current routed flow
    int residualCapacity() const { return capacity - flow; }
};
```

### Configurable Scaled Cost Function:
To guarantee **exact integer arithmetic** and eliminate floating-point precision error, cycling, or non-termination in Dijkstra:
$$\text{Cost}_{\text{scaled}} = \text{round}\left(100 \times \left(\alpha \cdot \text{Latency} + \beta \cdot \text{BandwidthCost}\right)\right)$$
- $\alpha = 1.0$: Latency weight
- $\beta = 2.0$: Bandwidth cost weight
- Fixed-point scale factor: $S = 100$ ($₹1.00 = 100\text{ cost units}$)

---

## 5. DAA Algorithms Used

| Algorithm | Role in Project | Time Complexity |
|---|---|---|
| **Adjacency List Residual Graph** | Explicit forward and reverse edges with capacity and cost | Space: $\mathcal{O}(|V| + |E|)$ |
| **Dijkstra's Algorithm** | Shortest augmenting path finder using binary min-heap | $\mathcal{O}(|E| \log |V|)$ |
| **Johnson's Node Potentials** | Guarantees non-negative reduced costs on residual edges | $\mathcal{O}(1)$ update per node |
| **Successive Shortest Path (SSP)** | Solves Min-Cost Max-Flow by iterative augmentation | $\mathcal{O}(K \cdot |E| \log |V|)$ |
| **Bottleneck Capacity Push** | Augments maximum possible flow along the shortest path | $\mathcal{O}(|V|)$ per path |

---

## 6. Min-Cost Max-Flow Explanation

The Min-Cost Max-Flow problem is solved using the **Successive Shortest Path (SSP)** framework:
1. **Initialize**: Flow $f(u, v) = 0$ for all edges. Potentials $\pi(v) = 0$ for all nodes.
2. **Find Shortest Augmenting Path**: Run Dijkstra on the residual network using reduced costs $c^\pi(u, v)$ from Source ($s=0$) to Sink ($t=5$).
3. **Check Reachability**: If no path with residual capacity exists, the maximum possible flow has been reached. Terminate.
4. **Identify Bottleneck Capacity**:
   $$\Delta = \min_{(u, v) \in P} \left(c(u, v) - f(u, v)\right)$$
   Also limit $\Delta$ by remaining unmet demand: $\Delta = \min(\Delta, \text{remainingDemand})$.
5. **Augment Flow**: For each edge $(u, v)$ on path $P$:
   $$f(u, v) \leftarrow f(u, v) + \Delta, \quad f(v, u) \leftarrow f(v, u) - \Delta$$
6. **Update Potentials**: Set $\pi(v) \leftarrow \pi(v) + \text{dist}(v)$ for all reachable nodes.
7. **Repeat** until required demand is satisfied or network is saturated.

---

## 7. Dijkstra + Johnson's Potentials (Reduced Costs)

### The Negative Edge Cost Problem in Residual Networks
When flow is pushed along a forward edge $(u, v)$ with cost $c(u, v) > 0$, an anti-symmetric reverse edge $(v, u)$ is created in the residual graph with cost:
$$c(v, u) = -c(u, v) < 0$$
Standard Dijkstra's algorithm fails when negative-weight edges exist, because the greedy invariant (visited vertices have permanent shortest distances) no longer holds. While Bellman-Ford can handle negative costs, it requires $\mathcal{O}(|V| \cdot |E|)$ time per augmentation, which is inefficient.

### The Solution: Johnson's Node Potentials
We assign each vertex $v$ a potential $\pi(v)$ and define the **reduced cost** of an edge $(u, v)$ as:
$$c^\pi(u, v) = c(u, v) + \pi(u) - \pi(v)$$

**Mathematical Proof of Non-Negativity**:
- In the initial graph, all forward edge costs are non-negative ($c(u, v) \ge 0$), and all $\pi(v) = 0$, so $c^\pi(u, v) = c(u, v) \ge 0$.
- In subsequent iterations, if $\text{dist}(v)$ represents the shortest distance from $s$ to $v$ under reduced costs $c^\pi$, then by the triangle inequality:
  $$\text{dist}(v) \le \text{dist}(u) + c^\pi(u, v)$$
  $$\text{dist}(v) - \text{dist}(u) \le c(u, v) + \pi(u) - \pi(v)$$
  $$0 \le c(u, v) + (\pi(u) + \text{dist}(u)) - (\pi(v) + \text{dist}(v))$$
- Updating potentials as $\pi_{\text{new}}(v) = \pi(v) + \text{dist}(v)$ guarantees that:
  $$c^{\pi_{\text{new}}}(u, v) \ge 0 \quad \forall (u, v) \in G_f$$
- Furthermore, for any path $P$ from $s$ to $t$:
  $$\sum_{(u, v) \in P} c^\pi(u, v) = \sum_{(u, v) \in P} c(u, v) + \pi(s) - \pi(t)$$
  Since $\pi(s) - \pi(t)$ is constant for all $s$-$t$ paths, **a shortest path under reduced costs is strictly identical to the shortest path under original costs**.
- Therefore, Dijkstra with a binary min-heap (`std::priority_queue`) is provably correct and executes in $\mathcal{O}(|E| \log |V|)$ time.

---

## 8. Residual Graph Implementation

The residual network $G_f$ is maintained through reverse edge indices:
```cpp
// Adding a forward edge and corresponding anti-symmetric reverse residual edge
void Graph::addEdge(int from, int to, int capacity, int cost) {
    Edge forward{to, (int)adj[to].size(), capacity, cost, 0};
    Edge reverse{from, (int)adj[from].size(), 0, -cost, 0};
    adj[from].push_back(forward);
    adj[to].push_back(reverse);
}
```
- Forward edge has capacity $C$ and cost $+W$.
- Reverse residual edge has initial capacity $0$ and cost $-W$.
- In residual capacity queries: `residualCapacity() = capacity - flow`.
- Reverse index lookup is $\mathcal{O}(1)$.

---

## 9. Adaptive Routing Loop

The router operates in a closed feedback loop:
```
CDN Telemetry (Telemetry Simulator)
              │
              ▼
   State Change Detection (Debounce & Threshold Check)
              │
              ├─ No significant change? ──► Reuse Cached Routing Decision
              │
              ▼ (Significant change detected)
   Rebuild Flow Network with Updated Node Capacities & Latencies
              │
              ▼
   Run Min-Cost Max-Flow Engine (SSP + Dijkstra with Potentials)
              │
              ▼
   Generate Routing Decision Table + Diff vs Previous Allocation
              │
              ▼
   Export Canonical JSON (routing_decision.json) for Dashboard
```

State changes triggering recomputation:
1. Any server status transition (`HEALTHY` $\leftrightarrow$ `CONGESTED` $\leftrightarrow$ `FAILED`)
2. Latency shifts exceeding $15\text{ ms}$
3. Global demand shifts exceeding $500\text{ req/s}$

---

## 10. Complexity Analysis

### Topology-Specific Augmenting Path Bound:
In general flow networks, Successive Shortest Path with integer capacities has a pseudo-polynomial bound of $\mathcal{O}(F \cdot |E| \log |V|)$ in the worst-case if augmenting paths push unit flows.

**Crucial Invariant for this CDN Topology**:
- In our CDN architecture, client requests flow from Source ($s$) directly through one of the 4 Edge Servers to the Sink ($t$).
- Each augmenting path $P = (s \to v_i \to t)$ pushes the bottleneck residual capacity:
  $$B = \min\left(r(s, v_i), r(v_i, t), \text{remainingDemand}\right)$$
- Because capacities are large ($C_i \ge 5000$), each path pushes the entire capacity of server $v_i$ (or satisfies the entire remaining demand).
- Therefore, for this specific topology, the number of augmentations $K$ is strictly bounded by the number of edge servers:
  $$K \le |\text{CDN Edge Servers}| = 4$$

### Asymptotic Bounds for this Implementation:
- **Time Complexity per Augmentation**: $\mathcal{O}(|E| \log |V|)$ via Dijkstra with binary heap.
- **Total Time Complexity**: 
  $$\mathcal{O}(K \cdot |E| \log |V|) \quad \text{where } K \le 4$$
- Because $K \le 4$, $|V| = 6$, and $|E| = 18$, the algorithm executes in a small constant number of operations $\mathcal{O}(1)$ relative to total traffic volume $F$.
- **Space Complexity**:
  $$\mathcal{O}(|V| + |E|) \quad (\text{Adjacency list + potential vectors} < 10\text{ KB})$$

*(Note: We do not claim this as a universal bound for arbitrary multi-hop flow networks, but as an exact invariant of this CDN topology).*

---

## 11. Test Scenarios (Verification Suite)

All 7 test scenarios pass deterministically with assertions:

```
========================================================================================
                         7-SCENARIO SUITE VERIFICATION REPORT
========================================================================================
 #    | Scenario Name          |   Demand |   Routed | Unrouted | Status               | Verdict  |
----------------------------------------------------------------------------------------
 1    | Normal Traffic         |     8000 |     8000 |        0 | OPTIMAL              | PASS     |
 2    | Traffic Spike          |    16000 |    16000 |        0 | CONGESTED_REROUTE    | PASS     |
 3    | Pune Congestion        |    10000 |    10000 |        0 | CONGESTED_REROUTE    | PASS     |
 4    | Mumbai Failure         |    12000 |    12000 |        0 | FAILOVER             | PASS     |
 5    | Multiple Congestion    |    11000 |    11000 |        0 | CONGESTED_REROUTE    | PASS     |
 6    | Total Fleet Overload   |    26000 |    21500 |     4500 | INSUFFICIENT_CAPACITY | PASS     |
 7    | Fleet Recovery         |    10000 |    10000 |        0 | OPTIMAL              | PASS     |
========================================================================================
 FINAL VERDICT: 7/7 SCENARIOS PASSED.
========================================================================================
```

1. **Scenario 1 (Normal Traffic)**: Saturated lowest-cost server (Mumbai at $6,000\text{ req/s}$) and placed remainder in Bangalore ($2,000\text{ req/s}$).
2. **Scenario 2 (Traffic Spike)**: Surged demand to $16,000\text{ req/s}$; absorbed across all 4 servers without drops.
3. **Scenario 3 (Pune Congestion)**: Pune latency increased to $120\text{ ms}$; cost escalated; router shifted $100\%$ of traffic to Mumbai and Bangalore.
4. **Scenario 4 (Mumbai Failure)**: Mumbai capacity clamped to $0$; router triggered failover to Pune, Delhi, and Bangalore ($0$ traffic to Mumbai).
5. **Scenario 5 (Multiple Congestion)**: Simultaneous congestion in Pune and Delhi; router pushed traffic through healthy nodes (Mumbai & Bangalore).
6. **Scenario 6 (Fleet Overload)**: Demand of $26,000\text{ req/s}$ exceeded fleet limit ($21,500$); max flow saturated cleanly at $21,500$, safely isolating $4,500\text{ unrouted req/s}$.
7. **Scenario 7 (Fleet Recovery)**: Restored all nodes to healthy nominal baseline; global cost re-stabilized.

---

## 12. Benchmark Results

Measured using `std::chrono::high_resolution_clock` across 200 repetitions per demand tier:

| Demand (req/s) | Routed (req/s) | Cost (₹) | Augmenting Paths ($K$) | Graph Build ($\mu$s) | Dijkstra ($\mu$s) | MCMF ($\mu$s) | MCMF (ms) |
|---|---|---|---|---|---|---|---|
| **1,000** | 1,000 | ₹35,000.00 | 1 | 2.54 $\mu$s | < 1 $\mu$s | 2.92 $\mu$s | **0.0029 ms** |
| **5,000** | 5,000 | ₹175,000.00 | 1 | 3.17 $\mu$s | < 1 $\mu$s | < 1 $\mu$s | **0.0000 ms** |
| **10,000** | 10,000 | ₹386,000.00 | 2 | 2.96 $\mu$s | < 1 $\mu$s | < 1 $\mu$s | **0.0000 ms** |
| **20,000** | 20,000 | ₹880,000.00 | 4 | 2.65 $\mu$s | 0.72 $\mu$s | 2.88 $\mu$s | **0.0029 ms** |
| **50,000** | 21,500 | ₹962,500.00 | 4 | 2.87 $\mu$s | < 1 $\mu$s | < 1 $\mu$s | **0.0000 ms** |

**Observation**: Across all demand levels from $1,000$ to $50,000\text{ req/s}$, the number of augmenting paths pushed never exceeds $4$, confirming our theoretical bound.

---

## 13. How to Build

From within the `DAA/` directory:

```cmd
build.bat
```

Or using manual compilation with GCC MinGW (C++17):

```cmd
g++ -O3 -std=c++17 -Wall -Wextra -Iinclude src/*.cpp -o bin\cdn_engine.exe
```

---

## 14. How to Run

From within the `DAA/` directory:

```cmd
:: Run full demonstration (all 7 scenarios + benchmarks)
bin\cdn_engine.exe --demo

:: Run automated 7-point DAA invariant self-test suite
bin\cdn_engine.exe --selftest

:: Run only the 7 verification scenarios
bin\cdn_engine.exe --scenarios

:: Run only the performance benchmarking suite
bin\cdn_engine.exe --bench

:: Run algorithm trace mode on sample topology
bin\cdn_engine.exe --trace

:: Run the continuous adaptive simulation loop
bin\cdn_engine.exe --loop

:: Run persistent stdio JSON-IPC daemon for dashboard integration
bin\cdn_engine.exe --ipc
```

