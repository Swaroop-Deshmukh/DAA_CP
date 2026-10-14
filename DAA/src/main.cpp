#include "CDNServer.h"
#include "TrafficSimulator.h"
#include "Graph.h"
#include "MinCostMaxFlow.h"
#include "Router.h"
#include "SimulationScenario.h"
#include "Benchmark.h"
#include "SelfTest.h"

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

#include "IpcEngine.h"

void printBanner() {
    std::cout << "========================================================================================\n";
    std::cout << "        ADAPTIVE CDN TRAFFIC ROUTER WITH MIN-COST MAX-FLOW (C++17)                      \n";
    std::cout << "        Design and Analysis of Algorithms (DAA) Capstone Project                        \n";
    std::cout << "========================================================================================\n";
    std::cout << " Algorithms: Successive Shortest Path (SSP) + Dijkstra with Johnson's Node Potentials   \n";
    std::cout << " Network:    Bipartite Graph (Source -> N Regions -> M Servers -> Convex Bands -> Sink) \n";
    std::cout << " Fallback:   Direct Region -> Origin Fallback (Guaranteed Feasibility & Miss Modeling)  \n";
    std::cout << " Congestion: Piecewise-Linear Convex Utilization Bands (0-50%, 50-80%, 80-95%, 95-100%) \n";
    std::cout << " Cost Model: Scaled Integer Arithmetic W = 100 * (alpha * Latency + beta * BandwidthCost)\n";
    std::cout << " Modes:      --ipc, --selftest, --trace, --scenarios, --bench, --demo                   \n";
    std::cout << "========================================================================================\n\n";
}

void saveJsonDecision(const RoutingDecision& decision) {
    IpcEngine::saveDecisionToStandardPaths(decision.toJson());
    std::cout << ">> [EXPORT SUCCESS]: Latest routing decision exported atomically to 'data/routing_decision.json'\n";
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
    std::string mode = "--demo";
    if (argc > 1) {
        mode = argv[1];
    }

    // Phase 6 Requirement: Long-lived engine IPC daemon (no banner on stdout)
    if (mode == "--ipc" || mode == "--daemon") {
        IpcEngine engine;
        engine.runInteractive();
        return 0;
    }

    printBanner();

    if (mode == "--selftest" || mode == "--self-test" || mode == "--test") {
        bool allPassed = SelfTestRunner::runAllSelfTests();
        return allPassed ? 0 : 1;
    } else if (mode == "--trace" || mode == "--trace-small" || mode == "-t") {
        SimulationScenarioRunner runner;
        auto traceReport = runner.runTrace3x3Demo(true);

        // Export trace array to data/routing_decision.json
        const std::vector<std::string> targetPaths = {
            "../data/routing_decision.json",
            "../../data/routing_decision.json",
            "data/routing_decision.json"
        };
        bool saved = false;
        for (const auto& path : targetPaths) {
            std::ofstream outFile(path);
            if (outFile.is_open()) {
                outFile << traceReport.toJsonString();
                outFile.close();
                std::cout << ">> [EXPORT SUCCESS]: Algorithm trace JSON exported to '" << path << "'\n";
                saved = true;
                break;
            }
        }
        if (!saved) {
            std::ofstream outFile("data/routing_decision.json");
            if (outFile.is_open()) outFile << traceReport.toJsonString();
        }

        // Also export to results/trace_3x3_output.json
        std::ofstream resFile("results/trace_3x3_output.json");
        if (resFile.is_open()) {
            resFile << traceReport.toJsonString();
            resFile.close();
        }
        std::ofstream resFile2("DAA/results/trace_3x3_output.json");
        if (resFile2.is_open()) {
            resFile2 << traceReport.toJsonString();
            resFile2.close();
        }
    } else if (mode == "--counterexample" || mode == "--ce") {
        SimulationScenarioRunner runner;
        runner.runCounterexampleScenario();
    } else if (mode == "--network" || mode == "--topo" || mode == "--topology") {
        std::string cfgPath = (argc > 2) ? argv[2] : "";
        SimulationScenarioRunner runner;
        runner.runDefault8RegionNetworkDemo(cfgPath);
    } else if (mode == "--mincut" || mode == "--cut") {
        SimulationScenarioRunner runner;
        runner.runMinCutOverloadAnalysis();
    } else if (mode == "--baselines" || mode == "--compare" || mode == "--eval") {
        SimulationScenarioRunner runner;
        runner.runStrategyComparisonAcrossAllScenarios();
    } else if (mode == "--cache" || mode == "--knapsack") {
        SimulationScenarioRunner runner;
        runner.runCacheKnapsackComparison();
    } else if (mode == "--forecast" || mode == "--spike-compare" || mode == "--proactive") {
        SimulationScenarioRunner runner;
        runner.runSpikeForecastComparison();
    } else if (mode == "--stability" || mode == "--hysteresis") {
        SimulationScenarioRunner runner;
        runner.runStabilityHysteresisComparison();
    } else if (mode == "--scenarios" || mode == "1") {
        SimulationScenarioRunner runner;
        runner.runAllScenarios();
        saveJsonDecision(runner.getRouter().getLastDecision());
    } else if (mode == "--bench" || mode == "--benchmark" || mode == "2") {
        BenchmarkSuite bench;
        auto scalRecords = bench.runScalabilityBenchmark({10, 50, 100, 500, 1000}, 5);
        bench.printScalabilityReport(scalRecords);

        // Also run fixed-topology demand benchmark
        auto demandRecords = bench.runFullDemandSuite();
        bench.printReport(demandRecords);
        bench.printTheoreticalComplexityAnalysis();
    } else if (mode == "--bench-demand") {
        BenchmarkSuite bench;
        auto demandRecords = bench.runFullDemandSuite();
        bench.printReport(demandRecords);
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
        if (scen == "counterexample" || scen == "ce") {
            runner.runCounterexampleScenario();
        } else if (scen == "network" || scen == "topo") {
            runner.runDefault8RegionNetworkDemo();
        } else if (scen == "normal" || scen == "1" || scen == "reset") {
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
        std::cout << "Executing complete demonstration: [Counterexample] + [8-Region Network] + [7 Scenarios] + [Benchmarks]...\n\n";

        SimulationScenarioRunner runner;

        // 1. Run Phase 1 Counterexample (Greedy vs MCMF)
        runner.runCounterexampleScenario();

        // 2. Run Phase 1 Default 8-Region Network with Convex Bands
        runner.runDefault8RegionNetworkDemo();

        // 3. Run Phase 2a Min-Cut Overload Analysis (Max-Flow Min-Cut Theorem)
        runner.runMinCutOverloadAnalysis();

        // 4. Run Phase 2a Baseline Strategy Comparisons across all scenarios
        runner.runStrategyComparisonAcrossAllScenarios();

        // 5. Run Phase 2b Cache Placement Optimization (0/1 Knapsack DP vs Greedy)
        runner.runCacheKnapsackComparison();

        // 6. Run Phase 2b Traffic Spike Scenario (Reactive vs Proactive Holt Smoothing)
        runner.runSpikeForecastComparison();

        // 7. Run Phase 2b Router Stability & Hysteresis Benchmark (100 Ticks)
        runner.runStabilityHysteresisComparison();

        // 8. Run all 7 legacy verification scenarios (4-Server preset)
        runner.runAllScenarios();
        saveJsonDecision(runner.getRouter().getLastDecision());

        // 9. Run Phase 5 Algorithm Trace Mode (Small 3x3 Educational Example)
        runner.runTrace3x3Demo(true);

        // 10. Run Phase 4 Random-Graph Scalability Benchmark & Complexity Analysis
        BenchmarkSuite bench;
        auto scalRecords = bench.runScalabilityBenchmark({10, 50, 100, 500, 1000}, 5);
        bench.printScalabilityReport(scalRecords);
        bench.printTheoreticalComplexityAnalysis();
    }

    std::cout << "All DAA C++ Engine modules executed successfully.\n";
    return 0;
}
