#include "Graph.h"
#include <cmath>
#include <map>
#include <iomanip>

Graph::Graph(int nodes, double latWeight, double bwWeight, int scaleFactor)
    : nodeCount(nodes),
      topologyType(TopologyType::LEGACY_4_SERVER),
      sourceNode(0),
      sinkNode(nodes > 0 ? nodes - 1 : 5),
      originNode(-1),
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
    if (from < 0 || from >= nodeCount || to < 0 || to >= nodeCount) {
        return;
    }

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

/**
 * Legacy 4-server topology builder (retained for backward compatibility).
 */
void Graph::buildFromServers(const std::vector<CDNServer>& servers) {
    topologyType = TopologyType::LEGACY_4_SERVER;
    sourceNode = 0;
    sinkNode = 5;
    originNode = -1;
    nodeCount = 6;

    adj.assign(nodeCount, std::vector<Edge>());
    nodeNames = {
        "Source (User)",
        "Pune",
        "Mumbai",
        "Delhi",
        "Bangalore",
        "Sink (Origin)"
    };

    regionNodeMap.clear();
    serverNodeMap.clear();
    nodeToRegionMap.clear();
    nodeToServerMap.clear();

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
        int edgeCap = isFailed ? 0 : static_cast<int>(server.getMaximumCapacity());
        int routingCost = calculateScaledCost(server.getCurrentLatency(), server.getBandwidthCost(), isFailed);
        addEdge(sourceNode, nodeIdx, edgeCap, routingCost);

        // 2. Edge: Edge Server -> Origin Sink (5)
        int backhaulCap = isFailed ? 0 : static_cast<int>(server.getMaximumCapacity());
        int backhaulCost = static_cast<int>(std::round(5.0 * costScaleFactor));
        addEdge(nodeIdx, sinkNode, backhaulCap, backhaulCost);
    }
}

/**
 * Phase 1: Build non-trivial flow network from NetworkConfig:
 * 
 * S -> Region_i            (cap = demand_i, cost 0)
 * Region_i -> Edge_j       (cap = link bandwidth, cost = alpha*latency_ij + beta*bwCost_j, scaled)
 * Edge_j -> T (Bands)      (parallel edges for 0-50%, 50-80%, 80-95%, 95-100% convex queuing cost)
 * Region_i -> Origin -> T  (cap = INF, cost = very large penalty: unserved edge traffic)
 */
void Graph::buildFromNetworkConfig(
    const NetworkConfig& config,
    const std::map<std::string, bool>& serverFailedMap
) {
    topologyType = TopologyType::MULTI_REGION_CONVEX;
    activeConfig = config;
    latencyWeight = config.latencyWeight;
    bandwidthWeight = config.bandwidthWeight;
    costScaleFactor = config.costScaleFactor;

    int numRegions = static_cast<int>(config.regions.size());
    int numServers = static_cast<int>(config.servers.size());

    sourceNode = 0;
    originNode = 1 + numRegions + numServers;
    sinkNode = originNode + 1;
    nodeCount = sinkNode + 1;

    adj.assign(nodeCount, std::vector<Edge>());
    nodeNames.assign(nodeCount, "");

    regionNodeMap.clear();
    serverNodeMap.clear();
    nodeToRegionMap.clear();
    nodeToServerMap.clear();

    nodeNames[sourceNode] = "Source (User Ingress)";
    nodeNames[originNode] = "Origin (Fallback Center)";
    nodeNames[sinkNode] = "Sink (Terminal)";

    // Assign Region Node IDs: [1 .. numRegions]
    for (int i = 0; i < numRegions; ++i) {
        int rNode = 1 + i;
        const auto& r = config.regions[i];
        regionNodeMap[r.name] = rNode;
        nodeToRegionMap[rNode] = r.name;
        nodeNames[rNode] = "Reg:" + r.name;
    }

    // Assign Server Node IDs: [1 + numRegions .. numRegions + numServers]
    std::map<std::string, const EdgeServerConfig*> serverCfgMap;
    for (int j = 0; j < numServers; ++j) {
        int sNode = 1 + numRegions + j;
        const auto& s = config.servers[j];
        serverNodeMap[s.name] = sNode;
        nodeToServerMap[sNode] = s.name;
        nodeNames[sNode] = "Edge:" + s.name;
        serverCfgMap[s.name] = &s;
    }

    // 1. Source -> Region_i
    for (const auto& r : config.regions) {
        int rNode = regionNodeMap[r.name];
        addEdge(sourceNode, rNode, r.demand, 0);
    }

    // 2. Region_i -> Edge Server_j links
    for (const auto& link : config.links) {
        if (!regionNodeMap.count(link.fromRegion) || !serverNodeMap.count(link.toServer)) {
            continue;
        }
        int rNode = regionNodeMap[link.fromRegion];
        int sNode = serverNodeMap[link.toServer];
        const auto* sCfg = serverCfgMap[link.toServer];

        bool isFailed = false;
        auto failIt = serverFailedMap.find(link.toServer);
        if (failIt != serverFailedMap.end()) {
            isFailed = failIt->second;
        }

        int linkCap = isFailed ? 0 : link.capacity;
        int linkCost = calculateScaledCost(link.latency, sCfg->bandwidthCost, isFailed);
        addEdge(rNode, sNode, linkCap, linkCost);
    }

    // 3. Edge Server_j -> Sink via Parallel Convex Utilization Bands
    for (const auto& s : config.servers) {
        int sNode = serverNodeMap[s.name];
        bool isFailed = false;
        auto failIt = serverFailedMap.find(s.name);
        if (failIt != serverFailedMap.end()) {
            isFailed = failIt->second;
        }

        for (const auto& band : s.bands) {
            double bandFraction = band.upperPct - band.lowerPct;
            int bandCap = isFailed ? 0 : static_cast<int>(std::round(bandFraction * s.capacity));
            // Multiplier escalates the base transit cost (1.0x, 1.5x, 3.0x, 8.0x)
            double bandRawCost = config.baseTransitCost * band.costMultiplier;
            int bandCost = static_cast<int>(std::round(bandRawCost * costScaleFactor));
            addEdge(sNode, sinkNode, bandCap, bandCost);
        }
    }

    // 4. Region_i -> Origin -> Sink (Cache-miss / Overload fallback)
    if (config.enableOriginFallback) {
        for (const auto& r : config.regions) {
            int rNode = regionNodeMap[r.name];
            // Financial penalty + origin latency cost
            double rawOriginCost = (latencyWeight * r.originLatency) + r.originCost;
            int originCost = static_cast<int>(std::round(rawOriginCost * costScaleFactor));
            // Capacity = region demand (ensuring infinite fallback feasibility)
            addEdge(rNode, originNode, r.demand, originCost);
        }

        // Origin -> Sink has large infinite capacity and 0 marginal cost
        addEdge(originNode, sinkNode, 10000000, 0);
    }
}

int Graph::getRegionNode(const std::string& name) const {
    auto it = regionNodeMap.find(name);
    return (it != regionNodeMap.end()) ? it->second : -1;
}

int Graph::getServerNode(const std::string& name) const {
    auto it = serverNodeMap.find(name);
    return (it != serverNodeMap.end()) ? it->second : -1;
}

int Graph::getOriginFlow() const {
    if (originNode < 0 || originNode >= nodeCount) return 0;
    int flow = 0;
    for (const auto& e : adj[originNode]) {
        if (e.to == sinkNode && e.capacity > 0) {
            flow += e.flow;
        }
    }
    return flow;
}

std::map<std::string, int> Graph::getRegionRoutedFlows() const {
    std::map<std::string, int> flows;
    for (const auto& kv : regionNodeMap) {
        const std::string& rName = kv.first;
        int rNode = kv.second;
        int routedToEdge = 0;
        for (const auto& e : adj[rNode]) {
            // Count flow to edge servers only (exclude Origin)
            if (e.capacity > 0 && e.to != originNode && nodeToServerMap.count(e.to)) {
                routedToEdge += e.flow;
            }
        }
        flows[rName] = routedToEdge;
    }
    return flows;
}

std::map<std::string, int> Graph::getServerRoutedFlows() const {
    std::map<std::string, int> flows;
    for (const auto& kv : serverNodeMap) {
        const std::string& sName = kv.first;
        int sNode = kv.second;
        int totalServerFlow = 0;
        for (const auto& e : adj[sNode]) {
            if (e.to == sinkNode && e.capacity > 0) {
                totalServerFlow += e.flow;
            }
        }
        flows[sName] = totalServerFlow;
    }
    return flows;
}

std::map<std::pair<std::string, std::string>, int> Graph::getLinkFlows() const {
    std::map<std::pair<std::string, std::string>, int> flows;
    for (const auto& rkv : regionNodeMap) {
        const std::string& rName = rkv.first;
        int rNode = rkv.second;
        for (const auto& e : adj[rNode]) {
            if (e.capacity > 0 && nodeToServerMap.count(e.to)) {
                std::string sName = nodeToServerMap.at(e.to);
                flows[{rName, sName}] = e.flow;
            }
        }
    }
    return flows;
}

std::string Graph::getNodeName(int u) const {
    if (u >= 0 && u < (int)nodeNames.size() && !nodeNames[u].empty()) {
        return nodeNames[u];
    }
    return "Node " + std::to_string(u);
}

void Graph::printGraph() const {
    std::cout << "\n========================================================================================\n";
    std::cout << "                 CDN FLOW NETWORK GRAPH (Adjacency List with Residuals)                 \n";
    std::cout << "========================================================================================\n";
    std::cout << " Topology: " << (topologyType == TopologyType::MULTI_REGION_CONVEX ? "MULTI_REGION_CONVEX" : "LEGACY_4_SERVER") << "\n";
    std::cout << " Nodes: " << nodeCount << " | Source: " << sourceNode << " | Sink: " << sinkNode
              << (originNode >= 0 ? " | Origin: " + std::to_string(originNode) : "") << "\n";
    std::cout << " Cost Weights: alpha = " << latencyWeight << ", beta = " << bandwidthWeight
              << " | Scale Factor: " << costScaleFactor << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(24) << "From Node"
              << std::setw(24) << "To Node"
              << std::setw(12) << "Capacity"
              << std::setw(10) << "Flow"
              << std::setw(14) << "Residual Cap"
              << std::setw(14) << "Cost (Scaled)"
              << std::setw(10) << "Edge Type"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    for (int u = 0; u < nodeCount; u++) {
        for (const auto& e : adj[u]) {
            std::string fromStr = getNodeName(u) + " (" + std::to_string(u) + ")";
            std::string toStr = getNodeName(e.to) + " (" + std::to_string(e.to) + ")";
            std::string typeStr = (e.capacity == 0) ? "BACKWARD" : "FORWARD";

            std::cout << std::left
                      << std::setw(24) << fromStr
                      << std::setw(24) << toStr
                      << std::setw(12) << e.capacity
                      << std::setw(10) << e.flow
                      << std::setw(14) << e.residualCapacity()
                      << std::setw(14) << e.cost
                      << std::setw(10) << typeStr
                      << "\n";
        }
    }
    std::cout << "========================================================================================\n\n";
}

void Graph::printMultiRegionFlowSummary() const {
    if (topologyType != TopologyType::MULTI_REGION_CONVEX) {
        return;
    }

    int totalDemand = activeConfig.getTotalDemand();
    auto srvFlows = getServerRoutedFlows();
    int edgeRoutedTotal = 0;
    for (const auto& kv : srvFlows) edgeRoutedTotal += kv.second;
    int originFlow = getOriginFlow();

    std::cout << "\n========================================================================================\n";
    std::cout << "                  MULTI-REGION CDN TRAFFIC ROUTING SUMMARY                              \n";
    std::cout << "========================================================================================\n";
    std::cout << " Total Regional Ingress Demand: " << totalDemand << " req/s\n";
    std::cout << " Routed through Edge Fleet:     " << edgeRoutedTotal << " req/s ("
              << (totalDemand > 0 ? (edgeRoutedTotal * 100.0 / totalDemand) : 0.0) << "%)\n";
    std::cout << " Diverted to Origin Fallback:   " << originFlow << " req/s ("
              << (totalDemand > 0 ? (originFlow * 100.0 / totalDemand) : 0.0) << "%)\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    std::cout << " [EDGE SERVER POPS & CONVEX BAND UTILIZATION]:\n";
    for (const auto& s : activeConfig.servers) {
        int sNode = serverNodeMap.at(s.name);
        int flow = srvFlows[s.name];
        double utilPct = (s.capacity > 0) ? (flow * 100.0 / s.capacity) : 0.0;

        std::cout << "  * " << std::left << std::setw(14) << s.name
                  << " Total Routed: " << std::setw(6) << flow << " / " << std::setw(6) << s.capacity
                  << " (" << std::fixed << std::setprecision(1) << utilPct << "% util)\n";

        // Breakdown per convex band
        int bandIndex = 1;
        for (const auto& e : adj[sNode]) {
            if (e.to == sinkNode && e.capacity > 0) {
                double bandPct = (e.capacity > 0) ? (e.flow * 100.0 / e.capacity) : 0.0;
                std::cout << "      Band " << bandIndex++ << ": "
                          << std::setw(5) << e.flow << " / " << std::setw(5) << e.capacity
                          << " req/s (" << std::setprecision(1) << bandPct << "% saturated) | Cost: "
                          << (static_cast<double>(e.cost) / costScaleFactor) << " Rs/req\n";
            }
        }
    }

    std::cout << "\n [REGIONAL ROUTING DESTINATIONS]:\n";
    auto linkFlows = getLinkFlows();
    for (const auto& r : activeConfig.regions) {
        int rNode = regionNodeMap.at(r.name);
        std::cout << "  * " << std::left << std::setw(14) << r.name << " Demand: " << r.demand << " req/s\n";
        for (const auto& s : activeConfig.servers) {
            auto it = linkFlows.find({r.name, s.name});
            if (it != linkFlows.end() && it->second > 0) {
                std::cout << "      -> " << std::left << std::setw(12) << s.name
                          << ": " << it->second << " req/s\n";
            }
        }
        for (const auto& e : adj[rNode]) {
            if (e.to == originNode && e.capacity > 0 && e.flow > 0) {
                std::cout << "      -> " << std::left << std::setw(12) << "ORIGIN"
                          << ": " << e.flow << " req/s (FALLBACK)\n";
            }
        }
    }
    std::cout << "========================================================================================\n\n";
}
