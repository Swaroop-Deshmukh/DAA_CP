# 5-Minute Evaluation & Viva Demo Script
## Adaptive CDN Traffic Router — DAA Min-Cost Max-Flow Engine

> **Target Audience**: Evaluators, Professors, and System Engineers during a viva or project demonstration.  
> **Total Duration**: 5 Minutes (300 seconds)  
> **Pre-demo Setup**:
> 1. Launch FastAPI backend: `python -m uvicorn integration.dashboard.web_app:app --host 127.0.0.1 --port 8000`
> 2. Open browser: Navigate to `http://127.0.0.1:8000/`
> 3. Verify server status badge displays green **"ENGINE ONLINE (IPC)"**.

---

## Demo Schedule & Panel Flow

```
[00:00 - 00:45] ➔ Panel 1: Network Topology Graph (MCMF Formulation)
[00:45 - 01:30] ➔ Panel 2: Strategy Toggle & Comparison (MCMF vs Greedy vs Round-Robin)
[01:30 - 02:15] ➔ Panel 4: Algorithm Trace Stepper (SSP with Johnson's Potentials)
[02:15 - 03:00] ➔ Panel 3: Min-Cut / Max-Flow Bottleneck Overlay (Max-Flow Min-Cut Theorem)
[03:00 - 03:45] ➔ Panel 5: Live Controls & Stress Testing (Fault Injection & Hysteresis)
[03:45 - 04:15] ➔ Panel 6: Predictive Routing (Holt's Linear Exponential Smoothing)
[04:15 - 04:40] ➔ Panel 7: Edge Cache View (0/1 Knapsack DP vs Greedy Counterexample)
[04:40 - 05:00] ➔ Panel 8, 9 & 10: Scalability Benchmark, Explainability & Data Export
```

---

### Minute 0:00 – 0:45 | Panel 1: Network Topology Graph
**Action**: Click the **"Network Graph"** tab (Panel 1).

* **What you say**:
  > *"Welcome. This project solves dynamic traffic routing across multi-region Content Delivery Networks using Design and Analysis of Algorithms (DAA) principles. Rather than relying on naive heuristic load balancing, our core solver transforms CDN routing into a **Min-Cost Max-Flow (MCMF)** network optimization problem."*
* **What you point out on screen**:
  - Point to the **Virtual Super-Source (S)** supplying total ingress client demand.
  - Point to the **Virtual Super-Sink (T)** terminating delivered responses.
  - Highlight the 4 edge PoP nodes: **Mumbai (West Coastal)**, **Pune (West)**, **Bangalore (South)**, and **Delhi (North)**.
  - Highlight the live SVG link pipes:
    - Pipe line thickness represents **link capacity** ($u_{ij}$).
    - Gradient animation speed represents **live routed traffic flow** ($f_{ij}$).
    - Color indicates link utilization: Green ($<70\%$), Amber ($70\%-85\%$), Red ($>85\%$).
* **DAA Concept**: Flow conservation constraints ($\sum f_{in} = \sum f_{out}$), capacity constraints ($0 \le f_{ij} \le u_{ij}$), and convex cost functions modeling M/M/1 queuing delay.

---

### Minute 0:45 – 1:30 | Panel 2: Strategy Toggle & Live Comparison
**Action**: Click the **"Strategy Comparison"** tab (Panel 2). Click the toggle buttons: **"Round-Robin"**, **"Greedy (Min-Latency)"**, and then return to **"MCMF (Optimal)"**.

* **What you say**:
  > *"Here we benchmark our MCMF algorithm directly against standard production baselines: Round-Robin and Greedy Min-Latency."*
* **What you point out on screen**:
  - Look at the live metric comparison table and side-by-side bar charts.
  - **Round-Robin**: Blindly splits requests equally across all PoPs regardless of link distance. Result: High average latency (~29.8 ms) and higher bandwidth cost.
  - **Greedy Min-Latency**: Funnels all traffic to the lowest baseline latency server (Mumbai @ 18ms). Notice what happens: Mumbai's capacity is instantly overwhelmed, hitting 100% CPU and triggering steep queuing delays.
  - **MCMF (Optimal)**: Solves the global cost objective:
    $$\min \sum_{(u,v) \in E} c(u,v) \cdot f(u,v)$$
    It saturates Mumbai up to its safe convex inflection point, then overflows the marginal requests onto Bangalore (24ms) and Pune (25ms), maintaining zero dropped packets and a 20–35% overall cost saving.

---

### Minute 1:30 – 2:15 | Panel 4: Algorithm Trace Stepper
**Action**: Click the **"Algorithm Trace"** tab (Panel 4). Click **"Step Forward ▶"** or **"▶ Auto Play"**.

* **What you say**:
  > *"To ensure academic transparency and full theoretical rigor, the C++ engine generates an execution trace of every step in the **Successive Shortest Path (SSP)** algorithm."*
* **What you point out on screen**:
  - **Johnson's Node Potentials**: Show the potential table $\pi(u)$. Point out that by computing reduced costs:
    $$c^\pi(u,v) = c(u,v) + \pi(u) - \pi(v) \ge 0$$
    every edge in the residual graph has non-negative reduced cost.
  - **Dijkstra vs Bellman-Ford**: Because all reduced costs are non-negative, the solver can run **Dijkstra with Fibonacci/Binary Heaps in $O(E \log V)$** per augmentation step instead of slow Bellman-Ford $O(V \cdot E)$.
  - **Step-by-step execution**:
    - Step 1: Initial potential assignment via Bellman-Ford / zero potentials.
    - Step 2: Dijkstra extracts shortest augmenting path (e.g. $S \to \text{Mumbai} \to T$, distance $d = 30.0$).
    - Step 3: Augment bottleneck flow $\Delta = 6000$ req/s.
    - Step 4: Residual graph update — create backward edge with cost $-c(u,v)$ and update node potentials $\pi(u) \leftarrow \pi(u) + d(u)$.
    - Step 5: Next shortest path diverted to Bangalore/Pune.
* **DAA Concept**: Theorem of Optimality — A flow is min-cost if and only if its residual graph contains no negative-cost directed cycles.

---

### Minute 2:15 – 3:00 | Panel 3: Min-Cut / Max-Flow Bottleneck Overlay
**Action**: Click the **"Min-Cut Overlay"** tab (Panel 3).

* **What you say**:
  > *"Here we visualize the fundamental **Ford-Fulkerson Max-Flow Min-Cut Theorem** on the live topology."*
* **What you point out on screen**:
  - Point out the **S-Partition (Reachable from Source)** highlighted in cyan and the **T-Partition (Reachable to Sink)** highlighted in purple.
  - Notice the **Red Dashed Cut Boundary Line** dividing the network.
  - Point to the **Bottleneck Edges Crossing the Cut**:
    - Capacity of the minimum cut equals exactly the maximum sustainable flow of the network ($C(S,T) = \sum_{u \in S, v \in T} u_{uv} = 21,500$ req/s).
    - As traffic approaches network saturation, the bottleneck edges are highlighted in glowing crimson with their residual capacity reading $r(u,v) = 0$.
* **DAA Concept**: Max-Flow Min-Cut Theorem: $\max |f| = \min C(S, T)$. Provides formal mathematical proof that no algorithm can push additional traffic without dropping requests once this cut is saturated.

---

### Minute 3:00 – 3:45 | Panel 5: Interactive Controls & Stress Testing
**Action**: Click the **"Controls & Stress"** tab (Panel 5).

* **What you do**:
  1. Drag the **Demand Slider** from 13,500 up to **18,000 req/s**. Click **"Apply Demand"**.
  2. Click **"Fail"** on **Mumbai**.
  3. Observe the immediate re-routing in real-time.
  4. Toggle the **Hysteresis Threshold** switch ON.
* **What you say**:
  > *"Let's put the system under live stress. I am increasing ingress client demand to 18,000 req/s, and simulating an abrupt hardware outage on Mumbai PoP."*
* **What you point out on screen**:
  - In less than **50 microseconds**, the C++ engine redistributes the 6,000 req/s loss to Bangalore, Pune, and Delhi.
  - **Hysteresis Filter**: Explain why naive routers suffer from route flapping / flip-flop thrashing under minor traffic jitter. Our engine employs a dual-threshold hysteresis band ($\tau_{\text{enter}} = 85\%, \tau_{\text{exit}} = 70\%$) with a minimum delta threshold ($\pm 5\%$), preventing expensive DNS cache thrashing.
  - Click **"Recover"** on Mumbai to restore healthy fleet state.

---

### Minute 3:45 – 4:15 | Panel 6: Predictive Traffic Forecasting
**Action**: Click the **"Forecast & Proactive"** tab (Panel 6).

* **What you say**:
  > *"Reactive load balancing only responds AFTER a server is already congested. To prevent latency spikes before they occur, our engine incorporates **Holt's Two-Parameter Linear Exponential Smoothing**."*
* **What you point out on screen**:
  - Show the forecast chart tracking historical demand, current level, and predicted trajectory ($k=1$ step ahead).
  - Formulas used:
    $$L_t = \alpha Y_t + (1-\alpha)(L_{t-1} + T_{t-1}) \quad (\alpha = 0.40)$$
    $$T_t = \beta (L_t - L_{t-1}) + (1-\beta) T_{t-1} \quad (\beta = 0.30)$$
    $$\hat{Y}_{t+k} = L_t + k \cdot T_t$$
  - Point to the **Proactive Reroute Trigger**: At Tick 5, the trend slope $T_t$ exceeds the proactive threshold (+100 req/s/tick). The engine proactively pre-allocates headroom 1 step before the flash crowd peaks.
  - Compare the peak latency: Proactive routing limits peak response time to **38.4 ms**, whereas reactive routing spikes to **94.2 ms** (+59% delay reduction).

---

### Minute 4:15 – 4:40 | Panel 7: Edge Cache View (0/1 Knapsack DP)
**Action**: Click the **"Cache Knapsack"** tab (Panel 7).

* **What you say**:
  > *"Edge servers have bounded fast-tier NVMe cache storage (100 MB). Deciding which high-demand video chunks to cache is a classic **0/1 Knapsack Problem**."*
* **What you point out on screen**:
  - Point to the comparison card: **0/1 Knapsack DP (Optimal)** vs **Greedy by Value Density ($B_i / S_i$)**.
  - Show the **Counterexample Item Set**:
    - Item 1: `Video_Alpha` (Size: 60 MB, Benefit: 6.3, Density: 0.105)
    - Item 2: `Video_Beta` (Size: 50 MB, Benefit: 5.0, Density: 0.100)
    - Item 3: `Video_Gamma` (Size: 50 MB, Benefit: 5.0, Density: 0.100)
  - Explain the outcome:
    - **Greedy** greedily takes `Video_Alpha` (highest density 0.105), consuming 60 MB. Remaining capacity is 40 MB, so neither Beta nor Gamma fits. Total Benefit = **6.3**.
    - **Dynamic Programming** solves the recurrence $DP[i][w] = \max(DP[i-1][w], DP[i-1][w-s_i] + b_i)$ and selects `{Video_Beta, Video_Gamma}` ($50 + 50 = 100$ MB). Total Benefit = **10.0** (+58.7% gain).
  - Show the interactive 2D DP matrix highlighting the optimal backtrack path.

---

### Minute 4:40 – 5:00 | Panels 8, 9 & 10: Scalability, Explainability & Export
**Action**: Quickly showcase **"Scalability"** (Panel 8), **"Explainability"** (Panel 9), and **"Data Export"** (Panel 10).

* **What you say**:
  > *"Finally, we demonstrate enterprise-scale readiness, explainability, and full data reproducibility."*
* **What you point out on screen**:
  - **Panel 8 (Scalability)**:
    - Asymptotic upper bound: $\mathcal{O}(K \cdot |E| \log |V|)$.
    - Empirical benchmark log-log chart from $N=10$ to $N=1,000$ servers.
    - Constant execution ratio:
      $$\frac{T(N)}{K \cdot E \log V} \approx 4.06 \text{ ns/operation}$$
      confirming the empirical runtime matches theoretical asymptotic complexity.
  - **Panel 9 ("Why this decision?")**:
    - Real-time diagnostic tree explaining the mathematical rationale: *"Why was Mumbai limited to 6,000 req/s? Tier 2 convex cost multiplier ($1.5\times$) would trigger if Mumbai exceeded 6,000, making Bangalore the lower-cost marginal path."*
  - **Panel 10 (CSV/JSON Export)**:
    - Click **"Export Allocations CSV"** or **"Export Full JSON"** to demonstrate one-click audit and reporting capability.

* **Closing Sentence (5:00)**:
  > *"To summarize: this system integrates Min-Cost Max-Flow routing with Johnson's potentials, Max-Flow Min-Cut bottleneck proofs, Holt predictive smoothing, and 0/1 Knapsack caching into a single unified C++ architecture running at sub-millisecond speeds. Thank you, and I am happy to answer any questions."*

---

## Quick Reference: Answers to Common Viva Questions

| Question | Theoretical Answer | System Implementation |
|---|---|---|
| **Why not just use Bellman-Ford?** | Bellman-Ford is $\mathcal{O}(V \cdot E)$. On a 100-node graph with 500 edges, that's 50,000 operations per augmentation. | We compute node potentials $\pi(u)$ once and run Dijkstra with Fibonacci/binary heap in $\mathcal{O}(E \log V)$, which is $500 \log_2(100) \approx 3,320$ operations (~15× faster). |
| **Why does Greedy fail for CDN traffic?** | Greedy evaluates the *marginal* cost locally without accounting for link saturation and queuing delays. | Convex cost multipliers ($1.0\times, 1.5\times, 3.0\times, 8.0\times$) penalize overloaded nodes, steering traffic globally via residual graph augmenting paths. |
| **Why does Greedy fail for 0/1 Knapsack?** | Because items cannot be fractionally divided (unlike Fractional Knapsack where greedy is optimal). An item with slightly higher density can waste significant leftover capacity. | We use 2D DP table over discrete capacity states $W$, achieving exact pseudo-polynomial optimality in $\mathcal{O}(n \cdot W)$. |
| **How does Hysteresis prevent oscillation?** | Minor continuous fluctuations cause standard algorithms to flip flow back and forth between two equal-cost servers (route flapping). | A dual-threshold band ($\tau_{\text{enter}} = 85\%, \tau_{\text{exit}} = 70\%$) requires a sustained change before rerouting triggers. |
| **What is the single source of truth?** | The compiled C++17 binary (`cdn_engine.exe`), interacting via persistent stdio JSON-IPC with the Python FastAPI layer. | FastAPI and JavaScript act purely as thin presentation and telemetry transport layers; all algorithmic decisions originate from C++. |
