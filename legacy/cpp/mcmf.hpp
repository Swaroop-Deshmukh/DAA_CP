#ifndef MCMF_HPP
#define MCMF_HPP

#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <map>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <limits>
#include "cdn_graph.hpp"

// ============================================================================
// Phase 4: Min-Cost Max-Flow (MCMF) Engine (DAA Implementation in C++)
// Algorithm: Successive Shortest Path (SSP) with Dijkstra & Node Potentials
// ============================================================================

/**
 * Edge representation in the MCMF residual network.
 */
struct MCMFEdge {
    int from;           // Tail node
    int to;             // Head node
    double capacity;    // Maximum capacity limit
    double flow;        // Flow currently assigned
    double cost;        // Cost per unit of flow
    int rev_idx;        // Index of reverse edge in adj[to]
    bool is_residual;   // True if this is an artificial backward edge

    MCMFEdge(int u, int v, double cap, double c, int rev, bool residual = false)
        : from(u), to(v), capacity(cap), flow(0.0), cost(c), rev_idx(rev), is_residual(residual) {}

    // Residual capacity remaining on this edge
    double residual_capacity() const {
        return capacity - flow;
    }
};

/**
 * Result metrics returned by the MCMF Engine.
 */
struct MCMFResult {
    double requested_traffic;               // Total demand requested (req/s)
    double routed_traffic;                  // Total flow successfully routed (req/s)
    double total_cost;                      // Minimum total routing cost
    int num_augmentations;                  // Number of augmenting paths pushed
    double execution_time_ms;               // Wall-clock algorithmic execution time
    std::map<std::string, double> allocation; // Traffic routed to each edge server
    std::map<std::string, double> server_cost; // Cost incurred by each server
    bool fully_routed;                      // True if requested_traffic == routed_traffic
};

// Global algorithmic constants
static const double MCMF_INF = 1e18;
static const double MCMF_EPS = 1e-9;

/**
 * Min-Cost Max-Flow Engine.
 * 
 * Uses Successive Shortest Path with Dijkstra + Johnson's Node Potentials:
 * - Solves the negative-cycle / negative-edge issue on residual backward edges.
 * - Reduced cost: c^pi(u, v) = c(u, v) + pi(u) - pi(v) >= 0.
 * - Uses std::priority_queue for O((E log V) * Augmentations) time complexity.
 */
class MinCostMaxFlow {
public:
    int num_nodes;
    std::vector<std::vector<MCMFEdge>> adj;
    std::vector<double> potential;  // Node potentials pi(v)
    std::vector<double> dist;       // Shortest path distances from source
    std::vector<int> parent_node;   // Parent node on shortest path
    std::vector<int> parent_edge;   // Edge index on shortest path
    std::vector<std::string> node_names;

    MinCostMaxFlow(int n = 6) : num_nodes(n) {
        adj.resize(n);
        potential.assign(n, 0.0);
        dist.resize(n);
        parent_node.resize(n);
        parent_edge.resize(n);
        node_names = {"Source", "Pune", "Mumbai", "Delhi", "Bangalore", "Sink"};
    }

    /**
     * Adds directed forward edge and backward residual edge.
     */
    void addEdge(int from, int to, double capacity, double cost) {
        MCMFEdge forward_edge(from, to, capacity, cost, (int)adj[to].size(), false);
        MCMFEdge backward_edge(to, from, 0.0, -cost, (int)adj[from].size(), true);

        adj[from].push_back(forward_edge);
        adj[to].push_back(backward_edge);
    }

    /**
     * Dijkstra's algorithm using reduced costs:
     * c_pi(u, v) = cost(u, v) + potential[u] - potential[v]
     * Invariant: c_pi(u, v) >= 0 for all edges with residual_capacity > EPS.
     */
    bool dijkstra(int source, int sink) {
        dist.assign(num_nodes, MCMF_INF);
        parent_node.assign(num_nodes, -1);
        parent_edge.assign(num_nodes, -1);

        // Min-heap storing pairs of (distance, node_id)
        typedef std::pair<double, int> PDI;
        std::priority_queue<PDI, std::vector<PDI>, std::greater<PDI>> pq;

        dist[source] = 0.0;
        pq.push({0.0, source});

        while (!pq.empty()) {
            auto top = pq.top();
            pq.pop();

            double d = top.first;
            int u = top.second;

            if (d > dist[u] + MCMF_EPS) continue;

            for (int i = 0; i < (int)adj[u].size(); i++) {
                const auto& edge = adj[u][i];
                if (edge.residual_capacity() <= MCMF_EPS) continue; // No remaining capacity

                // Reduced cost formula
                double reduced_cost = edge.cost + potential[u] - potential[edge.to];

                // Precision check
                if (reduced_cost < -1e-6) {
                    reduced_cost = 0.0; // Rounding tolerance
                }

                if (dist[edge.to] > dist[u] + reduced_cost + MCMF_EPS) {
                    dist[edge.to] = dist[u] + reduced_cost;
                    parent_node[edge.to] = u;
                    parent_edge[edge.to] = i;
                    pq.push({dist[edge.to], edge.to});
                }
            }
        }

        // Return true if an augmenting path reached the sink
        return dist[sink] < MCMF_INF - 1000.0;
    }

    /**
     * Solves Min-Cost Max-Flow for a given requested traffic demand.
     * 
     * @param source Source node ID (0)
     * @param sink Sink node ID (5)
     * @param target_traffic Traffic demand to satisfy (req/s)
     * @return MCMFResult containing routed traffic, cost, allocation, and stats
     */
    MCMFResult solve(int source, int sink, double target_traffic) {
        auto start_time = std::chrono::high_resolution_clock::now();

        double total_flow = 0.0;
        double total_cost = 0.0;
        int augmentations = 0;

        // Initialize node potentials
        potential.assign(num_nodes, 0.0);

        // Loop: Find minimum-cost augmenting paths using Dijkstra with potentials
        while (total_flow < target_traffic - MCMF_EPS && dijkstra(source, sink)) {
            // Update node potentials to preserve non-negative reduced costs
            for (int i = 0; i < num_nodes; i++) {
                if (dist[i] < MCMF_INF - 1000.0) {
                    potential[i] += dist[i];
                }
            }

            // Step 1: Find the bottleneck capacity along the augmenting path
            double bottleneck = target_traffic - total_flow;
            for (int curr = sink; curr != source; curr = parent_node[curr]) {
                int p = parent_node[curr];
                int edge_idx = parent_edge[curr];
                bottleneck = std::min(bottleneck, adj[p][edge_idx].residual_capacity());
            }

            if (bottleneck <= MCMF_EPS) break;

            // Step 2: Augment flow along the path and update residual edges
            for (int curr = sink; curr != source; curr = parent_node[curr]) {
                int p = parent_node[curr];
                int edge_idx = parent_edge[curr];
                int rev_idx = adj[p][edge_idx].rev_idx;

                adj[p][edge_idx].flow += bottleneck;
                adj[curr][rev_idx].flow -= bottleneck;
                total_cost += bottleneck * adj[p][edge_idx].cost;
            }

            total_flow += bottleneck;
            augmentations++;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end_time - start_time;

        // Extract per-server allocation
        std::map<std::string, double> allocation;
        std::map<std::string, double> server_cost;
        std::map<int, std::string> node_to_city = {
            {1, "Pune"}, {2, "Mumbai"}, {3, "Delhi"}, {4, "Bangalore"}
        };

        for (const auto& edge : adj[source]) {
            if (!edge.is_residual && node_to_city.count(edge.to)) {
                std::string city = node_to_city[edge.to];
                allocation[city] = edge.flow;
                server_cost[city] = edge.flow * edge.cost;
            }
        }

        MCMFResult result;
        result.requested_traffic = target_traffic;
        result.routed_traffic = total_flow;
        result.total_cost = total_cost;
        result.num_augmentations = augmentations;
        result.execution_time_ms = duration.count();
        result.allocation = allocation;
        result.server_cost = server_cost;
        result.fully_routed = (std::abs(total_flow - target_traffic) <= MCMF_EPS);

        return result;
    }

    /**
     * Helper to build graph from ServerState vector.
     */
    void buildFromState(const std::vector<ServerState>& servers, double alpha = 1.0, double beta = 2.0) {
        for (int i = 0; i < num_nodes; i++) adj[i].clear();

        std::map<std::string, int> city_map = {
            {"Pune", 1}, {"pune", 1},
            {"Mumbai", 2}, {"mumbai", 2},
            {"Delhi", 3}, {"delhi", 3},
            {"Bangalore", 4}, {"bangalore", 4}
        };

        for (const auto& s : servers) {
            if (!city_map.count(s.name)) continue;
            int u = city_map[s.name];

            bool is_down = (s.status == "DOWN");
            double cap = is_down ? 0.0 : s.capacity;
            double cost = is_down ? 9999.0 : (alpha * s.latency + beta * s.bandwidth_cost);

            // Source (0) -> Edge Server (u)
            addEdge(0, u, cap, cost);
            // Edge Server (u) -> Sink (5)
            addEdge(u, 5, cap, 0.0);
        }
    }
};

#endif // MCMF_HPP
