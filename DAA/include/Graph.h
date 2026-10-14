#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <map>
#include "CDNServer.h"
#include "GraphConfig.h"

/**
 * Directed Edge representation in the flow network.
 * Supports residual graph traversal for Min-Cost Max-Flow.
 */
struct Edge {
    int to;             // Head vertex (destination)
    int reverseIndex;   // Index of reverse edge in adj[to] for O(1) residual lookup
    int capacity;       // Maximum flow capacity (req/s)
    int cost;           // Scaled integer routing cost per unit of flow
    int flow;           // Current routed flow

    // Residual capacity remaining on this edge
    int residualCapacity() const {
        return capacity - flow;
    }
};

/**
 * Topology Type Enum:
 * - LEGACY_4_SERVER: Original 6-node network (Source -> 4 PoPs -> Sink)
 * - MULTI_REGION_CONVEX: General bipartite network with N regions, M edge servers,
 *                        parallel convex utilization bands, and Origin fallback
 */
enum class TopologyType {
    LEGACY_4_SERVER,
    MULTI_REGION_CONVEX
};

/**
 * Directed Flow Network Graph for CDN traffic routing.
 */
class Graph {
private:
    int nodeCount;
    std::vector<std::vector<Edge>> adj;
    std::vector<std::string> nodeNames;

    // Topology and specialized node IDs
    TopologyType topologyType;
    int sourceNode;
    int sinkNode;
    int originNode;

    // Mappings for multi-region graph
    std::map<std::string, int> regionNodeMap;
    std::map<std::string, int> serverNodeMap;
    std::map<int, std::string> nodeToRegionMap;
    std::map<int, std::string> nodeToServerMap;

    NetworkConfig activeConfig;

    // Configurable routing cost weights
    double latencyWeight;
    double bandwidthWeight;
    int costScaleFactor; // Fixed-point scaling factor (e.g. 100) for exact integer DAA arithmetic

public:
    // Legacy static node constants (retained for backward compatibility)
    static const int SOURCE_NODE = 0;
    static const int SINK_NODE = 5;

    Graph(int nodes = 6, double latWeight = 1.0, double bwWeight = 2.0, int scaleFactor = 100);

    // DAA Graph Operations
    void addEdge(int from, int to, int capacity, int cost);
    void buildFromServers(const std::vector<CDNServer>& servers);
    void buildFromNetworkConfig(
        const NetworkConfig& config,
        const std::map<std::string, bool>& serverFailedMap = {}
    );

    int calculateScaledCost(double latency, double bwCost, bool isFailed = false) const;
    void clearEdges();
    void resetFlows();

    // Getters
    int getNodeCount() const { return nodeCount; }
    TopologyType getTopologyType() const { return topologyType; }
    int getSourceNode() const { return sourceNode; }
    int getSinkNode() const { return sinkNode; }
    int getOriginNode() const { return originNode; }
    void setSourceNode(int s) { sourceNode = s; }
    void setSinkNode(int t) { sinkNode = t; }
    const NetworkConfig& getNetworkConfig() const { return activeConfig; }

    const std::vector<Edge>& getEdges(int u) const { return adj[u]; }
    std::vector<Edge>& getEdges(int u) { return adj[u]; }
    const std::vector<std::vector<Edge>>& getAdjacencyList() const { return adj; }
    std::string getNodeName(int u) const;
    double getLatencyWeight() const { return latencyWeight; }
    double getBandwidthWeight() const { return bandwidthWeight; }
    int getCostScaleFactor() const { return costScaleFactor; }

    int getRegionNode(const std::string& name) const;
    int getServerNode(const std::string& name) const;

    // Detailed flow extraction for multi-region topologies
    int getOriginFlow() const;
    std::map<std::string, int> getRegionRoutedFlows() const;
    std::map<std::string, int> getServerRoutedFlows() const;
    std::map<std::pair<std::string, std::string>, int> getLinkFlows() const;

    void printGraph() const;
    void printMultiRegionFlowSummary() const;
};

#endif // GRAPH_H
