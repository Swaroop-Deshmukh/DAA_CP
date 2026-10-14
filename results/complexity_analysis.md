# DAA Theoretical Complexity & Performance Analysis

**Project**: Adaptive CDN Traffic Router with Min-Cost Max-Flow
**Course**: Design and Analysis of Algorithms (DAA)
**Date**: October 2026
**Implementation**: C++17 (`DAA/src/Graph.cpp`, `MinCostMaxFlow.cpp`, `Router.cpp`)

---

## Phase 4: Random-Graph Scalability Benchmark Results

Empirical performance measured across seeded random networks (10 to 1,000 edge servers with proportional client regions):

| Servers | Regions | $|V|$ | $|E|$ | $K$ (Paths) | Dijkstra (us) | Bellman-Ford (us) | Primal-Dual (us) | BF / Dijk (x) | Theoretical $K \cdot |E| \log_2 |V|$ | Ratio (ns/op) |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| 10 | 5 | 18 | 142 | 9 | 17.2 | 11.6 | 19.7 | 0.67x | 5329 | **3.229** |
| 50 | 25 | 78 | 718 | 62 | 764.3 | 321.5 | 857.1 | 0.42x | 279801 | **2.732** |
| 100 | 50 | 153 | 1438 | 125 | 3606.4 | 1394.0 | 2418.5 | 0.39x | 1304515 | **2.765** |
| 500 | 250 | 753 | 7140 | 590 | 75659.0 | 39855.0 | 49919.0 | 0.53x | 40257737 | **1.879** |
| 1000 | 500 | 1503 | 14272 | 1229 | 525025.0 | 331964.0 | 149421.0 | 0.63x | 185113697 | **2.836** |

### Algorithmic Insights from Scalability Benchmark:

1. **Asymptotic Constant Ratio**: The measured-to-theoretical ratio $\frac{\text{Measured Time}}{K \cdot |E| \log_2 |V|}$ remains roughly constant across instances spanning two orders of magnitude ($S = 10$ to $S = 1,000$), empirically validating the $\mathcal{O}(K \cdot |E| \log |V|)$ complexity bound.
2. **Reference Solver Specification**: The reference solver uses Classic Bellman-Ford ($|V|-1$ passes with early termination). On shallow networks (hop depth 4), Bellman-Ford exits after 4 passes; on deeper adversarial topologies, Dijkstra's $\mathcal{O}(|E| \log |V|)$ is significantly faster.
3. **Primal-Dual Optimization**: Serving multiple augmenting paths per Dijkstra run via Dinic-style DFS blocking flow reduces Dijkstra invocations and yields substantial performance improvements.

