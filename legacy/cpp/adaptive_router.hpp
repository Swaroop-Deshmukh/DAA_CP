#ifndef ADAPTIVE_ROUTER_HPP
#define ADAPTIVE_ROUTER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include "mcmf.hpp"

// ============================================================================
// Phase 5: Adaptive CDN Traffic Router (DAA Implementation in C++)
// Automatically detects Redis state changes, triggers MCMF re-routing,
// avoids unnecessary recalculations, and outputs JSON for React dashboards.
// ============================================================================

/**
 * Snapshot of a single server's live condition from Redis.
 */
struct ServerCondition {
    std::string name;
    double capacity;
    double current_requests;
    double latency;
    double cpu;
    double bandwidth_usage;
    double bandwidth_cost;
    std::string status;       // "HEALTHY", "CONGESTED", "DOWN"
    std::string timestamp;
};

/**
 * Detailed per-server traffic assignment.
 */
struct ServerAllocationItem {
    std::string server;
    double assigned_traffic;
    double capacity;
    double utilization_pct;
    double unit_cost;
    double subtotal_cost;
    double latency;
    std::string status;
};

/**
 * Structured Routing Decision consumed by React Dashboards (Phase 6).
 */
struct RoutingDecision {
    int cycle_id;
    std::string timestamp;
    double requested_traffic;
    double routed_traffic;
    double unrouted_traffic;
    double total_cost;
    std::string status;             // "OPTIMAL", "CONGESTED_REROUTE", "FAILOVER", "INSUFFICIENT_CAPACITY"
    bool change_detected;
    std::string trigger_reason;
    std::map<std::string, double> previous_allocation;
    std::map<std::string, double> new_allocation;
    std::map<std::string, double> allocation_diff;
    std::vector<ServerAllocationItem> servers;
    double execution_time_ms;

    // Serializes decision to JSON for React / API consumption
    std::string to_json() const {
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"cycle_id\": " << cycle_id << ",\n";
        ss << "  \"timestamp\": \"" << timestamp << "\",\n";
        ss << "  \"requested_traffic\": " << std::fixed << std::setprecision(1) << requested_traffic << ",\n";
        ss << "  \"routed_traffic\": " << std::fixed << std::setprecision(1) << routed_traffic << ",\n";
        ss << "  \"unrouted_traffic\": " << std::fixed << std::setprecision(1) << unrouted_traffic << ",\n";
        ss << "  \"total_cost\": " << std::fixed << std::setprecision(2) << total_cost << ",\n";
        ss << "  \"status\": \"" << status << "\",\n";
        ss << "  \"change_detected\": " << (change_detected ? "true" : "false") << ",\n";
        ss << "  \"trigger_reason\": \"" << trigger_reason << "\",\n";

        // Previous Allocation
        ss << "  \"previous_allocation\": {";
        bool first = true;
        for (const auto& kv : previous_allocation) {
            if (!first) ss << ", ";
            ss << "\"" << kv.first << "\": " << kv.second;
            first = false;
        }
        ss << "},\n";

        // New Allocation
        ss << "  \"new_allocation\": {";
        first = true;
        for (const auto& kv : new_allocation) {
            if (!first) ss << ", ";
            ss << "\"" << kv.first << "\": " << kv.second;
            first = false;
        }
        ss << "},\n";

        // Allocation Diff
        ss << "  \"allocation_diff\": {";
        first = true;
        for (const auto& kv : allocation_diff) {
            if (!first) ss << ", ";
            ss << "\"" << kv.first << "\": " << (kv.second >= 0 ? "+" : "") << kv.second;
            first = false;
        }
        ss << "},\n";

        // Server Details List
        ss << "  \"server_details\": [\n";
        for (size_t i = 0; i < servers.size(); i++) {
            const auto& s = servers[i];
            ss << "    {\n";
            ss << "      \"server\": \"" << s.server << "\",\n";
            ss << "      \"assigned_traffic\": " << s.assigned_traffic << ",\n";
            ss << "      \"capacity\": " << s.capacity << ",\n";
            ss << "      \"utilization_pct\": " << std::fixed << std::setprecision(1) << s.utilization_pct << ",\n";
            ss << "      \"unit_cost\": " << std::fixed << std::setprecision(2) << s.unit_cost << ",\n";
            ss << "      \"subtotal_cost\": " << std::fixed << std::setprecision(2) << s.subtotal_cost << ",\n";
            ss << "      \"latency\": " << s.latency << ",\n";
            ss << "      \"status\": \"" << s.status << "\"\n";
            ss << "    }" << (i + 1 < servers.size() ? "," : "") << "\n";
        }
        ss << "  ],\n";
        ss << "  \"execution_time_ms\": " << std::fixed << std::setprecision(3) << execution_time_ms << "\n";
        ss << "}";
        return ss.str();
    }
};

/**
 * Adaptive Router Engine.
 * Monitors Redis state, detects significant condition shifts,
 * and recalculates the Min-Cost Max-Flow allocation.
 */
class AdaptiveRouter {
private:
    int cycle_counter;
    double last_requested_traffic;
    std::vector<ServerCondition> prev_conditions;
    RoutingDecision last_decision;
    double alpha; // Latency weight
    double beta;  // Bandwidth cost weight

public:
    AdaptiveRouter(double a = 1.0, double b = 2.0)
        : cycle_counter(0), last_requested_traffic(0.0), alpha(a), beta(b) {}

    /**
     * Requirement 11: Avoid unnecessary recalculations when nothing relevant changed.
     * Checks if latency delta > 2ms, status changed, or capacity changed > 50 req/s.
     */
    bool hasSignificantChange(
        const std::vector<ServerCondition>& curr,
        double requested_traffic,
        std::string& out_reason
    ) {
        if (prev_conditions.empty()) {
            out_reason = "Initial router cold start";
            return true;
        }

        if (std::abs(requested_traffic - last_requested_traffic) > 50.0) {
            std::stringstream ss;
            ss << "Traffic demand changed: " << last_requested_traffic << " -> " << requested_traffic << " req/s";
            out_reason = ss.str();
            return true;
        }

        if (curr.size() != prev_conditions.size()) {
            out_reason = "Server fleet membership changed";
            return true;
        }

        for (size_t i = 0; i < curr.size(); i++) {
            const auto& c = curr[i];
            const auto& p = prev_conditions[i];

            // 1. Status change (e.g. HEALTHY -> CONGESTED or DOWN)
            if (c.status != p.status) {
                out_reason = c.name + " status changed: " + p.status + " -> " + c.status;
                return true;
            }

            // 2. Capacity changed significantly (> 50 req/s)
            if (std::abs(c.capacity - p.capacity) > 50.0) {
                out_reason = c.name + " capacity changed: " + std::to_string((int)p.capacity) + " -> " + std::to_string((int)c.capacity);
                return true;
            }

            // 3. Latency shifted (> 2.0 ms triggers cost delta)
            if (std::abs(c.latency - p.latency) > 2.0) {
                out_reason = c.name + " latency shifted: " + std::to_string((int)p.latency) + "ms -> " + std::to_string((int)c.latency) + "ms";
                return true;
            }
        }

        out_reason = "No significant condition changes detected (cached decision reused)";
        return false;
    }

    /**
     * Evaluates live conditions, detects changes, runs MCMF if needed,
     * and produces the comprehensive before/after RoutingDecision.
     */
    RoutingDecision evaluate(
        const std::vector<ServerCondition>& current_conditions,
        double requested_traffic,
        const std::string& timestamp_str = ""
    ) {
        cycle_counter++;
        std::string trigger_reason;
        bool changed = hasSignificantChange(current_conditions, requested_traffic, trigger_reason);

        // If nothing changed, return cached decision with updated timestamp
        if (!changed && !prev_conditions.empty()) {
            RoutingDecision cached = last_decision;
            cached.cycle_id = cycle_counter;
            cached.change_detected = false;
            cached.trigger_reason = trigger_reason;
            if (!timestamp_str.empty()) cached.timestamp = timestamp_str;
            return cached;
        }

        // Construct graph and execute MCMF
        auto start_time = std::chrono::high_resolution_clock::now();

        MinCostMaxFlow mcmf(6);
        std::vector<ServerState> server_states;
        for (const auto& c : current_conditions) {
            ServerState s;
            s.name = c.name;
            s.capacity = c.capacity;
            s.current_requests = c.current_requests;
            s.latency = c.latency;
            s.cpu = c.cpu;
            s.bandwidth_usage = c.bandwidth_usage;
            s.bandwidth_capacity = c.capacity * 0.2;
            s.bandwidth_cost = c.bandwidth_cost;
            s.status = c.status;
            s.timestamp = c.timestamp;
            server_states.push_back(s);
        }

        mcmf.buildFromState(server_states, alpha, beta);
        MCMFResult mcmf_res = mcmf.solve(0, 5, requested_traffic);

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end_time - start_time;

        // Build Decision structure
        RoutingDecision decision;
        decision.cycle_id = cycle_counter;
        decision.timestamp = timestamp_str.empty() ? "2026-10-01T22:35:00Z" : timestamp_str;
        decision.requested_traffic = requested_traffic;
        decision.routed_traffic = mcmf_res.routed_traffic;
        decision.unrouted_traffic = std::max(0.0, requested_traffic - mcmf_res.routed_traffic);
        decision.total_cost = mcmf_res.total_cost;
        decision.change_detected = true;
        decision.trigger_reason = trigger_reason;
        decision.execution_time_ms = duration.count();
        decision.previous_allocation = last_decision.new_allocation;
        decision.new_allocation = mcmf_res.allocation;

        // Determine decision state badge
        bool any_down = false;
        bool any_congested = false;
        for (const auto& c : current_conditions) {
            if (c.status == "DOWN") any_down = true;
            if (c.status == "CONGESTED") any_congested = true;
        }

        if (!mcmf_res.fully_routed) {
            decision.status = "INSUFFICIENT_CAPACITY";
        } else if (any_down) {
            decision.status = "FAILOVER";
        } else if (any_congested) {
            decision.status = "CONGESTED_REROUTE";
        } else {
            decision.status = "OPTIMAL";
        }

        // Compute Allocation Diffs and Server Details
        for (const auto& c : current_conditions) {
            double new_flow = mcmf_res.allocation.count(c.name) ? mcmf_res.allocation[c.name] : 0.0;
            double prev_flow = decision.previous_allocation.count(c.name) ? decision.previous_allocation[c.name] : 0.0;
            decision.allocation_diff[c.name] = new_flow - prev_flow;

            ServerAllocationItem item;
            item.server = c.name;
            item.assigned_traffic = new_flow;
            item.capacity = (c.status == "DOWN") ? 0.0 : c.capacity;
            item.utilization_pct = (item.capacity > 0) ? (new_flow / item.capacity * 100.0) : 0.0;
            item.latency = c.latency;
            item.status = c.status;
            item.unit_cost = (c.status == "DOWN") ? 9999.0 : (alpha * c.latency + beta * c.bandwidth_cost);
            item.subtotal_cost = new_flow * item.unit_cost;
            decision.servers.push_back(item);
        }

        // Update state cache
        prev_conditions = current_conditions;
        last_requested_traffic = requested_traffic;
        last_decision = decision;

        return decision;
    }
};

#endif // ADAPTIVE_ROUTER_HPP
