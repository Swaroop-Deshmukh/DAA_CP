#ifndef SIMULATION_SCENARIO_H
#define SIMULATION_SCENARIO_H

#include "TrafficSimulator.h"
#include "Router.h"
#include <string>
#include <vector>

/**
 * Result record for a single scenario execution.
 */
struct ScenarioResult {
    int scenarioNumber;
    std::string name;
    std::string description;
    int requestedTraffic;
    int routedTraffic;
    int unroutedTraffic;
    double totalCostRs;
    std::string status;
    bool passedAssertions;
    std::string summaryMessage;
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
    void printFooter(bool passed, const std::string& note) const;

public:
    SimulationScenarioRunner();

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

    TrafficSimulator& getSimulator() { return simulator; }
    Router& getRouter() { return router; }
};

#endif // SIMULATION_SCENARIO_H
