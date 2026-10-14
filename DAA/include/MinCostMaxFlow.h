#ifndef MIN_COST_MAX_FLOW_H
#define MIN_COST_MAX_FLOW_H

#include "Graph.h"
#include "AlgorithmTrace.h"
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
 * Implements the Successive Shortest Path (SSP) algorithm:
 * 1. Primary Solver: Dijkstra with Johnson's Node Potentials (Reduced Costs) in O(K * E log V).
 * 2. Reference Solver: Bellman-Ford / SPFA directly handling negative residual costs in O(K * V * E).
 */
class MinCostMaxFlow {
private:
    int nodeCount;
    std::vector<long long> potential;  // Node potentials pi(v)
    std::vector<long long> dist;       // Distance array for Dijkstra / Bellman-Ford
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
     * Requirement 6: Min-Cost Max-Flow solve() via Dijkstra + Potentials
     * Returns: pair<totalFlow, totalCost>
     */
    std::pair<int, long long> solve(
        Graph& graph,
        int source,
        int sink,
        int requiredFlow
    );

    /**
     * Phase 5 Requirement: Successive Shortest Path (SSP) solve with full step-by-step trace
     */
    std::pair<int, long long> solveWithTrace(
        Graph& graph,
        int source,
        int sink,
        int requiredFlow,
        AlgorithmTraceReport& traceReport
    );

    /**
     * Phase 2a Requirement 3: Reference Solver (Bellman-Ford SSP without potentials)
     */
    bool bellmanFord(Graph& graph, int source, int sink);

    std::pair<int, long long> solveBellmanFord(
        Graph& graph,
        int source,
        int sink,
        int requiredFlow
    );

    /**
     * Optional Third Solver: Primal-Dual algorithm with DFS blocking flow.
     * After each Dijkstra run with potentials, pushes multiple augmenting paths
     * along zero-reduced-cost edges in the admissible graph (Dinic style).
     * Returns: pair<totalFlow, totalCost>
     */
    std::pair<int, long long> solvePrimalDual(
        Graph& graph,
        int source,
        int sink,
        int requiredFlow,
        int& dijkstraRunsOut,
        int& augmentationsOut
    );

    /**
     * Verification routine: Asserts that Dijkstra+potentials and Bellman-Ford
     * produce mathematically identical flow and cost values.
     */
    bool verifyDijkstraAgainstBellmanFord(
        Graph& graph,
        int source,
        int sink,
        int requiredFlow
    );


    /**
     * Phase 3 Invariant Check 2: Verify reduced costs are strictly non-negative
     * for all residual edges: c^pi(u, v) = c(u, v) + pi(u) - pi(v) >= 0.
     */
    bool verifyReducedCostsNonNegative(const Graph& graph) const;

    // Extraction & Diagnostics
    std::map<std::string, int> extractServerAllocations(const Graph& graph) const;
    std::map<std::string, double> extractServerCosts(const Graph& graph, int scaleFactor = 100) const;

    int getAugmentationCount() const { return augmentationCount; }
    double getExecutionTimeMs() const { return executionTimeMs; }
};

#endif // MIN_COST_MAX_FLOW_H
