#include "CDNServer.h"
#include "TrafficSimulator.h"
#include "Graph.h"
#include "MinCostMaxFlow.h"
#include "Router.h"
#include "SimulationScenario.h"
#include "Benchmark.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void printBanner() {
    std::cout << "========================================================================================\n";
    std::cout << "        ADAPTIVE CDN TRAFFIC ROUTER WITH MIN-COST MAX-FLOW (C++17)                      \n";
    std::cout << "        Design and Analysis of Algorithms (DAA) Capstone Project                        \n";
    std::cout << "========================================================================================\n";
    std::cout << " Algorithms: Successive Shortest Path (SSP) + Dijkstra with Johnson's Node Potentials   \n";
    std::cout << " Complexity: O(K * |E| log |V|) where K <= |CDN Edge Servers| = 4                       \n";
    std::cout << " Topology:   Source (0) -> 4 Edge Servers (1..4) -> Origin Sink (5)                     \n";
    std::cout << " Nodes:      Pune, Mumbai, Delhi, Bangalore                                             \n";
    std::cout << " Cost Model: Scaled Integer Arithmetic W = 100 * (alpha * Latency + beta * BandwidthCost)\n";
    std::cout << "========================================================================================\n\n";
}

void saveJsonDecision(const RoutingDecision& decision) {
    const std::vector<std::string> targetPaths = {
        "../data/routing_decision.json",
        "../../data/routing_decision.json",
        "data/routing_decision.json"
    };

    bool saved = false;
    for (const auto& path : targetPaths) {
        std::ofstream outFile(path);
        if (outFile.is_open()) {
            outFile << decision.toJson();
            outFile.close();
            std::cout << ">> [EXPORT SUCCESS]: Latest routing decision exported to '" << path << "'\n";
            saved = true;
            break;
        }
    }
    if (!saved) {
        std::cerr << "!! [EXPORT WARNING]: Could not find data/ directory to export routing_decision.json\n";
    }
}

void runLiveAdaptiveLoop(int iterations = 10, int delayMs = 1500) {
    std::cout << "\n========================================================================================\n";
    std::cout << "                   STARTING LIVE ADAPTIVE SIMULATION LOOP                               \n";
    std::cout << "========================================================================================\n";

    TrafficSimulator sim;
    Router router;

    std::vector<int> demandPattern = {8000, 10000, 16000, 12000, 10000, 22000, 14000, 9000, 8000, 8000};

    for (int cycle = 0; cycle < iterations; ++cycle) {
        int demand = demandPattern[cycle % demandPattern.size()];

        // Inject dynamic events into simulator
        if (cycle == 2) {
            std::cout << "\n>>> [EVENT TRIGGER]: Sudden Traffic Surge at Tick " << cycle << " <<<\n";
            sim.applyScenario(ScenarioType::TRAFFIC_SPIKE);
        } else if (cycle == 4) {
            std::cout << "\n>>> [EVENT TRIGGER]: Pune Congestion Escalation at Tick " << cycle << " <<<\n";
            sim.applyScenario(ScenarioType::PUNE_CONGESTION);
        } else if (cycle == 6) {
            std::cout << "\n>>> [EVENT TRIGGER]: Mumbai Server Hardware Crash at Tick " << cycle << " <<<\n";
            sim.failServer("Mumbai");
        } else if (cycle == 8) {
            std::cout << "\n>>> [EVENT TRIGGER]: Mumbai Server Recovery at Tick " << cycle << " <<<\n";
            sim.applyScenario(ScenarioType::RECOVERY);
        } else {
            sim.tick(true); // stochastic drift
        }

        RoutingDecision decision = router.route(sim.getServers(), demand);
        router.printDecisionTable(decision);

        // Export decision for external consumers (e.g. React dashboard)
        saveJsonDecision(decision);

        if (cycle + 1 < iterations) {
#ifdef _WIN32
            Sleep(delayMs);
#else
            usleep(delayMs * 1000);
#endif
        }
    }

    std::cout << "\n>> Live Adaptive Simulation Loop finished " << iterations << " cycles successfully.\n";
}

int main(int argc, char* argv[]) {
    printBanner();

    std::string mode = "--demo";
    if (argc > 1) {
        mode = argv[1];
    }

    if (mode == "--scenarios" || mode == "1") {
        SimulationScenarioRunner runner;
        runner.runAllScenarios();
        saveJsonDecision(runner.getRouter().getLastDecision());
    } else if (mode == "--bench" || mode == "2") {
        BenchmarkSuite bench;
        auto records = bench.runFullDemandSuite();
        bench.printReport(records);
        bench.printTheoreticalComplexityAnalysis();
    } else if (mode == "--loop" || mode == "3") {
        runLiveAdaptiveLoop(10, 1000);
    } else if (mode == "--scenario" || mode.rfind("--scenario-", 0) == 0 || mode == "--reset") {
        std::string scen = "normal";
        if (mode == "--scenario" && argc > 2) {
            scen = argv[2];
        } else if (mode.rfind("--scenario-", 0) == 0) {
            scen = mode.substr(11);
        } else if (mode == "--reset") {
            scen = "normal";
        }

        SimulationScenarioRunner runner;
        if (scen == "normal" || scen == "1" || scen == "reset") {
            runner.runScenario1_NormalTraffic();
        } else if (scen == "spike" || scen == "2") {
            runner.runScenario1_NormalTraffic();
            runner.runScenario2_TrafficSpike();
        } else if (scen == "pune" || scen == "pune_congestion" || scen == "3") {
            runner.runScenario1_NormalTraffic();
            runner.runScenario3_PuneCongestion();
        } else if (scen == "mumbai_fail" || scen == "mumbai" || scen == "4") {
            runner.runScenario1_NormalTraffic();
            runner.runScenario4_MumbaiFailure();
        } else if (scen == "multi" || scen == "multiple_congestion" || scen == "5") {
            runner.runScenario1_NormalTraffic();
            runner.runScenario5_MultipleCongestion();
        } else if (scen == "overload" || scen == "6") {
            runner.runScenario1_NormalTraffic();
            runner.runScenario6_Overload();
        } else if (scen == "recovery" || scen == "7") {
            runner.runScenario4_MumbaiFailure();
            runner.runScenario7_Recovery();
        } else {
            std::cerr << "Unknown scenario: " << scen << ". Running normal traffic by default.\n";
            runner.runScenario1_NormalTraffic();
        }
        saveJsonDecision(runner.getRouter().getLastDecision());
    } else {
        // Default: Full Demo
        std::cout << "Executing complete demonstration: [Scenarios Suite] + [Performance Benchmarking]...\n\n";

        // 1. Run all 7 scenarios
        SimulationScenarioRunner runner;
        runner.runAllScenarios();
        saveJsonDecision(runner.getRouter().getLastDecision());

        // 2. Run Benchmarks & Complexity Analysis
        BenchmarkSuite bench;
        auto records = bench.runFullDemandSuite();
        bench.printReport(records);
        bench.printTheoreticalComplexityAnalysis();
    }

    std::cout << "All DAA C++ Engine modules executed successfully.\n";
    return 0;
}
