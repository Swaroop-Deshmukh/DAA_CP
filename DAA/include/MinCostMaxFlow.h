#ifndef MIN_COST_MAX_FLOW_H
#define MIN_COST_MAX_FLOW_H

#include "Graph.h"
#include <vector>
#include <queue>
#include <utility>
#include <map>
#include <string>
#include <chrono>

// Algorithmic infinite cost sentinel
static const long long MCMF_INF_COST = 1000000000000000LL;

/**
 * Min-Cost Max-Flow Engine.
 * 
 * Implements the Successive Shortest Path (SSP) algorithm
 * with Dijkstra and Johnson's Node Potentials (Reduced Costs).
 * 
 * DAA Core Characteristics:
 * - Solves negative-cost reverse edges in residual graphs.
 * - Reduced Cost: c^pi(u, v) = c(u, v) + pi(u) - pi(v) >= 0.
 * - Time Complexity: O(K * E log V) where K is number of augmentations.
 */
class MinCostMaxFlow {
private:
    int nodeCount;
    std::vector<long long> potential;  // Node potentials pi(v)
    std::vector<long long> dist;       // Distance array for Dijkstra
    std::vector<int> parentNode;       // Predecessor node on shortest path
    std::vector<int> parentEdge;       // Edge index in adj[parentNode] on shortest path

    int augmentationCount;
    double executionTimeMs;

public:
    MinCostMaxFlow(int n = 6);

    /**
     * Requirement 5: Dijkstra with Potentials / Reduced Costs
     */
    bool dijkstra(Graph& graph, int source, int sink);

    /**
     * Requirement 6: Min-Cost Max-Flow solve()
     * Returns: pair<totalFlow, totalCost>
     */
    std::pair<int, long long> solve(
        Graph& graph,
        int source,
        int sink,
        int requiredFlow
    );

    // Extraction & Diagnostics
    std::map<std::string, int> extractServerAllocations(const Graph& graph) const;
    std::map<std::string, double> extractServerCosts(const Graph& graph, int scaleFactor = 100) const;

    int getAugmentationCount() const { return augmentationCount; }
    double getExecutionTimeMs() const { return executionTimeMs; }
};

#endif // MIN_COST_MAX_FLOW_H
