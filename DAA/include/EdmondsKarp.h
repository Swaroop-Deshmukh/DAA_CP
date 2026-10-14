#ifndef EDMONDS_KARP_H
#define EDMONDS_KARP_H

#include "Graph.h"
#include "AlgorithmTrace.h"
#include <vector>
#include <string>
#include <queue>
#include <iostream>

/**
 * Cut edge crossing the S-T min-cut partition.
 */
struct CutEdge {
    int from;
    int to;
    std::string fromName;
    std::string toName;
    int capacity;
    int flow;
};

/**
 * Result record from Min-Cut extraction.
 */
struct MinCutResult {
    int maxFlow;
    int cutCapacity;
    std::vector<int> sSideNodes;
    std::vector<int> tSideNodes;
    std::vector<CutEdge> cutEdges;
    std::vector<std::string> bottleneckRecommendations;
    bool minCutEqualsMaxFlow;
    double executionTimeUs;

    void printReport() const;
};

/**
 * Edmonds-Karp Max-Flow and Min-Cut Engine.
 * 
 * Implemented from first principles in standard C++17.
 * - Max-Flow: Breadth-First Search (BFS) finding shortest augmenting paths in O(V * E^2).
 * - Min-Cut: Residual graph reachability BFS from Source (S) to identify saturated bottlenecks.
 * - Theorem: cutCapacity == maxFlowValue (Max-Flow Min-Cut Theorem).
 */
class EdmondsKarp {
private:
    int nodeCount;
    std::vector<int> parentNode;
    std::vector<int> parentEdge;

    bool bfs(Graph& graph, int source, int sink);
    bool bfsWithLayers(Graph& graph, int source, int sink, std::map<int, int>& outLayers);

public:
    EdmondsKarp(int n = 6);

    /**
     * Compute Maximum Flow using Edmonds-Karp (BFS).
     */
    int solveMaxFlow(Graph& graph, int source, int sink, int maxDemand = -1);

    /**
     * Compute Maximum Flow with step-by-step BFS layer and path tracing.
     */
    int solveMaxFlowWithTrace(Graph& graph, int source, int sink, int maxDemand, AlgorithmTraceReport& traceReport);

    /**
     * Extract Min-Cut partition (S, T) and identify saturated bottleneck edges.
     * Asserts Max-Flow Min-Cut Theorem: cutCapacity == maxFlow.
     */
    MinCutResult extractMinCut(Graph& graph, int source, int sink);

    /**
     * Extract Min-Cut partition with traversal tracing.
     */
    MinCutResult extractMinCutWithTrace(Graph& graph, int source, int sink, AlgorithmTraceReport& traceReport);
};

#endif // EDMONDS_KARP_H
