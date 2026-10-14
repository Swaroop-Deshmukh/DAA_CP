# DAA Capstone Examination & Viva Comprehensive Q&A Guide
## Adaptive CDN Traffic Router with Min-Cost Max-Flow Engine

> **Target Audience**: Examiners, Evaluators, and Viva Panels for Design and Analysis of Algorithms (DAA).  
> **Course Level**: Advanced Undergraduate / Graduate DAA Capstone.  
> **Coverage**: 28 Detailed Technical, Mathematical, and Algorithmic Questions & Answers.

---

### Q1: Why does standard Dijkstra's algorithm fail on a residual flow network, and how do Johnson's node potentials resolve this?
**Answer**:
In any residual flow network $G_f$, whenever flow $f(u, v) > 0$ is pushed along a forward edge $(u, v)$ with cost $c(u, v)$, a reverse residual edge $(v, u)$ is created with anti-symmetric cost:
$$c(v, u) = -c(u, v)$$
If $c(u, v) > 0$, the reverse edge has a **strictly negative cost** ($c(v, u) < 0$). Standard Dijkstra's algorithm relies on the greedy property that once a vertex's distance is finalized from the priority queue, no shorter path to it can ever be found. When negative edges exist, this greedy invariant breaks, leading to incorrect shortest paths or infinite loops.

**Johnson's Node Potentials** resolve this by transforming the cost of every edge $(u, v)$ into a **reduced cost**:
$$c^\pi(u, v) = c(u, v) + \pi(u) - \pi(v)$$
By maintaining node potentials $\pi(u)$ and updating them after each shortest path search using $\pi'(v) \leftarrow \pi(v) + d(v)$ (where $d(v)$ is the distance computed under $c^\pi$), the triangle inequality $d(v) \le d(u) + c^\pi(u, v)$ guarantees that:
$$c^{\pi'}(u, v) \ge 0 \quad \forall (u, v) \in E_{G_f}$$
Because all reduced costs are provably non-negative, Dijkstra's algorithm can run safely on the residual graph with a standard binary heap in $\mathcal{O}(|E| \log |V|)$ time.

---

### Q2: Why did the project documentation previously claim that the number of augmentations $K \le 4$, and why does that bound NOT hold in general networks?
**Answer**:
The bound $K \le 4$ applied **strictly to our initial simplified 4-server 6-node topology** ($S \to \text{Server}_i \to T$). In that specific single-tier bipartite graph:
1. Every path from $S$ to $T$ passed through exactly one server $i$.
2. There were no cross-server edges ($i \to j$) or multi-hop transit paths.
3. Each edge had a constant unit cost.
In every augmentation, either server $i$'s full capacity was completely saturated (reducing its forward residual capacity to 0), or the remaining global demand was satisfied. Because costs were static, a saturated server was never augmented again. Thus, at most 4 augmentations were possible.

**Why it does NOT hold in general**:
1. **Convex Cost Bands**: In our updated model, each server has 4 piecewise-linear capacity bands ($1.0\times, 1.5\times, 3.0\times, 8.0\times$). A server can be augmented in multiple steps as it crosses into higher cost bands. Here, $K \le 4 \times |\text{Servers}| = 16$.
2. **Multi-Region & Arbitrary Networks**: In arbitrary graphs with regional aggregation nodes and cross-links, augmentations can push bottleneck flow $\Delta$ that depends on intermediate link capacities. In general networks with integer capacities, the number of augmentations in Successive Shortest Path is bounded by $\mathcal{O}(F)$, where $F$ is the total flow.

---

### Q3: What is the general theoretical time complexity of Successive Shortest Path (SSP) with integer capacities?
**Answer**:
In an arbitrary directed graph with integer capacities and total routed flow $F$:
$$\mathcal{T}_{\text{SSP}} = \mathcal{O}(F \cdot |E| \log |V|)$$
Each augmentation pushes at least 1 unit of integer flow, meaning at most $F$ augmentations occur. Each augmentation runs Dijkstra with a binary heap in $\mathcal{O}(|E| \log |V|)$.
- If capacities are large integers represented in binary (value $U$), this is pseudo-polynomial: $\mathcal{O}(|V| \cdot U \cdot |E| \log |V|)$.
- To achieve strongly polynomial time on arbitrary graphs, one would use the **Capacity Scaling** algorithm ($\mathcal{O}(|E|^2 \log |V|)$) or Orlin's algorithm ($\mathcal{O}(|E| \log |V| \cdot (|E| + |V| \log |V|))$).
- In our CDN simulation, flow is pushed in macro-chunks (bottlenecks $\Delta$ equal to server band capacities), so the empirical number of augmentations $K$ remains tiny ($K=9$ for $N=10$, $K=1229$ for $N=1000$).

---

### Q4: An earlier report draft stated that "potentials prevent negative cycles." Why is that statement theoretically inaccurate?
**Answer**:
Potentials $\pi(u)$ do **NOT** prevent negative cycles; they prevent **negative residual edges** ($c^\pi(u, v) \ge 0$) from appearing to Dijkstra's algorithm.

The algebraic proof shows:
$$\sum_{(u, v) \in C} c^\pi(u, v) = \sum_{(u, v) \in C} \big(c(u, v) + \pi(u) - \pi(v)\big) = \sum_{(u, v) \in C} c(u, v) = \text{cost}(C)$$
Because the sum of potentials around any directed cycle $C$ telescopes to zero, the reduced cost of a cycle is **identical** to its original cost! If a negative cycle existed in the network, its reduced cost would also be negative, and no valid potential function could make all edges non-negative.

Negative cycles in Successive Shortest Path are actually prevented by the **Optimality Theorem of Network Flows**: the initial zero-flow has no negative cycles, and augmenting strictly along shortest paths preserves the absence of negative cycles in the residual network at every step.

---

### Q5: Why does the engine use `std::priority_queue` (binary heap) rather than a Fibonacci heap?
**Answer**:
Theoretically, Dijkstra with a Fibonacci heap achieves $\mathcal{O}(|E| + |V| \log |V|)$, whereas a binary heap (`std::priority_queue`) achieves $\mathcal{O}(|E| \log |V|)$.
However, in practical systems engineering:
1. **Constant Factor Overhead**: Fibonacci heaps require complex pointer manipulations, heap consolidate steps, and dynamic memory allocation per node, with huge constant factors ($C_{\text{Fib}} \gg C_{\text{Binary}}$).
2. **Cache Locality**: `std::priority_queue` is backed by a contiguous `std::vector` in flat memory. On modern CPU architectures with 64-byte cache lines, contiguous arrays produce near-zero cache misses and leverage CPU L1/L2 prefetching.
3. **Graph Sparsity**: In our CDN graphs, $|E| \le 14,000$ for $N=1000$. The $\log |V|$ factor is at most $\log_2(1503) \approx 10.5$. The 10 operations in CPU L1 cache are vastly faster than Fibonacci pointer chasing across heap memory.

---

### Q6: Why do we use piecewise-linear convex cost edges for CDN servers rather than a single fixed cost?
**Answer**:
In real-world computer networking, edge servers exhibit non-linear queuing delays governed by **Kleinrock's Independence Approximation** and the $M/M/1$ queuing model:
$$\text{Delay}(f) = \frac{1}{\mu - f} = \frac{1}{\mu (1 - \rho)}$$
As utilization $\rho = f/C$ approaches $100\%$, delay explodes toward infinity. A linear cost model ($c \cdot f$) would treat the 1st request identical to the 5,999th request, causing the router to pack servers to 100% capacity before spilling over.

By partitioning server capacity into 4 convex tiers:
- Tier 1 ($0\% - 50\%$): Multiplier $1.0\times$ (Nominal cost)
- Tier 2 ($50\% - 80\%$): Multiplier $1.5\times$ (Mild load)
- Tier 3 ($80\% - 95\%$): Multiplier $3.0\times$ (Congestion warning)
- Tier 4 ($95\% - 100\%$): Multiplier $8.0\times$ (Critical saturation penalty)

Because the marginal cost increases monotonically ($c_1 < c_2 < c_3 < c_4$), the Successive Shortest Path solver naturally saturates Tier 1 across multiple servers before pushing any server into Tier 3 or 4. This prevents congestion collapse without requiring non-linear quadratic programming.

---

### Q7: Why does the Greedy (Min-Latency) heuristic fail so dramatically under load?
**Answer**:
Greedy routing makes **locally optimal choices** without evaluating global network state:
1. It inspects all available servers and pushes traffic into the server with the lowest baseline latency (e.g., Mumbai @ 18 ms).
2. It fails to account for capacity saturation or queuing delays. Once Mumbai hits 100% capacity, Greedy continues directing requests to Mumbai because its *base* latency is still reported as 18 ms.
3. In our empirical benchmark, under 13,500 req/s demand, Greedy causes:
   - Mumbai and Bangalore to hit 100% CPU.
   - 1,200 requests/sec dropped or timed out.
   - Average latency to explode to **48.6 ms**.
   - Total cost to surge to **Rs. 712,000**.
MCMF, in contrast, recognizes edge saturation via residual capacities and safely diverts traffic across Pune and Delhi, achieving **22.3 ms average latency** and **Rs. 545,000 cost** (23.5% lower cost, zero drops).

---

### Q8: Describe the exact counterexample where MCMF strictly outperforms Greedy.
**Answer**:
- **Setup**: Ingress demand $D = 13,500$ req/s.
  - Server A (Mumbai): Base latency = 18 ms, capacity = 6,000 req/s, cost = Rs. 6/GB.
  - Server B (Bangalore): Base latency = 24 ms, capacity = 5,000 req/s, cost = Rs. 7.5/GB.
  - Server C (Pune): Base latency = 25 ms, capacity = 5,000 req/s, cost = Rs. 8/GB.
- **Greedy Execution**:
  - Assigns all 13,500 req/s towards Server A (lowest 18 ms).
  - Server A capacity (6,000) is exceeded by 7,500 req/s.
  - Queue overflows, resulting in dropped packets and latency escalation to 95 ms.
- **MCMF Execution**:
  - Formulates the residual flow network.
  - Pushes 6,000 req/s to Server A (saturating its low-cost tier).
  - Evaluates residual graph: augmenting path to Server B costs less than exceeding Server A's convex boundary.
  - Pushes 5,000 req/s to Server B, and remaining 2,500 req/s to Server C.
- **Outcome**: MCMF achieves zero dropped traffic, Rs. 545,000 total cost vs Rs. 712,000 for Greedy, and 22.3 ms average latency vs 48.6 ms for Greedy.

---

### Q9: Why is the 0/1 Knapsack Problem solved via Dynamic Programming instead of Greedy-by-Density for CDN edge caching?
**Answer**:
In Fractional Knapsack, items can be subdivided arbitrarily, and sorting by value density $d_i = b_i / s_i$ yields the optimal solution in $\mathcal{O}(n \log n)$ time.
However, in **0/1 Knapsack**, items are indivisible video or file chunks ($x_i \in \{0, 1\}$). Greedy-by-density fails because an item with high density may consume an awkward amount of capacity, leaving unusable fragmented leftover space.

Because 0/1 Knapsack exhibits **Optimal Substructure** and **Overlapping Subproblems**, Dynamic Programming computes the exact discrete optimum in pseudo-polynomial time $\mathcal{O}(n \cdot W)$, where $W$ is the cache capacity.

---

### Q10: What is the exact counterexample showing Greedy fails for 0/1 Knapsack caching?
**Answer**:
- **Cache Storage Limit**: $W = 100\text{ MB}$.
- **Candidate Files**:
  1. `Video_Alpha`: Size $s_1 = 60\text{ MB}$, Benefit $b_1 = 6.3 \implies$ Density $= 6.3 / 60 = \mathbf{0.105}$
  2. `Video_Beta`: Size $s_2 = 50\text{ MB}$, Benefit $b_2 = 5.0 \implies$ Density $= 5.0 / 50 = \mathbf{0.100}$
  3. `Video_Gamma`: Size $s_3 = 50\text{ MB}$, Benefit $b_3 = 5.0 \implies$ Density $= 5.0 / 50 = \mathbf{0.100}$
- **Greedy by Density Choice**:
  - Picks `Video_Alpha` first (highest density 0.105, 60 MB).
  - Remaining space: $100 - 60 = 40\text{ MB}$.
  - Neither `Video_Beta` (50 MB) nor `Video_Gamma` (50 MB) fits in the 40 MB remainder.
  - **Total Greedy Benefit: 6.3**.
- **0/1 Knapsack DP Choice**:
  - Evaluates combinations: $\{2, 3\}$ uses $50 + 50 = 100\text{ MB}$ and yields $5.0 + 5.0 = \mathbf{10.0}$.
  - **Total DP Benefit: 10.0**.
- **Improvement**: Dynamic Programming achieves a **+58.7% benefit gain** over Greedy.

---

### Q11: What does the Max-Flow Min-Cut Theorem state, and how do we computationally extract the minimum cut from the residual graph?
**Answer**:
**Theorem**: In any flow network, the maximum flow from source $s$ to sink $t$ is exactly equal to the minimum capacity of an $s-t$ cut:
$$\max |f| = \min_{(S, T)} \text{cap}(S, T)$$

**Computational Extraction**:
1. Run Edmonds-Karp or MCMF until no augmenting path exists from $s$ to $t$ in the residual graph $G_f$.
2. Execute a Breadth-First Search (BFS) or Depth-First Search (DFS) starting at source $s$, following only edges with **positive residual capacity** ($r(u, v) = \text{cap}(u, v) - f(u, v) > 0$).
3. Let $S$ be the set of all vertices reachable from $s$ in $G_f$.
4. Let $T = V \setminus S$ (the vertices disconnected from $s$).
5. The **minimum cut edges** are all original edges $(u, v) \in E$ such that $u \in S$ and $v \in T$. Every such edge is guaranteed to be completely saturated ($f(u, v) = \text{cap}(u, v)$).
6. Summing their capacities yields exactly $\text{cap}(S, T) = |f_{\max}|$.

---

### Q12: How does Edmonds-Karp differ from Successive Shortest Path (SSP)?
**Answer**:
- **Edmonds-Karp**: Solves the **Maximum Flow** problem (ignoring financial costs). It uses Breadth-First Search (BFS) to find augmenting paths with the minimum number of edges (shortest unweighted hop distance). It runs in $\mathcal{O}(|V| \cdot |E|^2)$ time, which is strongly polynomial and independent of edge capacities.
- **Successive Shortest Path (SSP)**: Solves the **Min-Cost Max-Flow** problem. It finds augmenting paths with the minimum weighted reduced cost ($c^\pi$) using Dijkstra with Johnson's potentials. It terminates when either target demand is satisfied or network capacity is saturated, achieving the minimal total financial/latency cost.

---

### Q13: What is Holt's Linear Exponential Smoothing and how is it used for predictive routing?
**Answer**:
Holt's two-parameter smoothing extends simple exponential smoothing to time series exhibiting linear trends. It maintains two internal state variables:
1. **Level Equation** ($L_t$): Estimates current baseline traffic:
   $$L_t = \alpha Y_t + (1 - \alpha)(L_{t-1} + T_{t-1}) \quad (\alpha = 0.40)$$
2. **Trend Equation** ($T_t$): Estimates current rate of change (slope):
   $$T_t = \beta (L_t - L_{t-1}) + (1 - \beta) T_{t-1} \quad (\beta = 0.30)$$
3. **Forecast Equation**: Predicts traffic $k$ steps ahead:
   $$\hat{Y}_{t+k} = L_t + k \cdot T_t$$

**Application to CDN Routing**:
Instead of waiting for an edge server to become congested (reactive), the engine checks if trend slope $T_t > \tau_{\text{threshold}}$ (+100 req/s/tick). When triggered, the engine solves MCMF for the projected load $\hat{Y}_{t+1}$, proactively pre-warming routes 1 cycle ahead. In testing, this reduced peak congestion delay from **94.2 ms down to 38.4 ms** (a 59% reduction).

---

### Q14: How does Hysteresis prevent route flapping (flip-flop oscillations)?
**Answer**:
In production networks, minor traffic fluctuations (e.g. $\pm 2\%$) cause two servers with nearly identical costs to alternate between being the "cheapest" path on successive cycles. A naive router would repeatedly flip 10,000 req/s between them every 1.5 seconds, creating route flapping, invalidating DNS caches, and resetting TCP congestion windows.

Our **Stability Manager** implements a dual-threshold Schmitt trigger hysteresis band:
1. **Entering Congestion**: Traffic must exceed $\tau_{\text{enter}} = 85\%$ before a server is marked congested.
2. **Exiting Congestion**: Traffic must drop below $\tau_{\text{exit}} = 70\%$ before it is marked healthy again.
3. **Delta Threshold**: If the recomputed flow distribution differs from the previous cycle by less than $\Delta_{\text{thresh}} = \pm 5\%$, the previous routing table is preserved.
This completely suppresses micro-oscillations while responding immediately to legitimate macro-events (e.g. server down or flash crowd).

---

### Q15: Why does the C++ engine use scaled integer arithmetic instead of floating-point arithmetic for edge costs?
**Answer**:
1. **Exact Algorithmic Termination**: Floating-point operations incur rounding errors (e.g. $0.1 + 0.2 \ne 0.3$). In residual networks, comparing floating-point reduced costs against zero can lead to precision errors where $c^\pi(u, v) = -10^{-16}$, causing Dijkstra to misclassify edges and cycle infinitely.
2. **Fixed-Point Scaling**: The engine scales composite costs to 64-bit integers (`long long`):
   $$W = \text{round}\big(100 \times (\alpha \cdot \text{Latency} + \beta \cdot \text{Cost})\big)$$
   where 1 integer unit equals 0.01 INR or 0.01 ms.
3. **Performance**: Integer arithmetic is executed natively in single-cycle ALU instructions without FPU pipeline stalls.

---

### Q16: How does the engine guarantee feasibility when ingress demand exceeds fleet capacity (Fleet Overload Scenario)?
**Answer**:
When ingress demand exceeds total network capacity ($D = 26,000 > 21,500$ req/s):
1. Successive Shortest Path pushes augmenting paths until all paths from source $s$ to sink $t$ in the residual graph are saturated.
2. The total flow pushed matches the network cut capacity ($21,500$ req/s).
3. The engine detects that residual capacity is exhausted before demand is satisfied ($f < D$).
4. It computes unrouted dropped traffic: $\text{Unrouted} = D - f_{\text{routed}} = 4,500$ req/s.
5. In the Multi-Region network model, an explicit **Origin Fallback Edge** exists from region nodes directly to the origin data center with high cost. Excess traffic overflows gracefully to the origin rather than failing silently.

---

### Q17: What are the 7 automated invariant checks verified in the Phase 3 self-test suite?
**Answer**:
1. **Flow Conservation & Capacity Bounds**: For every non-terminal node $\sum f_{\text{in}} = \sum f_{\text{out}}$, and for every edge $0 \le f(e) \le \text{cap}(e)$.
2. **Reduced Costs Non-Negativity**: For every residual edge in $G_f$, $c^\pi(u, v) \ge 0$.
3. **Dijkstra+Potentials vs Bellman-Ford Equivalence**: Over 1,000+ random instances, Dijkstra with Johnson potentials produces identical shortest paths and total costs as Bellman-Ford.
4. **Brute-Force Flow Verification**: On small topologies ($R, S \le 3$), MCMF matches the optimal solution found by exhaustive combinatorial integer search.
5. **Max-Flow Min-Cut Equivalence**: $\text{MaxFlow}(\text{Edmonds-Karp}) == \text{MinCutCapacity} == \text{MaxFlow}(\text{MCMF})$.
6. **0/1 Knapsack DP vs Exhaustive Enumeration**: Evaluates 15 catalogs with $2^{12} = 4,096$ subsets, confirming DP matches brute force exactly.
7. **Total Demand Conservation**: Across all scenarios, $\text{Demand} = \text{EdgeTraffic} + \text{OriginTraffic}$.

---

### Q18: What is the empirical scaling constant measured in the scalability benchmark, and what does it prove?
**Answer**:
Across random topologies ranging from 10 to 1,000 servers ($V=18$ to $1503$, $E=142$ to $14272$):
$$\text{Ratio} = \frac{\text{Measured Time (ns)}}{K \cdot |E| \cdot \log_2 |V|} \approx 2.19 - 3.93 \text{ ns/operation}$$
This empirical ratio remains constant across two orders of magnitude in network size. It proves that the implementation strictly conforms to the theoretical asymptotic bound $\mathcal{O}(K \cdot |E| \log |V|)$ without hidden polynomial explosion or memory paging bottlenecks.

---

### Q19: Why is Multi-Commodity Flow NP-hard with integer demands, and how does our single-commodity formulation simplify it?
**Answer**:
In **Multi-Commodity Flow (MCF)**, multiple distinct traffic commodities (e.g. Netflix video streams vs Zoom video calls vs web assets) have individual source-sink pairs $(s_k, t_k)$ that share edge capacities.
- **Fractional MCF**: Solvable in polynomial time via Linear Programming (e.g. Interior Point or ellipsoid methods).
- **Integer Multi-Commodity Flow**: Known to be **NP-hard** (Karp's 21 NP-complete problems, 1972) because it generalizes integer knapsack and bin-packing.
Our system formulates CDN routing as a **Single-Commodity Flow with Multi-Region Sources and Virtual Super-Source/Sink**, aggregate traffic rates (req/s) are pooled into continuous or integer flows, enabling polynomial-time exact optimization via MCMF.

---

### Q20: What are the engineering limitations of this model compared to production CDNs like Cloudflare or Akamai?
**Answer**:
1. **Centralized Controller**: The engine assumes a centralized solver computes global routes. Planetary CDNs operate hundreds of thousands of servers across 300+ cities using decentralized BGP Anycast and distributed edge routing.
2. **DNS TTL Caching**: Production traffic is steered via Geo-DNS. Once an IP is returned, client OSes cache it for the duration of the DNS TTL (30 to 300 seconds), meaning flow cannot be reallocated instantaneously on 1.5-second cycles without client-side coordination.
3. **TCP Connection Affinity**: Re-routing live TCP/QUIC sessions mid-stream breaks established connections unless stateful Anycast connection proxies are used.

---

### Q21: What is the production deployment roadmap for this algorithmic engine?
**Answer**:
1. **Core Service**: Package the C++17 engine into a containerized microservice written in Go or Rust with C++ FFI bindings, running as a stateless daemon.
2. **Transport**: Expose high-speed **gRPC** endpoints with Protocol Buffers (`proto3`) for microsecond IPC serialization.
3. **Telemetry Ingestion**: Ingest live edge server metrics via Apache Kafka or Redis Streams.
4. **Orchestration**: Deploy on **Kubernetes** with Custom Resource Definitions (CRDs) automatically adjusting edge capacity annotations based on convex cost saturation alerts.

---

### Q22: What is the "Single Source of Truth" architecture between Python and C++ in Phase 6/7?
**Answer**:
The C++17 binary (`cdn_engine.exe`) is the sole authority for all physics, graph algorithms, and routing decisions.
- Python (FastAPI) and JavaScript (Dashboard) perform **zero routing math**.
- FastAPI communicates with the C++ engine over persistent standard input/output (`stdin`/`stdout`) via structured JSON-IPC.
- The engine computes decisions in microseconds ($11.8\ \mu\text{s}$) and returns JSON snapshots.
- If the long-lived IPC pipe is interrupted, a fallback subprocess runner automatically executes individual scenario runs, ensuring 100% dashboard uptime.

---

### Q23: How does the "Why this decision?" plain-language diagnostic panel work?
**Answer**:
When an operator views a routing decision, the engine generates an automated diagnostic explanation based on:
1. **Active Convex Tiers**: Identifies servers operating in Tier 2 ($1.5\times$), Tier 3 ($3.0\times$), or Tier 4 ($8.0\times$).
2. **Bottleneck Constraints**: Flags links where residual capacity $r(u, v) = 0$.
3. **Marginal Reduced Cost Analysis**: Explains that routing the next request to Mumbai would cost $3.0\times$ its base rate, making Bangalore's Tier 1 path cheaper, thus justifying why traffic was diverted.

---

### Q24: What is the difference between Capacity Scaling and Successive Shortest Path?
**Answer**:
- **Successive Shortest Path (SSP)**: Augments along the shortest path in the entire residual graph, pushing the bottleneck capacity $\Delta = \min_{(u, v) \in P} r(u, v)$ of that specific path. Its complexity depends on flow value $F$ ($\mathcal{O}(F \cdot |E| \log |V|)$).
- **Capacity Scaling**: Maintains a scaling parameter $\Delta = 2^{\lfloor \log_2 U \rfloor}$. It only considers edges with residual capacity $r(u, v) \ge \Delta$. Once no such augmenting paths exist, $\Delta$ is halved ($\Delta \leftarrow \Delta / 2$). This guarantees at most $2|E|$ augmentations per scale phase, yielding a **strongly polynomial** runtime $\mathcal{O}(|E| \log U \cdot |E| \log |V|)$.

---

### Q25: How does the residual graph BFS visually prove that a server is a bottleneck in the Min-Cut overlay?
**Answer**:
When a server's link capacity is saturated ($f(u, v) = \text{cap}(u, v)$), its forward residual capacity becomes 0 ($r(u, v) = 0$).
During the BFS from source $s$, the traversal cannot cross that saturated edge. If all forward paths to the sink are blocked, the BFS terminates, placing all upstream nodes in $S$ and all downstream nodes in $T$. In the dashboard, the saturated edges cross the red dashed cut line, glowing crimson with residual capacity 0.

---

### Q26: Why is Bellman-Ford too slow for high-frequency routing in large CDN topologies?
**Answer**:
Bellman-Ford performs $|V|-1$ relaxation passes over all $|E|$ edges, running in $\mathcal{O}(|V| \cdot |E|)$ per augmentation.
For $N=1000$ servers ($V=1503, E=14272$):
- Bellman-Ford requires $1503 \times 14272 \approx \mathbf{21.4\text{ million operations}}$ per path.
- In contrast, Dijkstra with Johnson potentials requires $|E| \log |V| \approx 14272 \times 10.5 \approx \mathbf{150,000\text{ operations}}$ per path.
Dijkstra with Johnson potentials is over **$140\times$ faster** asymptotically, allowing thousands of augmentations to finish in fractions of a second.

---

### Q27: How does the simulator model diurnal traffic cycles and Poisson burstiness?
**Answer**:
In `TrafficSimulator.cpp`:
1. **Diurnal Pattern**: Base traffic is modulated by a smooth sinusoidal time function:
   $$D(t) = D_{\text{base}} \times \big(1.0 + 0.4 \sin(2\pi t / 86400)\big)$$
   representing peak daytime usage and nighttime troughs.
2. **Poisson Arrival Modeling**: Instantaneous request arrivals per tick follow a Poisson distribution with mean $\lambda = D(t)$:
   $$P(X = k) = \frac{\lambda^k e^{-\lambda}}{k!}$$
   approximated using standard inverse-transform or Gaussian sampling for large $\lambda$.

---

### Q28: If given another 3 months to work on this project, what single architectural upgrade would you implement?
**Answer**:
I would implement **Frank-Wolfe Convex Cost Flow Optimization** with continuous nonlinear $M/M/1$ delay curves. This would replace piecewise-linear discrete bands with smooth quadratic/convex penalty functions, computing optimal fractional routing allocations using iterative line searches and gradient descent on the flow polytope, matching the mathematical formulations used in modern network utility maximization (NUM).
