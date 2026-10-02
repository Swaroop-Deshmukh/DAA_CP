#include "Graph.h"
#include <cmath>
#include <map>

Graph::Graph(int nodes, double latWeight, double bwWeight, int scaleFactor)
    : nodeCount(nodes),
      latencyWeight(latWeight),
      bandwidthWeight(bwWeight),
      costScaleFactor(scaleFactor)
{
    adj.resize(nodeCount);
    nodeNames = {
        "Source (User)",
        "Pune",
        "Mumbai",
        "Delhi",
        "Bangalore",
        "Sink (Origin)"
    };
}

void Graph::clearEdges() {
    for (int i = 0; i < nodeCount; i++) {
        adj[i].clear();
    }
}

void Graph::resetFlows() {
    for (int i = 0; i < nodeCount; i++) {
        for (auto& edge : adj[i]) {
            edge.flow = 0;
        }
    }
}

/**
 * Requirement 10: Implement AddEdge().
 * Inserts forward edge (u -> v) with capacity C and cost W.
 * Inserts reverse residual edge (v -> u) with capacity 0 and cost -W.
 */
void Graph::addEdge(int from, int to, int capacity, int cost) {
    Edge forwardEdge{
        to,
        (int)adj[to].size(),
        capacity,
        cost,
        0
    };

    Edge backwardEdge{
        from,
        (int)adj[from].size(),
        0,          // Backward residual capacity starts at 0
        -cost,      // Anti-symmetric cost: cost(v, u) = -cost(u, v)
        0
    };

    adj[from].push_back(forwardEdge);
    adj[to].push_back(backwardEdge);
}

/**
 * Requirement 4: Routing Cost Formula
 * cost = latencyWeight * latency + bandwidthWeight * bandwidthCost
 * 
 * Scaled Integer Decision:
 * Classic DAA network flow algorithms require integer costs to guarantee
 * strong polynomial termination and prevent floating-point precision error.
 * We multiply by costScaleFactor (100) to preserve 2 decimal places of precision.
 */
int Graph::calculateScaledCost(double latency, double bwCost, bool isFailed) const {
    if (isFailed) {
        return 999999; // Extreme barrier penalty for failed nodes
    }
    double rawCost = (latencyWeight * latency) + (bandwidthWeight * bwCost);
    return static_cast<int>(std::round(rawCost * costScaleFactor));
}

void Graph::buildFromServers(const std::vector<CDNServer>& servers) {
    clearEdges();

    std::map<std::string, int> serverToNode = {
        {"Pune", 1},
        {"Mumbai", 2},
        {"Delhi", 3},
        {"Bangalore", 4}
    };

    for (const auto& server : servers) {
        auto it = serverToNode.find(server.getName());
        if (it == serverToNode.end()) continue;

        int nodeIdx = it->second;
        bool isFailed = server.isFailed();

        // 1. Edge: Source (0) -> Edge Server
        // Capacity = Server's maximum capacity (clamped to 0 if FAILED)
        int edgeCap = isFailed ? 0 : static_cast<int>(server.getMaximumCapacity());
        int routingCost = calculateScaledCost(server.getCurrentLatency(), server.getBandwidthCost(), isFailed);
        addEdge(SOURCE_NODE, nodeIdx, edgeCap, routingCost);

        // 2. Edge: Edge Server -> Origin Sink (5)
        int backhaulCap = isFailed ? 0 : static_cast<int>(server.getMaximumCapacity());
        int backhaulCost = static_cast<int>(std::round(5.0 * costScaleFactor)); // Nominal transit to origin
        addEdge(nodeIdx, SINK_NODE, backhaulCap, backhaulCost);
    }
}

std::string Graph::getNodeName(int u) const {
    if (u >= 0 && u < (int)nodeNames.size()) {
        return nodeNames[u];
    }
    return "Node " + std::to_string(u);
}

void Graph::printGraph() const {
    std::cout << "\n========================================================================================\n";
    std::cout << "                 CDN FLOW NETWORK GRAPH (Adjacency List with Residuals)                 \n";
    std::cout << "========================================================================================\n";
    std::cout << " Cost Formula: Cost = (" << latencyWeight << " * Latency) + (" << bandwidthWeight << " * BandwidthCost)\n";
    std::cout << " Integer Cost Scale Factor: " << costScaleFactor << " (1 unit = " << (1.0 / costScaleFactor) << " Rs/unit)\n";
    std::cout << " Nodes: " << nodeCount << " [0: Source, 1: Pune, 2: Mumbai, 3: Delhi, 4: Bangalore, 5: Sink]\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(20) << "From Node"
              << std::setw(20) << "To Node"
              << std::setw(12) << "Capacity"
              << std::setw(10) << "Flow"
              << std::setw(14) << "Residual Cap"
              << std::setw(12) << "Cost (Scaled)"
              << std::setw(10) << "Edge Type"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    for (int u = 0; u < nodeCount; u++) {
        for (const auto& e : adj[u]) {
            std::string fromStr = getNodeName(u) + " (" + std::to_string(u) + ")";
            std::string toStr = getNodeName(e.to) + " (" + std::to_string(e.to) + ")";
            std::string typeStr = (e.capacity == 0) ? "BACKWARD" : "FORWARD";

            std::cout << std::left
                      << std::setw(20) << fromStr
                      << std::setw(20) << toStr
                      << std::setw(12) << e.capacity
                      << std::setw(10) << e.flow
                      << std::setw(14) << e.residualCapacity()
                      << std::setw(12) << e.cost
                      << std::setw(10) << typeStr
                      << "\n";
        }
    }
    std::cout << "========================================================================================\n\n";
}
