#ifndef ALGORITHM_TRACE_H
#define ALGORITHM_TRACE_H

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <iomanip>

/**
 * Snapshot of a residual edge at a specific augmentation step.
 */
struct ResidualEdgeSnapshot {
    int from;
    int to;
    std::string fromName;
    std::string toName;
    int capacity;
    int flow;
    int residualCapacity;
    int cost;
};

/**
 * Record of a single Successive Shortest Path (SSP) augmentation step.
 * Phase 5 Requirement: Complete inspection data for step-by-step tracing.
 */
struct SSPStepTrace {
    int iteration;
    std::vector<long long> dist;                // Dijkstra distances under reduced costs
    std::vector<long long> potentialsBefore;    // Potentials pi before Dijkstra/update
    std::vector<long long> potentialsAfter;     // Potentials pi after update
    std::vector<int> pathNodes;                 // Chosen augmenting path (Node IDs)
    std::vector<std::string> pathNames;         // Chosen augmenting path (Node Names)
    std::vector<std::string> allNodeNames;      // All graph node names indexed by node ID
    long long reducedPathCost;                  // Reduced cost of the path (dist[sink])
    long long realPathCost;                     // Real unreduced edge cost sum
    int bottleneckDelta;                        // Bottleneck capacity pushed
    int flowPushed;                             // Flow pushed in this augmentation
    int cumulativeFlow;                         // Total flow accumulated so far
    long long cumulativeCost;                   // Total cost accumulated so far
    std::vector<ResidualEdgeSnapshot> residualGraph; // Compact snapshot of residual edges
};

/**
 * Record of a single Edmonds-Karp BFS augmentation step.
 */
struct EKStepTrace {
    int iteration;
    std::map<int, int> bfsLayers;               // Node ID -> BFS hop distance from source
    std::vector<int> pathNodes;                 // Shortest augmenting path (Node IDs)
    std::vector<std::string> pathNames;         // Shortest augmenting path (Node Names)
    int bottleneckDelta;                        // Bottleneck capacity
    int cumulativeFlow;                         // Total max flow after this step
};

/**
 * Record of the Min-Cut residual BFS extraction.
 */
struct MinCutStepTrace {
    std::vector<int> bfsTraversalOrder;         // Order of nodes visited by residual BFS
    std::vector<int> sSideNodes;                // S-side partition (source-reachable)
    std::vector<std::string> sSideNames;
    std::vector<int> tSideNodes;                // T-side partition (disconnected)
    std::vector<std::string> tSideNames;
    std::vector<ResidualEdgeSnapshot> cutEdges; // Forward saturated edges crossing the cut
    int maxFlow;
    int cutCapacity;
    bool theoremVerified;                       // cutCapacity == maxFlow
};

/**
 * Comprehensive Algorithm Trace Report.
 * Contains SSP, Edmonds-Karp, and Min-Cut traces with JSON and stdout export.
 */
struct AlgorithmTraceReport {
    std::string topologyName;
    int totalDemand;
    int totalRouted;
    long long totalCost;
    std::vector<SSPStepTrace> sspSteps;
    std::vector<EKStepTrace> ekSteps;
    MinCutStepTrace minCutTrace;

    std::string toJsonString(int indent = 2) const;
    void printStdoutTrace() const;
    void printSmall3x3StdoutTrace() const;
};

#endif // ALGORITHM_TRACE_H
