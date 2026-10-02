#include "MinCostMaxFlow.h"
#include <algorithm>
#include <iostream>

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
    dist.assign(nodeCount, MCMF_INF_COST);
    parentNode.assign(nodeCount, -1);
    parentEdge.assign(nodeCount, -1);

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

    int totalFlow = 0;
    long long totalCost = 0;
    augmentationCount = 0;
    potential.assign(nodeCount, 0);

    // Iteratively augment along minimum-cost paths
    while (totalFlow < requiredFlow && dijkstra(graph, source, sink)) {
        // Step 1: Update node potentials to preserve non-negative reduced costs
        for (int i = 0; i < nodeCount; i++) {
            if (dist[i] < (MCMF_INF_COST - 1000000)) {
                potential[i] += dist[i];
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
    std::chrono::duration<double, std::milli> duration = endTime - startTime;
    executionTimeMs = duration.count();

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
