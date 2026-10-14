#ifndef SIMULATION_SCENARIO_H
#define SIMULATION_SCENARIO_H

#include "TrafficSimulator.h"
#include "Router.h"
#include "Graph.h"
#include "GraphConfig.h"
#include "EdmondsKarp.h"
#include "Baselines.h"
#include "CacheKnapsack.h"
#include "HoltForecast.h"
#include "StabilityManager.h"
#include "AlgorithmTrace.h"
#include <string>
#include <vector>
#include <map>

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
 * Detailed record comparing Greedy Heuristic vs Min-Cost Max-Flow
 * on the counterexample topology (Phase 1 Requirement 5).
 */
struct CounterexampleResult {
    int totalDemand;

    // Greedy Metrics
    double greedyTotalCost;
    int greedyOriginTraffic;
    std::map<std::string, std::string> greedyAssignments;

    // MCMF Metrics
    double mcmfTotalCost;
    int mcmfOriginTraffic;
    std::map<std::pair<std::string, std::string>, int> mcmfFlows;

    double costSavingsPercent;
    bool mcmfStrictlyBetter;
    bool passedAssertions;
};

/**
 * Test & demonstration harness for project scenarios.
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

    // Individual scenario runners (Legacy 4-Server Preset)
    ScenarioResult runScenario1_NormalTraffic();
    ScenarioResult runScenario2_TrafficSpike();
    ScenarioResult runScenario3_PuneCongestion();
    ScenarioResult runScenario4_MumbaiFailure();
    ScenarioResult runScenario5_MultipleCongestion();
    ScenarioResult runScenario6_Overload();
    ScenarioResult runScenario7_Recovery();

    // Phase 1: Counterexample Scenario (Greedy vs MCMF)
    CounterexampleResult runCounterexampleScenario();

    // Phase 1: Default 8-Region, 6-Server Network Demo with Convex Bands
    bool runDefault8RegionNetworkDemo(const std::string& configPath = "");

    // Phase 2a: Min-Cut Extraction for Total Fleet Overload
    MinCutResult runMinCutOverloadAnalysis();

    // Phase 2a: Multi-Strategy Baseline Comparison Suite across All Scenarios
    void runStrategyComparisonAcrossAllScenarios();

    // Phase 2b: Cache Placement (0/1 Knapsack DP vs Greedy-by-Density)
    bool runCacheKnapsackComparison();

    // Phase 2b: Traffic Spike Scenario (Reactive vs Proactive Holt Smoothing)
    void runSpikeForecastComparison();

    // Phase 2b: Router Stability & Hysteresis Benchmark (100 Ticks)
    void runStabilityHysteresisComparison();

    // Phase 5: Algorithm Trace Mode on 3-Region 3-Server Educational Demonstration
    AlgorithmTraceReport runTrace3x3Demo(bool printStdout = true);

    // Execute complete end-to-end suite (All 7 legacy scenarios)
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
