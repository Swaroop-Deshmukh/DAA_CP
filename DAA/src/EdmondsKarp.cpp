#include "EdmondsKarp.h"
#include <chrono>
#include <iomanip>
#include <algorithm>

EdmondsKarp::EdmondsKarp(int n)
    : nodeCount(n),
      parentNode(n, -1),
      parentEdge(n, -1)
{}

bool EdmondsKarp::bfs(Graph& graph, int source, int sink) {
    if (graph.getNodeCount() != nodeCount) {
        nodeCount = graph.getNodeCount();
        parentNode.assign(nodeCount, -1);
        parentEdge.assign(nodeCount, -1);
    } else {
        std::fill(parentNode.begin(), parentNode.end(), -1);
        std::fill(parentEdge.begin(), parentEdge.end(), -1);
    }

    std::queue<int> q;
    q.push(source);
    parentNode[source] = source;

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        if (u == sink) {
            return true;
        }

        const auto& edges = graph.getEdges(u);
        for (int i = 0; i < (int)edges.size(); ++i) {
            const auto& edge = edges[i];
            if (edge.residualCapacity() > 0 && parentNode[edge.to] == -1) {
                parentNode[edge.to] = u;
                parentEdge[edge.to] = i;
                q.push(edge.to);
            }
        }
    }

    return parentNode[sink] != -1;
}

bool EdmondsKarp::bfsWithLayers(Graph& graph, int source, int sink, std::map<int, int>& outLayers) {
    if (graph.getNodeCount() != nodeCount) {
        nodeCount = graph.getNodeCount();
        parentNode.assign(nodeCount, -1);
        parentEdge.assign(nodeCount, -1);
    } else {
        std::fill(parentNode.begin(), parentNode.end(), -1);
        std::fill(parentEdge.begin(), parentEdge.end(), -1);
    }

    outLayers.clear();
    std::queue<int> q;
    q.push(source);
    parentNode[source] = source;
    outLayers[source] = 0;

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        int currentLayer = outLayers[u];

        if (u == sink) {
            return true;
        }

        const auto& edges = graph.getEdges(u);
        for (int i = 0; i < (int)edges.size(); ++i) {
            const auto& edge = edges[i];
            if (edge.residualCapacity() > 0 && parentNode[edge.to] == -1) {
                parentNode[edge.to] = u;
                parentEdge[edge.to] = i;
                outLayers[edge.to] = currentLayer + 1;
                q.push(edge.to);
            }
        }
    }

    return parentNode[sink] != -1;
}

int EdmondsKarp::solveMaxFlow(Graph& graph, int source, int sink, int maxDemand) {
    if (graph.getNodeCount() != nodeCount) {
        nodeCount = graph.getNodeCount();
        parentNode.assign(nodeCount, -1);
        parentEdge.assign(nodeCount, -1);
    }

    int totalFlow = 0;

    while ((maxDemand < 0 || totalFlow < maxDemand) && bfs(graph, source, sink)) {
        // Step 1: Find bottleneck capacity along the shortest augmenting path
        int bottleneck = (maxDemand > 0) ? (maxDemand - totalFlow) : 2147483647;
        for (int curr = sink; curr != source; curr = parentNode[curr]) {
            int p = parentNode[curr];
            int edgeIdx = parentEdge[curr];
            bottleneck = std::min(bottleneck, graph.getEdges(p)[edgeIdx].residualCapacity());
        }

        if (bottleneck <= 0) break;

        // Step 2: Augment flow along forward and reverse residual edges
        for (int curr = sink; curr != source; curr = parentNode[curr]) {
            int p = parentNode[curr];
            int edgeIdx = parentEdge[curr];
            Edge& fwdEdge = graph.getEdges(p)[edgeIdx];
            Edge& revEdge = graph.getEdges(curr)[fwdEdge.reverseIndex];

            fwdEdge.flow += bottleneck;
            revEdge.flow -= bottleneck;
        }

        totalFlow += bottleneck;
    }

    return totalFlow;
}

int EdmondsKarp::solveMaxFlowWithTrace(Graph& graph, int source, int sink, int maxDemand, AlgorithmTraceReport& traceReport) {
    if (graph.getNodeCount() != nodeCount) {
        nodeCount = graph.getNodeCount();
        parentNode.assign(nodeCount, -1);
        parentEdge.assign(nodeCount, -1);
    }

    int totalFlow = 0;
    int iteration = 0;
    traceReport.ekSteps.clear();

    std::map<int, int> layers;
    while ((maxDemand < 0 || totalFlow < maxDemand) && bfsWithLayers(graph, source, sink, layers)) {
        iteration++;
        EKStepTrace ekStep;
        ekStep.iteration = iteration;
        ekStep.bfsLayers = layers;

        // Extract path
        std::vector<int> pathRev;
        for (int curr = sink; curr != -1; curr = parentNode[curr]) {
            pathRev.push_back(curr);
            if (curr == source) break;
        }
        std::vector<int> path(pathRev.rbegin(), pathRev.rend());
        ekStep.pathNodes = path;
        for (int u : path) {
            ekStep.pathNames.push_back(graph.getNodeName(u));
        }

        // Bottleneck
        int bottleneck = (maxDemand > 0) ? (maxDemand - totalFlow) : 2147483647;
        for (int curr = sink; curr != source; curr = parentNode[curr]) {
            int p = parentNode[curr];
            int edgeIdx = parentEdge[curr];
            bottleneck = std::min(bottleneck, graph.getEdges(p)[edgeIdx].residualCapacity());
        }

        if (bottleneck <= 0) break;

        // Augment flow
        for (int curr = sink; curr != source; curr = parentNode[curr]) {
            int p = parentNode[curr];
            int edgeIdx = parentEdge[curr];
            Edge& fwdEdge = graph.getEdges(p)[edgeIdx];
            Edge& revEdge = graph.getEdges(curr)[fwdEdge.reverseIndex];

            fwdEdge.flow += bottleneck;
            revEdge.flow -= bottleneck;
        }

        totalFlow += bottleneck;
        ekStep.bottleneckDelta = bottleneck;
        ekStep.cumulativeFlow = totalFlow;

        traceReport.ekSteps.push_back(ekStep);
    }

    return totalFlow;
}

MinCutResult EdmondsKarp::extractMinCut(Graph& graph, int source, int sink) {
    auto startTime = std::chrono::high_resolution_clock::now();

    MinCutResult res;
    nodeCount = graph.getNodeCount();

    // 1. Calculate Maximum Flow using Edmonds-Karp
    graph.resetFlows();
    res.maxFlow = solveMaxFlow(graph, source, sink);

    // 2. Perform BFS on the residual graph starting from Source (S)
    // Only traverse edges with positive residual capacity: e.residualCapacity() > 0
    std::vector<bool> visited(nodeCount, false);
    std::queue<int> q;
    q.push(source);
    visited[source] = true;

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        for (const auto& e : graph.getEdges(u)) {
            if (e.residualCapacity() > 0 && !visited[e.to]) {
                visited[e.to] = true;
                q.push(e.to);
            }
        }
    }

    // Partition nodes into S-side and T-side
    for (int i = 0; i < nodeCount; ++i) {
        if (visited[i]) {
            res.sSideNodes.push_back(i);
        } else {
            res.tSideNodes.push_back(i);
        }
    }

    // 3. Find all forward edges crossing from S-side to T-side
    res.cutCapacity = 0;
    for (int u : res.sSideNodes) {
        for (const auto& e : graph.getEdges(u)) {
            // Forward edge crossing cut: u in S, e.to in T
            if (!visited[e.to] && e.capacity > 0) {
                CutEdge ce;
                ce.from = u;
                ce.to = e.to;
                ce.fromName = graph.getNodeName(u);
                ce.toName = graph.getNodeName(e.to);
                ce.capacity = e.capacity;
                ce.flow = e.flow;
                res.cutEdges.push_back(ce);
                res.cutCapacity += e.capacity;

                // Capacity upgrade recommendation
                std::string rec = "Bottleneck Edge: " + ce.fromName + " -> " + ce.toName +
                                  " (Saturated at " + std::to_string(ce.capacity) + " req/s). " +
                                  "Recommendation: Expand bandwidth/capacity at " + ce.toName + " by >= 25%.";
                res.bottleneckRecommendations.push_back(rec);
            }
        }
    }

    // 4. Assert Max-Flow Min-Cut Theorem
    res.minCutEqualsMaxFlow = (res.cutCapacity == res.maxFlow);

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> dur = endTime - startTime;
    res.executionTimeUs = dur.count();

    return res;
}

MinCutResult EdmondsKarp::extractMinCutWithTrace(Graph& graph, int source, int sink, AlgorithmTraceReport& traceReport) {
    auto startTime = std::chrono::high_resolution_clock::now();

    MinCutResult res;
    nodeCount = graph.getNodeCount();

    // 1. Calculate Maximum Flow using Edmonds-Karp with trace
    graph.resetFlows();
    res.maxFlow = solveMaxFlowWithTrace(graph, source, sink, -1, traceReport);

    // 2. Perform BFS on the residual graph starting from Source (S)
    std::vector<bool> visited(nodeCount, false);
    std::queue<int> q;
    q.push(source);
    visited[source] = true;

    MinCutStepTrace mcTrace;
    mcTrace.bfsTraversalOrder.push_back(source);

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        for (const auto& e : graph.getEdges(u)) {
            if (e.residualCapacity() > 0 && !visited[e.to]) {
                visited[e.to] = true;
                mcTrace.bfsTraversalOrder.push_back(e.to);
                q.push(e.to);
            }
        }
    }

    // Partition nodes into S-side and T-side
    for (int i = 0; i < nodeCount; ++i) {
        if (visited[i]) {
            res.sSideNodes.push_back(i);
            mcTrace.sSideNodes.push_back(i);
            mcTrace.sSideNames.push_back(graph.getNodeName(i));
        } else {
            res.tSideNodes.push_back(i);
            mcTrace.tSideNodes.push_back(i);
            mcTrace.tSideNames.push_back(graph.getNodeName(i));
        }
    }

    // 3. Find all forward edges crossing from S-side to T-side
    res.cutCapacity = 0;
    for (int u : res.sSideNodes) {
        for (const auto& e : graph.getEdges(u)) {
            if (!visited[e.to] && e.capacity > 0) {
                CutEdge ce;
                ce.from = u;
                ce.to = e.to;
                ce.fromName = graph.getNodeName(u);
                ce.toName = graph.getNodeName(e.to);
                ce.capacity = e.capacity;
                ce.flow = e.flow;
                res.cutEdges.push_back(ce);
                res.cutCapacity += e.capacity;

                ResidualEdgeSnapshot snap;
                snap.from = u;
                snap.to = e.to;
                snap.fromName = ce.fromName;
                snap.toName = ce.toName;
                snap.capacity = e.capacity;
                snap.flow = e.flow;
                snap.residualCapacity = e.residualCapacity();
                snap.cost = e.cost;
                mcTrace.cutEdges.push_back(snap);

                std::string rec = "Bottleneck Edge: " + ce.fromName + " -> " + ce.toName +
                                  " (Saturated at " + std::to_string(ce.capacity) + " req/s). " +
                                  "Recommendation: Expand bandwidth/capacity at " + ce.toName + " by >= 25%.";
                res.bottleneckRecommendations.push_back(rec);
            }
        }
    }

    // 4. Assert Max-Flow Min-Cut Theorem
    res.minCutEqualsMaxFlow = (res.cutCapacity == res.maxFlow);
    mcTrace.maxFlow = res.maxFlow;
    mcTrace.cutCapacity = res.cutCapacity;
    mcTrace.theoremVerified = res.minCutEqualsMaxFlow;
    traceReport.minCutTrace = mcTrace;

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> dur = endTime - startTime;
    res.executionTimeUs = dur.count();

    return res;
}

void MinCutResult::printReport() const {
    std::cout << "\n========================================================================================\n";
    std::cout << "                  MAX-FLOW MIN-CUT THEOREM EXTRACTION REPORT                            \n";
    std::cout << "========================================================================================\n";
    std::cout << " Maximum Flow Value:    " << maxFlow << " req/s\n";
    std::cout << " Minimum Cut Capacity:  " << cutCapacity << " req/s\n";
    std::cout << " Theorem Assertion:     cutCapacity == maxFlow -> ["
              << (minCutEqualsMaxFlow ? "PASS: EQUALITY HOLDS STRICTLY" : "FAIL") << "]\n";
    std::cout << " Execution Time:        " << std::fixed << std::setprecision(2) << executionTimeUs << " microseconds\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    std::cout << " [S-SIDE PARTITION (Source-Reachable in Residual Graph)] (" << sSideNodes.size() << " nodes):\n   ";
    for (int u : sSideNodes) {
        std::cout << "[" << u << "] ";
    }
    std::cout << "\n\n [T-SIDE PARTITION (Sink-Side Saturated Disconnected)] (" << tSideNodes.size() << " nodes):\n   ";
    for (int v : tSideNodes) {
        std::cout << "[" << v << "] ";
    }
    std::cout << "\n----------------------------------------------------------------------------------------\n";

    std::cout << " [SATURATED CUT EDGES CROSSING THE MIN-CUT (S -> T)] (" << cutEdges.size() << " edges):\n";
    std::cout << std::left
              << std::setw(24) << "From Node (S-Side)"
              << std::setw(24) << "To Node (T-Side)"
              << std::setw(15) << "Capacity"
              << std::setw(12) << "Routed Flow"
              << std::setw(15) << "Status"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    for (const auto& ce : cutEdges) {
        std::cout << std::left
                  << std::setw(24) << ce.fromName
                  << std::setw(24) << ce.toName
                  << std::setw(15) << (std::to_string(ce.capacity) + " req/s")
                  << std::setw(12) << (std::to_string(ce.flow) + " req/s")
                  << std::setw(15) << (ce.flow == ce.capacity ? "SATURATED [CUT]" : "SLACK")
                  << "\n";
    }
    std::cout << "----------------------------------------------------------------------------------------\n";

    std::cout << " [INFRASTRUCTURE CAPACITY UPGRADE RECOMMENDATIONS]:\n";
    for (size_t i = 0; i < bottleneckRecommendations.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << bottleneckRecommendations[i] << "\n";
    }
    std::cout << "========================================================================================\n\n";
}
