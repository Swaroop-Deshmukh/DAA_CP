#ifndef CDN_GRAPH_HPP
#define CDN_GRAPH_HPP

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <cmath>
#include <map>

// ============================================================================
// Phase 3: CDN Flow Network Graph Data Structures (DAA Course Project)
// Prepares Residual Graph Representation for Phase 4 Min-Cost Max-Flow (MCMF)
// ============================================================================

/**
 * Node structure representing an entity in the CDN flow network.
 */
struct Node {
    int id;
    std::string name;
    std::string type; // "SOURCE", "EDGE_POP", "SINK"

    Node(int _id = 0, const std::string& _name = "", const std::string& _type = "")
        : id(_id), name(_name), type(_type) {}
};

/**
 * Edge structure representing a directed link in the residual flow network.
 * Stores capacity, flow, cost, and residual pointers.
 */
struct Edge {
    int from;           // Origin vertex ID
    int to;             // Destination vertex ID
    double capacity;    // Maximum capacity (req/s or Mbps)
    double flow;        // Current routed flow
    double cost;        // Unit routing cost per unit flow
    int rev_idx;        // Index of the corresponding reverse edge in adj[to]
    bool is_residual;   // True if this is a backward residual edge

    Edge(int u, int v, double cap, double c, int rev, bool residual = false)
        : from(u), to(v), capacity(cap), flow(0.0), cost(c), rev_idx(rev), is_residual(residual) {}

    // Residual capacity remaining on this edge
    double residual_capacity() const {
        return capacity - flow;
    }
};

/**
 * Server dynamic telemetry snapshot read from Redis or simulator.
 */
struct ServerState {
    std::string name;
    double capacity;
    double current_requests;
    double latency;           // In ms (or -1.0 / large if DOWN)
    double cpu;               // Percentage
    double bandwidth_usage;   // Mbps
    double bandwidth_capacity;// Mbps
    double bandwidth_cost;    // ₹/GB
    std::string status;       // "HEALTHY", "CONGESTED", "DOWN"
    std::string timestamp;
};

/**
 * Cost Configuration Parameters.
 * Total Cost = (alpha * Latency) + (beta * BandwidthCost)
 */
struct CostWeights {
    double alpha; // Latency weight (cost per ms)
    double beta;  // Bandwidth cost weight (cost per ₹/GB)

    CostWeights(double a = 1.0, double b = 2.0) : alpha(a), beta(b) {}
};

/**
 * CDN Flow Graph Manager.
 * Constructs and manages the flow network with residual edges.
 */
class CDNGraph {
public:
    int num_nodes;
    std::vector<Node> nodes;
    std::vector<std::vector<Edge>> adj;
    CostWeights cost_weights;

    // Standard node indices
    static const int SOURCE_ID = 0;
    static const int SINK_ID = 5;

    CDNGraph(int n = 6, CostWeights weights = CostWeights(1.0, 2.0))
        : num_nodes(n), cost_weights(weights) {
        adj.resize(num_nodes);
        initializeNodes();
    }

    void initializeNodes() {
        nodes.clear();
        nodes.emplace_back(0, "Source (Traffic)", "SOURCE");
        nodes.emplace_back(1, "Pune", "EDGE_POP");
        nodes.emplace_back(2, "Mumbai", "EDGE_POP");
        nodes.emplace_back(3, "Delhi", "EDGE_POP");
        nodes.emplace_back(4, "Bangalore", "EDGE_POP");
        nodes.emplace_back(5, "Sink (Origin)", "SINK");
    }

    /**
     * Requirement 10 & 11: AddEdge()
     * Adds forward edge (u -> v) with capacity C and cost W.
     * Adds backward residual edge (v -> u) with capacity 0 and cost -W.
     */
    void addEdge(int u, int v, double capacity, double cost) {
        Edge forward_edge(u, v, capacity, cost, (int)adj[v].size(), false);
        Edge backward_edge(v, u, 0.0, -cost, (int)adj[u].size(), true);

        adj[u].push_back(forward_edge);
        adj[v].push_back(backward_edge);
    }

    /**
     * Calculates the composite routing cost for an edge server.
     * Formula: Cost = alpha * Latency + beta * BandwidthCost
     */
    double calculateRoutingCost(double latency_ms, double bw_cost_per_gb, bool is_down) const {
        if (is_down) {
            return 9999.0; // High barrier for down servers
        }
        return (cost_weights.alpha * latency_ms) + (cost_weights.beta * bw_cost_per_gb);
    }

    /**
     * Builds the flow graph dynamically based on current edge server states.
     *
     * Topology:
     *   Source (0) ---> Edge Servers (1..4) ---> Sink / Origin (5)
     */
    void buildFromState(const std::vector<ServerState>& servers) {
        // Clear previous edges while preserving node metadata
        for (int i = 0; i < num_nodes; i++) {
            adj[i].clear();
        }

        // Map server name to node ID
        std::map<std::string, int> server_node_map = {
            {"Pune", 1}, {"pune", 1},
            {"Mumbai", 2}, {"mumbai", 2},
            {"Delhi", 3}, {"delhi", 3},
            {"Bangalore", 4}, {"bangalore", 4}
        };

        for (const auto& s : servers) {
            auto it = server_node_map.find(s.name);
            if (it == server_node_map.end()) continue;

            int edge_node_id = it->second;
            bool is_down = (s.status == "DOWN");

            // 1. Source -> Edge Server
            // Capacity = Server's processing capacity (0 if DOWN)
            // Cost = dynamic routing cost based on latency & bandwidth cost
            double edge_capacity = is_down ? 0.0 : s.capacity;
            double routing_cost = calculateRoutingCost(s.latency, s.bandwidth_cost, is_down);

            addEdge(SOURCE_ID, edge_node_id, edge_capacity, routing_cost);

            // 2. Edge Server -> Origin Sink
            // Capacity = Backhaul egress capacity (e.g. 5000 req/s or bandwidth capacity)
            // Cost = Minimal origin fetch transit cost (e.g. 5.0)
            double backhaul_cap = is_down ? 0.0 : s.capacity;
            double backhaul_cost = 5.0; // Constant nominal backhaul link cost

            addEdge(edge_node_id, SINK_ID, backhaul_cap, backhaul_cost);
        }
    }

    /**
     * Pretty prints the graph with both forward and residual edges.
     */
    void printGraph() const {
        std::cout << "\n========================================================================================\n";
        std::cout << "                 CDN FLOW NETWORK GRAPH (Phase 3 - Graph Construction)                  \n";
        std::cout << "========================================================================================\n";
        std::cout << " Cost Formula: Cost = (" << cost_weights.alpha << " * Latency) + (" << cost_weights.beta << " * BandwidthCost)\n";
        std::cout << " Nodes: " << num_nodes << " [0: Source, 1: Pune, 2: Mumbai, 3: Delhi, 4: Bangalore, 5: Sink]\n";
        std::cout << "----------------------------------------------------------------------------------------\n";
        std::cout << std::left
                  << std::setw(18) << "From Node"
                  << std::setw(18) << "To Node"
                  << std::setw(12) << "Capacity"
                  << std::setw(10) << "Flow"
                  << std::setw(14) << "Residual Cap"
                  << std::setw(10) << "Cost"
                  << std::setw(12) << "Edge Type"
                  << "\n";
        std::cout << "----------------------------------------------------------------------------------------\n";

        for (int u = 0; u < num_nodes; u++) {
            for (const auto& e : adj[u]) {
                std::string from_name = nodes[e.from].name;
                std::string to_name = nodes[e.to].name;
                std::string edge_type = e.is_residual ? "BACKWARD" : "FORWARD";

                std::cout << std::left
                          << std::setw(18) << (from_name + " (" + std::to_string(e.from) + ")")
                          << std::setw(18) << (to_name + " (" + std::to_string(e.to) + ")")
                          << std::setw(12) << std::fixed << std::setprecision(1) << e.capacity
                          << std::setw(10) << std::fixed << std::setprecision(1) << e.flow
                          << std::setw(14) << std::fixed << std::setprecision(1) << e.residual_capacity()
                          << std::setw(10) << std::fixed << std::setprecision(2) << e.cost
                          << std::setw(12) << edge_type
                          << "\n";
            }
        }
        std::cout << "========================================================================================\n\n";
    }
};

#endif // CDN_GRAPH_HPP
