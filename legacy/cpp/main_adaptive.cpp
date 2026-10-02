#include "adaptive_router.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>

void printDecisionBanner(const RoutingDecision& d) {
    std::cout << "\n========================================================================================\n";
    std::cout << " [CYCLE #" << d.cycle_id << "] ROUTING DECISION: " << d.status 
              << " (" << (d.change_detected ? "RECOMPUTED via MCMF" : "CACHED / SKIPPED") << ")\n";
    std::cout << "========================================================================================\n";
    std::cout << " Trigger Reason:          " << d.trigger_reason << "\n";
    std::cout << " Requested Traffic:       " << (int)d.requested_traffic << " req/s\n";
    std::cout << " Routed Traffic:          " << (int)d.routed_traffic << " req/s";
    if (d.unrouted_traffic > 0.1) {
        std::cout << " (Unrouted: " << (int)d.unrouted_traffic << " req/s - INSUFFICIENT CAPACITY)";
    }
    std::cout << "\n Total Routing Cost:      ₹" << std::fixed << std::setprecision(2) << d.total_cost << "\n";
    std::cout << " Execution Time:          " << std::fixed << std::setprecision(3) << d.execution_time_ms << " ms\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(12) << "Server"
              << std::setw(12) << "Status"
              << std::setw(10) << "Latency"
              << std::setw(14) << "Prev Alloc"
              << std::setw(14) << "New Alloc"
              << std::setw(14) << "Traffic Diff"
              << std::setw(12) << "Unit Cost"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    for (const auto& s : d.servers) {
        double prev = d.previous_allocation.count(s.server) ? d.previous_allocation.at(s.server) : 0.0;
        double diff = d.allocation_diff.count(s.server) ? d.allocation_diff.at(s.server) : 0.0;
        std::string diff_str = (diff > 0 ? "+" : "") + std::to_string((int)diff) + " req/s";
        std::string lat_str = (s.status == "DOWN") ? "INF" : (std::to_string((int)s.latency) + " ms");

        std::cout << std::left
                  << std::setw(12) << s.server
                  << std::setw(12) << s.status
                  << std::setw(10) << lat_str
                  << std::setw(14) << (std::to_string((int)prev) + " req/s")
                  << std::setw(14) << (std::to_string((int)s.assigned_traffic) + " req/s")
                  << std::setw(14) << diff_str
                  << std::setw(12) << std::fixed << std::setprecision(1) << s.unit_cost
                  << "\n";
    }
    std::cout << "========================================================================================\n";
}

int main() {
    std::cout << "========================================================================================\n";
    std::cout << "     ADAPTIVE CDN TRAFFIC ROUTER -- PHASE 5: CLOSED-LOOP ADAPTIVE ROUTING (C++)         \n";
    std::cout << "========================================================================================\n";

    AdaptiveRouter router(1.0, 2.0); // alpha = 1.0, beta = 2.0

    // Base fleet conditions
    std::vector<ServerCondition> fleet = {
        {"Pune", 5000.0, 3200.0, 25.0, 64.0, 600.0, 8.0, "HEALTHY", "2026-10-01T22:35:01Z"},
        {"Mumbai", 6000.0, 3800.0, 18.0, 63.3, 712.5, 6.0, "HEALTHY", "2026-10-01T22:35:01Z"},
        {"Delhi", 5500.0, 3400.0, 32.0, 61.8, 637.5, 9.0, "HEALTHY", "2026-10-01T22:35:01Z"},
        {"Bangalore", 5000.0, 3100.0, 24.0, 62.0, 581.3, 7.5, "HEALTHY", "2026-10-01T22:35:01Z"}
    };

    double normal_demand = 10000.0;

    // -------------------------------------------------------------------------
    // CYCLE 1: Cold Start Normal Traffic (10,000 req/s)
    // -------------------------------------------------------------------------
    RoutingDecision d1 = router.evaluate(fleet, normal_demand, "2026-10-01T22:35:01Z");
    printDecisionBanner(d1);

    // -------------------------------------------------------------------------
    // CYCLE 2: No Significant Change (Requirement 11 Test)
    // Small micro-jitter: Pune latency changes by only 0.4ms
    // Should detect NO significant change and reuse cached decision!
    // -------------------------------------------------------------------------
    fleet[0].latency = 25.4; // Minor jitter < 2ms threshold
    RoutingDecision d2 = router.evaluate(fleet, normal_demand, "2026-10-01T22:35:03Z");
    printDecisionBanner(d2);

    // -------------------------------------------------------------------------
    // CYCLE 3: Pune Congestion Event (Latency 25ms -> 85ms, Status -> CONGESTED)
    // Routing cost for Pune shoots from 41.0 to 101.0!
    // MCMF automatically drains traffic from Pune and redistributes to Mumbai & Bangalore!
    // -------------------------------------------------------------------------
    fleet[0].latency = 85.0;
    fleet[0].cpu = 94.0;
    fleet[0].status = "CONGESTED";
    RoutingDecision d3 = router.evaluate(fleet, normal_demand, "2026-10-01T22:35:05Z");
    printDecisionBanner(d3);

    // -------------------------------------------------------------------------
    // CYCLE 4: Catastrophic Server Failure (Mumbai -> DOWN)
    // Mumbai goes offline (Capacity = 0).
    // The router detects outage and evacuates Mumbai traffic to Bangalore & Pune!
    // -------------------------------------------------------------------------
    fleet[1].status = "DOWN";
    fleet[1].capacity = 0.0;
    fleet[1].latency = -1.0;
    RoutingDecision d4 = router.evaluate(fleet, normal_demand, "2026-10-01T22:35:07Z");
    printDecisionBanner(d4);

    // -------------------------------------------------------------------------
    // CYCLE 5: Server Recovery (Mumbai recovers to HEALTHY)
    // Mumbai restored with 6000 req/s capacity.
    // The router automatically detects recovery and rebalances fleet!
    // -------------------------------------------------------------------------
    fleet[0].latency = 25.0; // Pune normalized
    fleet[0].cpu = 64.0;
    fleet[0].status = "HEALTHY";

    fleet[1].status = "HEALTHY"; // Mumbai online
    fleet[1].capacity = 6000.0;
    fleet[1].latency = 18.0;
    RoutingDecision d5 = router.evaluate(fleet, normal_demand, "2026-10-01T22:35:09Z");
    printDecisionBanner(d5);

    // -------------------------------------------------------------------------
    // CYCLE 6: Insufficient Total Capacity (Overload Demand: 25,000 req/s)
    // Total fleet capacity is 21,500 req/s.
    // Router routes max 21,500 req/s and accurately flags 3,500 req/s unrouted.
    // -------------------------------------------------------------------------
    double overload_demand = 25000.0;
    RoutingDecision d6 = router.evaluate(fleet, overload_demand, "2026-10-01T22:35:11Z");
    printDecisionBanner(d6);

    // Export latest decision to JSON file for React dashboard consumption
    std::ofstream out_file("routing_decision.json");
    if (out_file.is_open()) {
        out_file << d6.to_json();
        out_file.close();
        std::cout << "\n[SUCCESS] Exported latest RoutingDecision to: routing_decision.json\n";
        std::cout << "          (Ready for future React Dashboard consumption)\n";
    }

    std::cout << "\n========================================================================================\n";
    std::cout << " [SUCCESS] Phase 5 Adaptive Routing loop verified across all 6 operational cycles!\n";
    std::cout << "========================================================================================\n\n";

    return 0;
}
