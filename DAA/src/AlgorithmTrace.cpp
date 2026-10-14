#include "AlgorithmTrace.h"
#include <iomanip>
#include <sstream>

std::string AlgorithmTraceReport::toJsonString(int indent) const {
    (void)indent; // Standard indentation
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"schema_version\": 2,\n";
    ss << "  \"topology\": \"" << topologyName << "\",\n";
    ss << "  \"total_demand\": " << totalDemand << ",\n";
    ss << "  \"total_routed\": " << totalRouted << ",\n";
    ss << "  \"total_cost_scaled\": " << totalCost << ",\n";
    ss << "  \"total_cost_rs\": " << std::fixed << std::setprecision(2) << (static_cast<double>(totalCost) / 100.0) << ",\n";

    // 1. SSP Trace array ("trace" array required by Phase 5)
    ss << "  \"trace\": [\n";
    for (size_t i = 0; i < sspSteps.size(); ++i) {
        const auto& step = sspSteps[i];
        ss << "    {\n";
        ss << "      \"iteration\": " << step.iteration << ",\n";

        // dist array
        ss << "      \"dist\": [";
        for (size_t d = 0; d < step.dist.size(); ++d) {
            if (d > 0) ss << ", ";
            if (step.dist[d] >= 1000000000000000LL - 1000000) {
                ss << "-1";
            } else {
                ss << step.dist[d];
            }
        }
        ss << "],\n";

        // potentials_before
        ss << "      \"potentials_before\": [";
        for (size_t p = 0; p < step.potentialsBefore.size(); ++p) {
            if (p > 0) ss << ", ";
            ss << step.potentialsBefore[p];
        }
        ss << "],\n";

        // potentials_after
        ss << "      \"potentials_after\": [";
        for (size_t p = 0; p < step.potentialsAfter.size(); ++p) {
            if (p > 0) ss << ", ";
            ss << step.potentialsAfter[p];
        }
        ss << "],\n";

        // chosen path nodes
        ss << "      \"chosen_path_nodes\": [";
        for (size_t n = 0; n < step.pathNodes.size(); ++n) {
            if (n > 0) ss << ", ";
            ss << step.pathNodes[n];
        }
        ss << "],\n";

        // chosen path names
        ss << "      \"chosen_path_names\": [";
        for (size_t n = 0; n < step.pathNames.size(); ++n) {
            if (n > 0) ss << ", ";
            ss << "\"" << step.pathNames[n] << "\"";
        }
        ss << "],\n";

        ss << "      \"reduced_path_cost\": " << step.reducedPathCost << ",\n";
        ss << "      \"real_path_cost\": " << step.realPathCost << ",\n";
        ss << "      \"real_path_cost_rs\": " << std::fixed << std::setprecision(2) << (static_cast<double>(step.realPathCost) / 100.0) << ",\n";
        ss << "      \"bottleneck_delta\": " << step.bottleneckDelta << ",\n";
        ss << "      \"flow_pushed\": " << step.flowPushed << ",\n";
        ss << "      \"cumulative_flow\": " << step.cumulativeFlow << ",\n";
        ss << "      \"cumulative_cost\": " << step.cumulativeCost << ",\n";
        ss << "      \"cumulative_cost_rs\": " << std::fixed << std::setprecision(2) << (static_cast<double>(step.cumulativeCost) / 100.0) << ",\n";

        // compact residual-graph snapshot
        ss << "      \"residual_graph\": [\n";
        for (size_t r = 0; r < step.residualGraph.size(); ++r) {
            const auto& e = step.residualGraph[r];
            ss << "        {\"from\": " << e.from
               << ", \"to\": " << e.to
               << ", \"from_name\": \"" << e.fromName << "\""
               << ", \"to_name\": \"" << e.toName << "\""
               << ", \"capacity\": " << e.capacity
               << ", \"flow\": " << e.flow
               << ", \"residual_capacity\": " << e.residualCapacity
               << ", \"cost\": " << e.cost
               << "}" << (r + 1 < step.residualGraph.size() ? "," : "") << "\n";
        }
        ss << "      ]\n";

        ss << "    }" << (i + 1 < sspSteps.size() ? "," : "") << "\n";
    }
    ss << "  ],\n";

    // 2. Edmonds-Karp BFS Trace
    ss << "  \"edmonds_karp_trace\": [\n";
    for (size_t i = 0; i < ekSteps.size(); ++i) {
        const auto& ek = ekSteps[i];
        ss << "    {\n";
        ss << "      \"iteration\": " << ek.iteration << ",\n";
        ss << "      \"bfs_layers\": {";
        bool first = true;
        for (const auto& kv : ek.bfsLayers) {
            if (!first) ss << ", ";
            ss << "\"" << kv.first << "\": " << kv.second;
            first = false;
        }
        ss << "},\n";

        ss << "      \"chosen_path_nodes\": [";
        for (size_t n = 0; n < ek.pathNodes.size(); ++n) {
            if (n > 0) ss << ", ";
            ss << ek.pathNodes[n];
        }
        ss << "],\n";

        ss << "      \"chosen_path_names\": [";
        for (size_t n = 0; n < ek.pathNames.size(); ++n) {
            if (n > 0) ss << ", ";
            ss << "\"" << ek.pathNames[n] << "\"";
        }
        ss << "],\n";

        ss << "      \"bottleneck_delta\": " << ek.bottleneckDelta << ",\n";
        ss << "      \"cumulative_flow\": " << ek.cumulativeFlow << "\n";
        ss << "    }" << (i + 1 < ekSteps.size() ? "," : "") << "\n";
    }
    ss << "  ],\n";

    // 3. Min-Cut BFS Trace
    ss << "  \"min_cut_trace\": {\n";
    ss << "    \"bfs_traversal_order\": [";
    for (size_t i = 0; i < minCutTrace.bfsTraversalOrder.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << minCutTrace.bfsTraversalOrder[i];
    }
    ss << "],\n";

    ss << "    \"s_side_nodes\": [";
    for (size_t i = 0; i < minCutTrace.sSideNodes.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << minCutTrace.sSideNodes[i];
    }
    ss << "],\n";

    ss << "    \"s_side_names\": [";
    for (size_t i = 0; i < minCutTrace.sSideNames.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "\"" << minCutTrace.sSideNames[i] << "\"";
    }
    ss << "],\n";

    ss << "    \"t_side_nodes\": [";
    for (size_t i = 0; i < minCutTrace.tSideNodes.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << minCutTrace.tSideNodes[i];
    }
    ss << "],\n";

    ss << "    \"t_side_names\": [";
    for (size_t i = 0; i < minCutTrace.tSideNames.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "\"" << minCutTrace.tSideNames[i] << "\"";
    }
    ss << "],\n";

    ss << "    \"cut_edges\": [\n";
    for (size_t i = 0; i < minCutTrace.cutEdges.size(); ++i) {
        const auto& ce = minCutTrace.cutEdges[i];
        ss << "      {\"from\": " << ce.from
           << ", \"to\": " << ce.to
           << ", \"from_name\": \"" << ce.fromName << "\""
           << ", \"to_name\": \"" << ce.toName << "\""
           << ", \"capacity\": " << ce.capacity
           << ", \"flow\": " << ce.flow
           << ", \"residual_capacity\": " << ce.residualCapacity
           << "}" << (i + 1 < minCutTrace.cutEdges.size() ? "," : "") << "\n";
    }
    ss << "    ],\n";

    ss << "    \"max_flow\": " << minCutTrace.maxFlow << ",\n";
    ss << "    \"cut_capacity\": " << minCutTrace.cutCapacity << ",\n";
    ss << "    \"theorem_verified\": " << (minCutTrace.theoremVerified ? "true" : "false") << "\n";
    ss << "  }\n";

    ss << "}\n";
    return ss.str();
}

void AlgorithmTraceReport::printStdoutTrace() const {
    printSmall3x3StdoutTrace();
}

void AlgorithmTraceReport::printSmall3x3StdoutTrace() const {
    std::cout << "\n========================================================================================\n";
    std::cout << "                 PHASE 5: ALGORITHM TRACE MODE (MCMF + EK + MIN-CUT)                    \n";
    std::cout << "========================================================================================\n";
    std::cout << " Topology:     " << topologyName << "\n";
    std::cout << " Total Demand: " << totalDemand << " req/s | Routed: " << totalRouted 
              << " req/s | Total Cost: Rs. " << std::fixed << std::setprecision(2) << (static_cast<double>(totalCost) / 100.0) << "\n";
    std::cout << " Augmentations (K): " << sspSteps.size() << " Successive Shortest Path Iterations\n";
    std::cout << "========================================================================================\n\n";

    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << " PART A: SUCCESSIVE SHORTEST PATH (DIJKSTRA + JOHNSON'S POTENTIALS) AUGMENTATIONS\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    for (const auto& step : sspSteps) {
        std::cout << "\n>>> [AUGMENTATION STEP " << step.iteration << "] <<<\n";

        // Chosen path
        std::cout << " Chosen Path: ";
        for (size_t i = 0; i < step.pathNodes.size(); ++i) {
            std::cout << step.pathNames[i];
            if (i + 1 < step.pathNodes.size()) std::cout << " -> ";
        }
        std::cout << "\n Path Node IDs: [";
        for (size_t i = 0; i < step.pathNodes.size(); ++i) {
            std::cout << step.pathNodes[i] << (i + 1 < step.pathNodes.size() ? " -> " : "");
        }
        std::cout << "]\n";

        // Path cost & flow
        std::cout << " Bottleneck Delta (Delta): " << step.bottleneckDelta << " req/s (Flow Pushed: " << step.flowPushed << " req/s)\n";
        std::cout << " Reduced Path Cost:        " << step.reducedPathCost << " units (dist[sink])\n";
        std::cout << " Real Path Cost:           " << step.realPathCost << " units (Rs. " 
                  << std::fixed << std::setprecision(2) << (static_cast<double>(step.realPathCost) / 100.0) << " per unit)\n";
        std::cout << " Cumulative Flow:          " << step.cumulativeFlow << " / " << totalDemand << " req/s\n";
        std::cout << " Cumulative Cost:          Rs. " << std::fixed << std::setprecision(2) 
                  << (static_cast<double>(step.cumulativeCost) / 100.0) << " (" << step.cumulativeCost << " scaled units)\n";

        // Potentials & Distances Table
        std::cout << "\n Node Potentials & Reduced Distances:\n";
        std::cout << "   " << std::left << std::setw(6) << "Node" 
                  << std::setw(26) << "Name" 
                  << std::setw(16) << "pi (Before)" 
                  << std::setw(16) << "dist (Reduced)" 
                  << std::setw(16) << "pi (After)" << "\n";
        std::cout << "   " << std::string(76, '-') << "\n";

        size_t nNodes = step.dist.size();
        for (size_t u = 0; u < nNodes; ++u) {
            std::string dStr = (step.dist[u] >= 1000000000000000LL - 1000000) ? "INF" : std::to_string(step.dist[u]);
            std::string nameStr = (u < step.allNodeNames.size()) ? step.allNodeNames[u] : ("Node_" + std::to_string(u));
            if (u < step.potentialsBefore.size()) {
                std::cout << "   " << std::left << std::setw(6) << u
                          << std::setw(26) << nameStr
                          << std::setw(16) << step.potentialsBefore[u]
                          << std::setw(16) << dStr
                          << std::setw(16) << step.potentialsAfter[u] << "\n";
            }
        }

        // Compact Residual Graph Snapshot
        std::cout << "\n Residual Graph Snapshot (Active Residual Edges):\n";
        std::cout << "   " << std::left << std::setw(6) << "From" 
                  << std::setw(6) << "To" 
                  << std::setw(24) << "From Name" 
                  << std::setw(24) << "To Name" 
                  << std::setw(8) << "Cap" 
                  << std::setw(8) << "Flow" 
                  << std::setw(10) << "ResCap" 
                  << std::setw(8) << "Cost" << "\n";
        std::cout << "   " << std::string(92, '-') << "\n";

        int printedEdges = 0;
        for (const auto& e : step.residualGraph) {
            if (e.capacity > 0 || e.residualCapacity > 0) {
                std::cout << "   " << std::left << std::setw(6) << e.from
                          << std::setw(6) << e.to
                          << std::setw(24) << e.fromName
                          << std::setw(24) << e.toName
                          << std::setw(8) << e.capacity
                          << std::setw(8) << e.flow
                          << std::setw(10) << e.residualCapacity
                          << std::setw(8) << e.cost << "\n";
                printedEdges++;
            }
        }
        std::cout << "   [" << printedEdges << " active residual edges shown]\n";
    }

    // PART B: EDMONDS-KARP TRACE
    std::cout << "\n----------------------------------------------------------------------------------------\n";
    std::cout << " PART B: EDMONDS-KARP BFS MAX-FLOW AUGMENTATIONS (SHORTEST HOP-COUNT PATHS)\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    for (const auto& ek : ekSteps) {
        std::cout << " [EK Step " << ek.iteration << "] Path: ";
        for (size_t i = 0; i < ek.pathNames.size(); ++i) {
            std::cout << ek.pathNames[i] << (i + 1 < ek.pathNames.size() ? " -> " : "");
        }
        std::cout << " | Delta: " << ek.bottleneckDelta << " req/s | Cumulative Flow: " << ek.cumulativeFlow << " req/s\n";
        std::cout << "   BFS Layers (Hop Depth): ";
        for (const auto& kv : ek.bfsLayers) {
            std::cout << "[Node " << kv.first << ": Layer " << kv.second << "] ";
        }
        std::cout << "\n";
    }

    // PART C: MIN-CUT BFS TRACE
    std::cout << "\n----------------------------------------------------------------------------------------\n";
    std::cout << " PART C: MIN-CUT RESIDUAL BFS EXTRACTION & THEOREM VERIFICATION\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << " Residual BFS Traversal Order from Source (S): ";
    for (size_t i = 0; i < minCutTrace.bfsTraversalOrder.size(); ++i) {
        std::cout << minCutTrace.bfsTraversalOrder[i] << (i + 1 < minCutTrace.bfsTraversalOrder.size() ? " -> " : "");
    }
    std::cout << "\n";

    std::cout << " S-Side Partition (Source-Reachable Nodes) [" << minCutTrace.sSideNodes.size() << " nodes]:\n   ";
    for (size_t i = 0; i < minCutTrace.sSideNodes.size(); ++i) {
        std::cout << minCutTrace.sSideNames[i] << " (" << minCutTrace.sSideNodes[i] << ") ";
    }
    std::cout << "\n";

    std::cout << " T-Side Partition (Sink-Disconnected Nodes) [" << minCutTrace.tSideNodes.size() << " nodes]:\n   ";
    for (size_t i = 0; i < minCutTrace.tSideNodes.size(); ++i) {
        std::cout << minCutTrace.tSideNames[i] << " (" << minCutTrace.tSideNodes[i] << ") ";
    }
    std::cout << "\n\n Saturated Cut Edges Crossing S -> T (" << minCutTrace.cutEdges.size() << " edges):\n";
    for (const auto& ce : minCutTrace.cutEdges) {
        std::cout << "   * " << ce.fromName << " (" << ce.from << ") -> " 
                  << ce.toName << " (" << ce.to << ") | Capacity: " << ce.capacity 
                  << " req/s | Flow: " << ce.flow << " req/s [SATURATED]\n";
    }
    std::cout << " Max-Flow Value:      " << minCutTrace.maxFlow << " req/s\n";
    std::cout << " Min-Cut Capacity:    " << minCutTrace.cutCapacity << " req/s\n";
    std::cout << " Theorem Assertion:   cutCapacity == maxFlow -> [" 
              << (minCutTrace.theoremVerified ? "PASS: MAX-FLOW MIN-CUT THEOREM HOLDS STRICTLY" : "FAIL") << "]\n";
    std::cout << "========================================================================================\n\n";
}
