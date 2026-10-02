#ifndef SIMULATION_SCENARIO_H
#define SIMULATION_SCENARIO_H

#include "TrafficSimulator.h"
#include "Router.h"
#include "Graph.h"
#include <string>
#include <vector>

/**
 * Result record for a single scenario execution in DAA validation.
 */
struct ScenarioResult {
    int scenarioNumber;
    std::string name;
    std::string description;
    int demand;                     // Input demand (req/s)
    int totalAvailableCapacity;     // Total active fleet capacity (req/s)
    int routedFlow;                 // Routed flow (req/s)
    int unroutedFlow;               // Unrouted / dropped flow (req/s)
    double totalCostRs;             // Total cost in INR (Rs.)
    int activeServersCount;         // Number of active (non-failed) servers
    int augmentingPaths;            // Number of augmenting paths pushed in MCMF
    std::string routingStatus;      // Status badge (OPTIMAL, FAILOVER, etc.)
    double executionTimeUs;         // Execution time in microseconds

    // Automated DAA Assertion Flags:
    bool flowConservationPassed;    // Inflow == Outflow for all intermediate servers
    bool capacityConstraintPassed;  // 0 <= flow <= capacity on all forward edges
    bool demandConstraintPassed;    // routedFlow <= demand
    bool costNonNegativePassed;     // totalCost >= 0
    bool overloadCalcPassed;        // unroutedFlow == demand - routedFlow
    bool failureZeroFlowPassed;     // failed servers have exactly 0 flow
    bool feasibilityPassed;         // Flow is fully feasible
    bool passedAssertions;          // Overall scenario passed
    std::string summaryMessage;
    std::string assertionReport;
};

/**
 * Test & demonstration harness for all 7 project scenarios.
 */
class SimulationScenarioRunner {
private:
    TrafficSimulator simulator;
    Router router;
    std::vector<ScenarioResult> history;

    void printHeader(int number, const std::string& title, const std::string& objective) const;
    void printFooter(bool passed, const std::string& note, const ScenarioResult& res) const;

public:
    SimulationScenarioRunner();

    // Verification of DAA Network Flow Assertions
    static bool verifyAssertions(
        const Graph& graph,
        const std::vector<CDNServer>& servers,
        int demand,
        int routedFlow,
        int unroutedFlow,
        double totalCost,
        ScenarioResult& res
    );

    // Individual scenario runners
    ScenarioResult runScenario1_NormalTraffic();
    ScenarioResult runScenario2_TrafficSpike();
    ScenarioResult runScenario3_PuneCongestion();
    ScenarioResult runScenario4_MumbaiFailure();
    ScenarioResult runScenario5_MultipleCongestion();
    ScenarioResult runScenario6_Overload();
    ScenarioResult runScenario7_Recovery();

    // Execute complete end-to-end suite
    std::vector<ScenarioResult> runAllScenarios();

    // Summary table
    void printSummaryTable() const;

    // Export reports to disk
    void exportResultsToCsv(const std::string& filepath) const;
    void exportTestReport(const std::string& filepath) const;

    TrafficSimulator& getSimulator() { return simulator; }
    Router& getRouter() { return router; }
    const std::vector<ScenarioResult>& getHistory() const { return history; }
};

#endif // SIMULATION_SCENARIO_H
