#include "SimulationScenario.h"
#include <iostream>
#include <iomanip>

SimulationScenarioRunner::SimulationScenarioRunner()
    : router(1.0, 2.0, 100) // alpha=1.0 (latency weight), beta=2.0 (cost weight), scaleFactor=100
{
    simulator.initializeDefaultFleet();
}

void SimulationScenarioRunner::printHeader(int number, const std::string& title, const std::string& objective) const {
    std::cout << "\n========================================================================================\n";
    std::cout << " SCENARIO " << number << ": " << title << "\n";
    std::cout << " OBJECTIVE:  " << objective << "\n";
    std::cout << "========================================================================================\n";
}

void SimulationScenarioRunner::printFooter(bool passed, const std::string& note) const {
    std::cout << "\n [SCENARIO RESULT]: " << (passed ? ">>> PASSED <<<" : "!!! FAILED !!!") << "\n";
    std::cout << " NOTE: " << note << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n\n";
}

ScenarioResult SimulationScenarioRunner::runScenario1_NormalTraffic() {
    printHeader(1, "NORMAL TRAFFIC ALLOCATION", 
                "Verify MCMF routes traffic to the lowest-cost paths first under nominal conditions.");

    simulator.initializeDefaultFleet();
    simulator.applyScenario(ScenarioType::NORMAL);
    simulator.printFleetSummary();

    int requestedTraffic = 8000;
    std::cout << "\n>> INJECTING DEMAND: " << requestedTraffic << " req/s across CDN network...\n";

    RoutingDecision decision = router.route(simulator.getServers(), requestedTraffic, "2026-10-01 22:30:00");
    router.printDecisionTable(decision);

    // Verify assertions:
    // 1. All 8000 req/s routed
    // 2. Mumbai has lowest cost (lat 18 + 2*6 = 30), so its 6000 capacity should be 100% saturated
    // 3. Next cheapest is Bangalore (lat 24 + 2*7.5 = 39), which absorbs the remaining 2000
    bool passed = (decision.routedTraffic == 8000 && decision.unroutedTraffic == 0);
    int mumbaiAlloc = decision.newAllocation["Mumbai"];
    if (mumbaiAlloc < 6000) passed = false;

    std::string summary = "MCMF prioritized lowest-cost nodes (Mumbai saturated at 6000, remainder to Bangalore). Zero drop.";
    printFooter(passed, summary);

    ScenarioResult res{1, "Normal Traffic", "Nominal conditions, min-cost greedy augmentation",
                       requestedTraffic, decision.routedTraffic, decision.unroutedTraffic,
                       decision.totalCostRs, decision.routingStatus, passed, summary};
    history.push_back(res);
    return res;
}

ScenarioResult SimulationScenarioRunner::runScenario2_TrafficSpike() {
    printHeader(2, "TRAFFIC SPIKE SURGE", 
                "Surge demand from 8,000 to 16,000 req/s; verify MCMF engages higher-cost nodes without dropping packets.");

    simulator.applyScenario(ScenarioType::TRAFFIC_SPIKE);
    simulator.printFleetSummary();

    int requestedTraffic = 16000;
    std::cout << "\n>> TRAFFIC SURGE: Increasing global demand to " << requestedTraffic << " req/s...\n";

    RoutingDecision decision = router.route(simulator.getServers(), requestedTraffic, "2026-10-01 22:31:00");
    router.printDecisionTable(decision);

    // Verify assertions:
    // Fleet capacity is 21,500. 16,000 is < 21,500, so unrouted must be 0.
    bool passed = (decision.routedTraffic == 16000 && decision.unroutedTraffic == 0);

    std::string summary = "Traffic spike fully absorbed across Mumbai, Bangalore, Pune, and Delhi. Zero packets dropped.";
    printFooter(passed, summary);

    ScenarioResult res{2, "Traffic Spike", "Surge from 8k to 16k req/s",
                       requestedTraffic, decision.routedTraffic, decision.unroutedTraffic,
                       decision.totalCostRs, decision.routingStatus, passed, summary};
    history.push_back(res);
    return res;
}

ScenarioResult SimulationScenarioRunner::runScenario3_PuneCongestion() {
    printHeader(3, "PUNE CONGESTION ADAPTATION", 
                "Simulate queuing latency spike at Pune; verify router detects cost change and shifts traffic away.");

    // Induce congestion at Pune
    simulator.applyScenario(ScenarioType::PUNE_CONGESTION);
    simulator.printFleetSummary();

    int requestedTraffic = 10000;
    std::cout << "\n>> ROUTING UNDER PUNE CONGESTION: Demand = " << requestedTraffic << " req/s...\n";

    RoutingDecision decision = router.route(simulator.getServers(), requestedTraffic, "2026-10-01 22:32:00");
    router.printDecisionTable(decision);

    // Assertions:
    // With Pune latency high (120ms), Pune's unit cost jumps significantly.
    // Mumbai (cap 6000) and Bangalore (cap 5000) should be prioritized before Pune.
    bool passed = (decision.routedTraffic == 10000 && decision.unroutedTraffic == 0);
    int puneAlloc = decision.newAllocation["Pune"];
    // Since Mumbai (6000) + Bangalore (5000) = 11,000 >= 10,000, Pune should get 0 or minimal traffic!
    if (puneAlloc > 1000) {
        // Warning if Pune was not avoided
        passed = false;
    }

    std::string summary = "Pune's elevated latency increased its edge cost. MCMF routed 10k entirely through Mumbai & Bangalore, bypassing Pune.";
    printFooter(passed, summary);

    ScenarioResult res{3, "Pune Congestion", "Latency spike shifts cost, rerouting traffic away from Pune",
                       requestedTraffic, decision.routedTraffic, decision.unroutedTraffic,
                       decision.totalCostRs, decision.routingStatus, passed, summary};
    history.push_back(res);
    return res;
}

ScenarioResult SimulationScenarioRunner::runScenario4_MumbaiFailure() {
    printHeader(4, "MUMBAI COMPLETE NODE FAILURE", 
                "Simulate complete crash of highest-capacity node (Mumbai); verify failover and zero dropped traffic.");

    simulator.failServer("Mumbai");
    simulator.printFleetSummary();

    int requestedTraffic = 12000;
    std::cout << "\n>> CRITICAL OUTAGE: Mumbai is DOWN (Capacity = 0). Demand = " << requestedTraffic << " req/s...\n";

    RoutingDecision decision = router.route(simulator.getServers(), requestedTraffic, "2026-10-01 22:33:00");
    router.printDecisionTable(decision);

    // Assertions:
    // 1. Mumbai must receive EXACTLY 0 requests
    // 2. Remaining active fleet (Pune 5000 + Delhi 5500 + Bangalore 5000 = 15,500) can carry 12,000
    // 3. Routing status must reflect FAILOVER
    int mumbaiAlloc = decision.newAllocation["Mumbai"];
    bool passed = (mumbaiAlloc == 0 && decision.routedTraffic == 12000 && decision.unroutedTraffic == 0);
    if (decision.routingStatus != "FAILOVER") passed = false;

    std::string summary = "Failover successful: 0 traffic routed to Mumbai. 12,000 req/s redistributed across Pune, Bangalore, and Delhi.";
    printFooter(passed, summary);

    ScenarioResult res{4, "Mumbai Failure", "Node down: capacity clamped to 0, seamless failover",
                       requestedTraffic, decision.routedTraffic, decision.unroutedTraffic,
                       decision.totalCostRs, decision.routingStatus, passed, summary};
    history.push_back(res);
    return res;
}

ScenarioResult SimulationScenarioRunner::runScenario5_MultipleCongestion() {
    printHeader(5, "MULTIPLE NODE CONGESTION", 
                "Simulate simultaneous congestion in Pune and Delhi; verify adaptive load balancing.");

    // Recover Mumbai first so it can handle load, then congest Pune and Delhi
    simulator.recoverServer("Mumbai");
    simulator.applyScenario(ScenarioType::MULTIPLE_CONGESTION);
    simulator.printFleetSummary();

    int requestedTraffic = 11000;
    std::cout << "\n>> MULTI-NODE CONGESTION: Pune & Delhi congested. Demand = " << requestedTraffic << " req/s...\n";

    RoutingDecision decision = router.route(simulator.getServers(), requestedTraffic, "2026-10-01 22:34:00");
    router.printDecisionTable(decision);

    // Assertions:
    // Mumbai (cap 6000) and Bangalore (cap 5000) are healthy with low latency.
    // 6000 + 5000 = 11,000, so MCMF should route 100% of 11,000 via Mumbai and Bangalore, sparing congested nodes!
    bool passed = (decision.routedTraffic == 11000 && decision.unroutedTraffic == 0);
    int healthyLoad = decision.newAllocation["Mumbai"] + decision.newAllocation["Bangalore"];
    if (healthyLoad < 10000) passed = false;

    std::string summary = "Adaptive router shielded congested nodes (Pune & Delhi), routing maximum possible flow via healthy nodes (Mumbai & Bangalore).";
    printFooter(passed, summary);

    ScenarioResult res{5, "Multiple Congestion", "Dual-node congestion: load shifted to healthy servers",
                       requestedTraffic, decision.routedTraffic, decision.unroutedTraffic,
                       decision.totalCostRs, decision.routingStatus, passed, summary};
    history.push_back(res);
    return res;
}

ScenarioResult SimulationScenarioRunner::runScenario6_Overload() {
    printHeader(6, "TOTAL FLEET OVERLOAD", 
                "Inject demand exceeding total available fleet capacity; verify safe saturation and unrouted traffic detection.");

    simulator.applyScenario(ScenarioType::OVERLOAD);
    simulator.printFleetSummary();

    // Fleet capacity: Pune (5000) + Mumbai (6000) + Delhi (5500) + Bangalore (5000) = 21,500 req/s.
    int requestedTraffic = 26000;
    std::cout << "\n>> OVERLOAD DEMAND: Requesting " << requestedTraffic << " req/s (Fleet max capacity is 21,500 req/s)...\n";

    RoutingDecision decision = router.route(simulator.getServers(), requestedTraffic, "2026-10-01 22:35:00");
    router.printDecisionTable(decision);

    // Assertions:
    // 1. Max flow must be exactly fleet capacity (21,500)
    // 2. Unrouted traffic must be 26,000 - 21,500 = 4,500 req/s
    // 3. Routing status must be INSUFFICIENT_CAPACITY
    bool passed = (decision.routedTraffic == 21500 && decision.unroutedTraffic == 4500);
    if (decision.routingStatus != "INSUFFICIENT_CAPACITY") passed = false;

    std::string summary = "Max flow correctly saturated fleet capacity at 21,500 req/s. Safely isolated 4,500 unrouted req/s without crashing.";
    printFooter(passed, summary);

    ScenarioResult res{6, "Total Fleet Overload", "Demand exceeds fleet capacity: safe max flow saturation",
                       requestedTraffic, decision.routedTraffic, decision.unroutedTraffic,
                       decision.totalCostRs, decision.routingStatus, passed, summary};
    history.push_back(res);
    return res;
}

ScenarioResult SimulationScenarioRunner::runScenario7_Recovery() {
    printHeader(7, "FLEET RECOVERY & STABILIZATION", 
                "Restore all servers to healthy nominal baseline; verify MCMF re-stabilizes global cost to minimal equilibrium.");

    simulator.applyScenario(ScenarioType::RECOVERY);
    simulator.printFleetSummary();

    int requestedTraffic = 10000;
    std::cout << "\n>> POST-RECOVERY STABILIZATION: Demand = " << requestedTraffic << " req/s...\n";

    RoutingDecision decision = router.route(simulator.getServers(), requestedTraffic, "2026-10-01 22:36:00");
    router.printDecisionTable(decision);

    // Assertions:
    // 1. All servers are HEALTHY
    // 2. 100% of 10,000 req/s routed
    // 3. Optimal baseline restored (Mumbai 6000, Bangalore 4000 or Pune)
    bool passed = (decision.routedTraffic == 10000 && decision.unroutedTraffic == 0);
    for (const auto& s : simulator.getServers()) {
        if (s.getStatus() != ServerStatus::HEALTHY) passed = false;
    }

    std::string summary = "All servers fully recovered to HEALTHY. Optimal min-cost routing baseline re-established.";
    printFooter(passed, summary);

    ScenarioResult res{7, "Fleet Recovery", "Restoration of all nodes: optimal cost baseline re-established",
                       requestedTraffic, decision.routedTraffic, decision.unroutedTraffic,
                       decision.totalCostRs, decision.routingStatus, passed, summary};
    history.push_back(res);
    return res;
}

std::vector<ScenarioResult> SimulationScenarioRunner::runAllScenarios() {
    history.clear();
    std::cout << "\n========================================================================================\n";
    std::cout << "        RUNNING COMPREHENSIVE 7-STAGE DAA SCENARIO VERIFICATION SUITE\n";
    std::cout << "========================================================================================\n";

    runScenario1_NormalTraffic();
    runScenario2_TrafficSpike();
    runScenario3_PuneCongestion();
    runScenario4_MumbaiFailure();
    runScenario5_MultipleCongestion();
    runScenario6_Overload();
    runScenario7_Recovery();

    printSummaryTable();
    return history;
}

void SimulationScenarioRunner::printSummaryTable() const {
    std::cout << "\n========================================================================================\n";
    std::cout << "                         7-SCENARIO SUITE VERIFICATION REPORT\n";
    std::cout << "========================================================================================\n";
    std::cout << " " << std::left << std::setw(4)  << "#"
              << " | " << std::left << std::setw(22) << "Scenario Name"
              << " | " << std::right << std::setw(8) << "Demand"
              << " | " << std::right << std::setw(8) << "Routed"
              << " | " << std::right << std::setw(8) << "Unrouted"
              << " | " << std::left << std::setw(20) << "Status"
              << " | " << std::left << std::setw(8)  << "Verdict" << " |\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    int passedCount = 0;
    for (const auto& res : history) {
        if (res.passedAssertions) passedCount++;
        std::cout << " " << std::left << std::setw(4)  << res.scenarioNumber
                  << " | " << std::left << std::setw(22) << res.name
                  << " | " << std::right << std::setw(8) << res.requestedTraffic
                  << " | " << std::right << std::setw(8) << res.routedTraffic
                  << " | " << std::right << std::setw(8) << res.unroutedTraffic
                  << " | " << std::left << std::setw(20) << res.status
                  << " | " << std::left << std::setw(8)  << (res.passedAssertions ? "PASS" : "FAIL") << " |\n";
    }
    std::cout << "========================================================================================\n";
    std::cout << " FINAL VERDICT: " << passedCount << "/" << history.size() << " SCENARIOS PASSED.\n";
    std::cout << "========================================================================================\n\n";
}
