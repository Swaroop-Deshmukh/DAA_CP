#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include "CDNServer.h"

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
 * Directed Flow Network Graph for CDN traffic routing.
 * Topology:
 *   Node 0: Source (User Traffic)
 *   Node 1: Pune
 *   Node 2: Mumbai
 *   Node 3: Delhi
 *   Node 4: Bangalore
 *   Node 5: Sink (Origin Data Center)
 */
class Graph {
private:
    int nodeCount;
    std::vector<std::vector<Edge>> adj;
    std::vector<std::string> nodeNames;

    // Configurable routing cost weights
    double latencyWeight;
    double bandwidthWeight;
    int costScaleFactor; // Fixed-point scaling factor (e.g. 100) for exact integer DAA arithmetic

public:
    static const int SOURCE_NODE = 0;
    static const int SINK_NODE = 5;

    Graph(int nodes = 6, double latWeight = 1.0, double bwWeight = 2.0, int scaleFactor = 100);

    // DAA Graph Operations
    void addEdge(int from, int to, int capacity, int cost);
    void buildFromServers(const std::vector<CDNServer>& servers);
    int calculateScaledCost(double latency, double bwCost, bool isFailed = false) const;
    void clearEdges();

    // Getters
    int getNodeCount() const { return nodeCount; }
    const std::vector<Edge>& getEdges(int u) const { return adj[u]; }
    std::vector<Edge>& getEdges(int u) { return adj[u]; }
    const std::vector<std::vector<Edge>>& getAdjacencyList() const { return adj; }
    std::string getNodeName(int u) const;
    double getLatencyWeight() const { return latencyWeight; }
    double getBandwidthWeight() const { return bandwidthWeight; }
    int getCostScaleFactor() const { return costScaleFactor; }

    void printGraph() const;
};

#endif // GRAPH_H
