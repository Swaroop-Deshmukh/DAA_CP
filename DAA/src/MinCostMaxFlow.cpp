#include "MinCostMaxFlow.h"
#include <algorithm>
#include <iostream>
#include <cassert>

MinCostMaxFlow::MinCostMaxFlow(int n)
    : nodeCount(n),
      augmentationCount(0),
      executionTimeMs(0.0)
{
    potential.assign(nodeCount, 0);
    dist.resize(nodeCount);
    parentNode.resize(nodeCount);
    parentEdge.resize(nodeCount);
}

/**
 * Requirement 5: Dijkstra with Potentials / Reduced Costs
 * 
 * DAA Theoretical Explanation:
 * Ordinary Dijkstra requires non-negative edge costs because it greedily finalizes
 * node distances. In a residual graph, backward edges have negative costs (-cost).
 * 
 * To eliminate negative costs, Johnson's potential function pi(v) transforms each edge:
 *   reducedCost = edge.cost + potential[u] - potential[v]
 * 
 * Because the initial graph has no negative cycles, initializing pi=0 and updating
 * pi(v) += dist[v] after every Dijkstra run guarantees that reducedCost >= 0 always.
 * This permits using a binary min-heap (std::priority_queue) in O(E log V) time.
 */
bool MinCostMaxFlow::dijkstra(Graph& graph, int source, int sink) {
    if (graph.getNodeCount() != nodeCount) {
        nodeCount = graph.getNodeCount();
        potential.assign(nodeCount, 0);
        dist.resize(nodeCount);
        parentNode.resize(nodeCount);
        parentEdge.resize(nodeCount);
    }

    std::fill(dist.begin(), dist.end(), MCMF_INF_COST);
    std::fill(parentNode.begin(), parentNode.end(), -1);
    std::fill(parentEdge.begin(), parentEdge.end(), -1);


    // Min-heap storing pairs of (reduced_distance, node_id)
    typedef std::pair<long long, int> PLI;
    std::priority_queue<PLI, std::vector<PLI>, std::greater<PLI>> pq;

    dist[source] = 0;
    pq.push({0, source});

    while (!pq.empty()) {
        auto top = pq.top();
        pq.pop();

        long long d = top.first;
        int u = top.second;

        // Skip stale queue entries
        if (d > dist[u]) continue;

        auto& edges = graph.getEdges(u);
        for (int i = 0; i < (int)edges.size(); i++) {
            const auto& edge = edges[i];
            
            // Only traverse edges with positive residual capacity
            if (edge.residualCapacity() <= 0) continue;

            // Compute non-negative reduced cost
            long long reducedCost = (long long)edge.cost + potential[u] - potential[edge.to];
            if (reducedCost < 0) {
                reducedCost = 0; // Guard against minor rounding tolerance
            }

            if (dist[edge.to] > dist[u] + reducedCost) {
                dist[edge.to] = dist[u] + reducedCost;
                parentNode[edge.to] = u;
                parentEdge[edge.to] = i;
                pq.push({dist[edge.to], edge.to});
            }
        }
    }

    // Return true if an augmenting path exists to the sink
    return dist[sink] < (MCMF_INF_COST - 1000000);
}

/**
 * Requirement 6: Min-Cost Max-Flow Core Solver
 * 
 * Algorithm (Successive Shortest Path):
 * 1. While target flow not reached and augmenting path exists:
 *    a. Find shortest path using Dijkstra + potentials
 *    b. Update node potentials: pi(v) += dist[v]
 *    c. Find bottleneck residual capacity along path
 *    d. Augment flow on forward edge, reduce flow on reverse edge
 *    e. Accumulate total cost = sum(bottleneck * edge.cost)
 * 2. Return pair<totalFlow, totalCost>
 */
std::pair<int, long long> MinCostMaxFlow::solve(
    Graph& graph,
    int source,
    int sink,
    int requiredFlow
) {
    auto startTime = std::chrono::high_resolution_clock::now();

    if (graph.getNodeCount() != nodeCount) {
        nodeCount = graph.getNodeCount();
        potential.assign(nodeCount, 0);
        dist.resize(nodeCount);
        parentNode.resize(nodeCount);
        parentEdge.resize(nodeCount);
    }

    int totalFlow = 0;
    long long totalCost = 0;
    augmentationCount = 0;
    potential.assign(nodeCount, 0);

    // Iteratively augment along minimum-cost paths
    while (totalFlow < requiredFlow && dijkstra(graph, source, sink)) {
        // Step 1: Update node potentials to preserve non-negative reduced costs.
        // Uses Ahuja-Magnanti-Orlin potential update: pi(i) += min(dist[i], dist[sink])
        // which unconditionally guarantees c^pi(u, v) >= 0 for all residual edges.
        long long sinkDist = dist[sink];
        for (int i = 0; i < nodeCount; i++) {
            if (dist[i] < (MCMF_INF_COST - 1000000)) {
                potential[i] += std::min(dist[i], sinkDist);
            } else {
                potential[i] += sinkDist;
            }
        }

        // Step 2: Compute bottleneck capacity along the augmenting path
        int bottleneck = requiredFlow - totalFlow;
        for (int curr = sink; curr != source; curr = parentNode[curr]) {
            int p = parentNode[curr];
            int edgeIdx = parentEdge[curr];
            bottleneck = std::min(bottleneck, graph.getEdges(p)[edgeIdx].residualCapacity());
        }

        if (bottleneck <= 0) break;

        // Step 3: Augment flow along forward and reverse edges
        for (int curr = sink; curr != source; curr = parentNode[curr]) {
            int p = parentNode[curr];
            int edgeIdx = parentEdge[curr];
            Edge& fwdEdge = graph.getEdges(p)[edgeIdx];
            Edge& revEdge = graph.getEdges(curr)[fwdEdge.reverseIndex];

            fwdEdge.flow += bottleneck;
            revEdge.flow -= bottleneck;
            totalCost += (long long)bottleneck * fwdEdge.cost;
        }

        totalFlow += bottleneck;
        augmentationCount++;
    }


    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> durationUs = endTime - startTime;
    executionTimeMs = durationUs.count() / 1000.0;

    return {totalFlow, totalCost};
}

std::pair<int, long long> MinCostMaxFlow::solveWithTrace(
    Graph& graph,
    int source,
    int sink,
    int requiredFlow,
    AlgorithmTraceReport& traceReport
) {
    auto startTime = std::chrono::high_resolution_clock::now();

    if (graph.getNodeCount() != nodeCount) {
        nodeCount = graph.getNodeCount();
        potential.assign(nodeCount, 0);
        dist.resize(nodeCount);
        parentNode.resize(nodeCount);
        parentEdge.resize(nodeCount);
    }

    int totalFlow = 0;
    long long totalCost = 0;
    augmentationCount = 0;
    potential.assign(nodeCount, 0);
    traceReport.sspSteps.clear();

    while (totalFlow < requiredFlow && dijkstra(graph, source, sink)) {
        SSPStepTrace step;
        step.iteration = augmentationCount + 1;
        step.dist = dist;
        step.potentialsBefore = potential;

        // Step 1: Update node potentials
        long long sinkDist = dist[sink];
        for (int i = 0; i < nodeCount; i++) {
            if (dist[i] < (MCMF_INF_COST - 1000000)) {
                potential[i] += std::min(dist[i], sinkDist);
            } else {
                potential[i] += sinkDist;
            }
        }
        step.potentialsAfter = potential;

        // Step 2: Extract path and compute bottleneck
        std::vector<int> pathRev;
        for (int curr = sink; curr != -1; curr = parentNode[curr]) {
            pathRev.push_back(curr);
            if (curr == source) break;
        }
        std::vector<int> path(pathRev.rbegin(), pathRev.rend());
        step.pathNodes = path;
        for (int u : path) {
            step.pathNames.push_back(graph.getNodeName(u));
        }
        for (int u = 0; u < nodeCount; ++u) {
            step.allNodeNames.push_back(graph.getNodeName(u));
        }

        int bottleneck = requiredFlow - totalFlow;
        for (int curr = sink; curr != source; curr = parentNode[curr]) {
            int p = parentNode[curr];
            int edgeIdx = parentEdge[curr];
            bottleneck = std::min(bottleneck, graph.getEdges(p)[edgeIdx].residualCapacity());
        }

        if (bottleneck <= 0) break;

        // Step 3: Compute reduced cost and real cost of path
        long long realPathCost = 0;
        long long reducedPathCost = 0;
        for (int curr = sink; curr != source; curr = parentNode[curr]) {
            int p = parentNode[curr];
            int edgeIdx = parentEdge[curr];
            const auto& e = graph.getEdges(p)[edgeIdx];
            realPathCost += e.cost;
            reducedPathCost += ((long long)e.cost + step.potentialsBefore[p] - step.potentialsBefore[curr]);
        }
        step.realPathCost = realPathCost;
        step.reducedPathCost = (dist[sink] < MCMF_INF_COST - 1000000) ? dist[sink] : reducedPathCost;
        step.bottleneckDelta = bottleneck;
        step.flowPushed = bottleneck;

        // Augment flow along path
        for (int curr = sink; curr != source; curr = parentNode[curr]) {
            int p = parentNode[curr];
            int edgeIdx = parentEdge[curr];
            Edge& fwdEdge = graph.getEdges(p)[edgeIdx];
            Edge& revEdge = graph.getEdges(curr)[fwdEdge.reverseIndex];

            fwdEdge.flow += bottleneck;
            revEdge.flow -= bottleneck;
            totalCost += (long long)bottleneck * fwdEdge.cost;
        }

        totalFlow += bottleneck;
        augmentationCount++;

        step.cumulativeFlow = totalFlow;
        step.cumulativeCost = totalCost;

        // Residual graph snapshot
        for (int u = 0; u < nodeCount; ++u) {
            for (const auto& e : graph.getEdges(u)) {
                if (e.capacity > 0 || e.flow > 0 || e.residualCapacity() > 0) {
                    ResidualEdgeSnapshot res;
                    res.from = u;
                    res.to = e.to;
                    res.fromName = graph.getNodeName(u);
                    res.toName = graph.getNodeName(e.to);
                    res.capacity = e.capacity;
                    res.flow = e.flow;
                    res.residualCapacity = e.residualCapacity();
                    res.cost = e.cost;
                    step.residualGraph.push_back(res);
                }
            }
        }

        traceReport.sspSteps.push_back(step);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> durationUs = endTime - startTime;
    executionTimeMs = durationUs.count() / 1000.0;

    traceReport.totalDemand = requiredFlow;
    traceReport.totalRouted = totalFlow;
    traceReport.totalCost = totalCost;

    return {totalFlow, totalCost};
}

std::map<std::string, int> MinCostMaxFlow::extractServerAllocations(const Graph& graph) const {
    std::map<std::string, int> allocations;
    std::map<int, std::string> nodeToCity = {
        {1, "Pune"},
        {2, "Mumbai"},
        {3, "Delhi"},
        {4, "Bangalore"}
    };

    for (const auto& edge : graph.getEdges(Graph::SOURCE_NODE)) {
        if (edge.capacity > 0 && nodeToCity.count(edge.to)) {
            allocations[nodeToCity[edge.to]] = edge.flow;
        }
    }
    return allocations;
}

std::map<std::string, double> MinCostMaxFlow::extractServerCosts(const Graph& graph, int scaleFactor) const {
    std::map<std::string, double> costs;
    std::map<int, std::string> nodeToCity = {
        {1, "Pune"},
        {2, "Mumbai"},
        {3, "Delhi"},
        {4, "Bangalore"}
    };

    for (const auto& edge : graph.getEdges(Graph::SOURCE_NODE)) {
        if (edge.capacity > 0 && nodeToCity.count(edge.to)) {
            double unscaledCost = static_cast<double>(edge.cost) / scaleFactor;
            costs[nodeToCity[edge.to]] = edge.flow * unscaledCost;
        }
    }
    return costs;
}

bool MinCostMaxFlow::bellmanFord(Graph& graph, int source, int sink) {
    if (graph.getNodeCount() != nodeCount) {
        nodeCount = graph.getNodeCount();
        potential.assign(nodeCount, 0);
        dist.resize(nodeCount);
        parentNode.resize(nodeCount);
        parentEdge.resize(nodeCount);
    }

    std::fill(dist.begin(), dist.end(), MCMF_INF_COST);
    std::fill(parentNode.begin(), parentNode.end(), -1);
    std::fill(parentEdge.begin(), parentEdge.end(), -1);

    dist[source] = 0;

    // Standard Classic Bellman-Ford: relax all edges |V| - 1 times with early termination
    for (int iter = 0; iter < nodeCount - 1; ++iter) {
        bool anyUpdate = false;
        for (int u = 0; u < nodeCount; ++u) {
            if (dist[u] >= (MCMF_INF_COST - 1000000)) continue;

            const auto& edges = graph.getEdges(u);
            for (int i = 0; i < (int)edges.size(); ++i) {
                const auto& edge = edges[i];
                if (edge.residualCapacity() <= 0) continue;

                if (dist[edge.to] > dist[u] + edge.cost) {
                    dist[edge.to] = dist[u] + edge.cost;
                    parentNode[edge.to] = u;
                    parentEdge[edge.to] = i;
                    anyUpdate = true;
                }
            }
        }
        if (!anyUpdate) break;
    }

    return dist[sink] < (MCMF_INF_COST - 1000000);
}

std::pair<int, long long> MinCostMaxFlow::solveBellmanFord(
    Graph& graph,
    int source,
    int sink,
    int requiredFlow
) {
    auto startTime = std::chrono::high_resolution_clock::now();

    if (graph.getNodeCount() != nodeCount) {
        nodeCount = graph.getNodeCount();
        potential.assign(nodeCount, 0);
        dist.resize(nodeCount);
        parentNode.resize(nodeCount);
        parentEdge.resize(nodeCount);
    }

    int totalFlow = 0;
    long long totalCost = 0;
    augmentationCount = 0;

    // Iteratively augment along minimum-cost paths using Bellman-Ford
    while (totalFlow < requiredFlow && bellmanFord(graph, source, sink)) {
        // Step 1: Compute bottleneck capacity along path
        int bottleneck = requiredFlow - totalFlow;
        for (int curr = sink; curr != source; curr = parentNode[curr]) {
            int p = parentNode[curr];
            int edgeIdx = parentEdge[curr];
            bottleneck = std::min(bottleneck, graph.getEdges(p)[edgeIdx].residualCapacity());
        }

        if (bottleneck <= 0) break;

        // Step 2: Augment flow
        for (int curr = sink; curr != source; curr = parentNode[curr]) {
            int p = parentNode[curr];
            int edgeIdx = parentEdge[curr];
            Edge& fwdEdge = graph.getEdges(p)[edgeIdx];
            Edge& revEdge = graph.getEdges(curr)[fwdEdge.reverseIndex];

            fwdEdge.flow += bottleneck;
            revEdge.flow -= bottleneck;
            totalCost += (long long)bottleneck * fwdEdge.cost;
        }

        totalFlow += bottleneck;
        augmentationCount++;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = endTime - startTime;
    executionTimeMs = duration.count();

    return {totalFlow, totalCost};
}

static int dfsAdmissibleGraph(
    Graph& graph,
    int u,
    int sink,
    int flowIn,
    const std::vector<long long>& pot,
    std::vector<int>& ptr,
    std::vector<bool>& inDfs,
    long long& totalCost
) {
    if (u == sink || flowIn <= 0) return flowIn;
    inDfs[u] = true;
    auto& edges = graph.getEdges(u);
    for (int& cid = ptr[u]; cid < (int)edges.size(); ++cid) {
        Edge& edge = edges[cid];
        if (edge.residualCapacity() <= 0) continue;
        long long reducedCost = (long long)edge.cost + pot[u] - pot[edge.to];
        if (reducedCost != 0) continue; // Only zero reduced-cost edges belong to the admissible graph
        if (inDfs[edge.to]) continue;   // Avoid directed cycles

        int pushed = dfsAdmissibleGraph(graph, edge.to, sink, std::min(flowIn, edge.residualCapacity()), pot, ptr, inDfs, totalCost);
        if (pushed > 0) {
            edge.flow += pushed;
            Edge& revEdge = graph.getEdges(edge.to)[edge.reverseIndex];
            revEdge.flow -= pushed;
            totalCost += (long long)pushed * edge.cost;
            inDfs[u] = false;
            return pushed;
        }
    }
    inDfs[u] = false;
    return 0;
}

std::pair<int, long long> MinCostMaxFlow::solvePrimalDual(
    Graph& graph,
    int source,
    int sink,
    int requiredFlow,
    int& dijkstraRunsOut,
    int& augmentationsOut
) {
    if (graph.getNodeCount() != nodeCount) {
        nodeCount = graph.getNodeCount();
        potential.assign(nodeCount, 0);
        dist.resize(nodeCount);
        parentNode.resize(nodeCount);
        parentEdge.resize(nodeCount);
    }

    int totalFlow = 0;
    long long totalCost = 0;
    dijkstraRunsOut = 0;
    augmentationsOut = 0;
    potential.assign(nodeCount, 0);

    std::vector<int> ptr(nodeCount, 0);
    std::vector<bool> inDfs(nodeCount, false);

    // Primal-Dual: Run Dijkstra with potentials, then push blocking flow on admissible edges
    while (totalFlow < requiredFlow && dijkstra(graph, source, sink)) {
        dijkstraRunsOut++;

        // Update potentials using Dijkstra distances to sink
        long long sinkDist = dist[sink];
        for (int i = 0; i < nodeCount; i++) {
            if (dist[i] < (MCMF_INF_COST - 1000000)) {
                potential[i] += std::min(dist[i], sinkDist);
            } else {
                potential[i] += sinkDist;
            }
        }

        // Push blocking flow over admissible zero-reduced-cost edges
        std::fill(ptr.begin(), ptr.end(), 0);
        while (totalFlow < requiredFlow) {
            std::fill(inDfs.begin(), inDfs.end(), false);
            int pushed = dfsAdmissibleGraph(graph, source, sink, requiredFlow - totalFlow, potential, ptr, inDfs, totalCost);
            if (pushed <= 0) break;
            totalFlow += pushed;
            augmentationsOut++;
        }
    }

    return {totalFlow, totalCost};
}


bool MinCostMaxFlow::verifyDijkstraAgainstBellmanFord(
    Graph& graph,
    int source,
    int sink,
    int requiredFlow
) {
    // 1. Solve on graph copy using Dijkstra + Potentials
    Graph gDijkstra = graph;
    gDijkstra.resetFlows();
    MinCostMaxFlow dijkstraSolver(gDijkstra.getNodeCount());
    auto dijkstraResult = dijkstraSolver.solve(gDijkstra, source, sink, requiredFlow);

    // 2. Solve on graph copy using Bellman-Ford Reference Solver
    Graph gBellman = graph;
    gBellman.resetFlows();
    MinCostMaxFlow bellmanSolver(gBellman.getNodeCount());
    auto bellmanResult = bellmanSolver.solveBellmanFord(gBellman, source, sink, requiredFlow);

    bool flowsMatch = (dijkstraResult.first == bellmanResult.first);
    bool costsMatch = (dijkstraResult.second == bellmanResult.second);

    return flowsMatch && costsMatch;
}

bool MinCostMaxFlow::verifyReducedCostsNonNegative(const Graph& graph) const {
    for (int u = 0; u < nodeCount; ++u) {
        for (const auto& edge : graph.getEdges(u)) {
            if (edge.residualCapacity() > 0) {
                long long reducedCost = (long long)edge.cost + potential[u] - potential[edge.to];
                if (reducedCost < 0) {
                    return false;
                }
            }
        }
    }
    return true;
}

