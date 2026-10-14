#ifndef BASELINES_H
#define BASELINES_H

#include "CDNServer.h"
#include "Graph.h"
#include "MinCostMaxFlow.h"
#include <string>
#include <vector>
#include <map>
#include <iostream>

/**
 * Supported traffic allocation strategies for DAA comparative evaluation.
 */
enum class RoutingStrategy {
    MCMF_OPTIMAL,
    ROUND_ROBIN,
    NEAREST_GEOGRAPHIC,
    GREEDY_CHEAPEST,
    BELLMAN_FORD_REFERENCE
};

std::string strategyToString(RoutingStrategy strat);

/**
 * Uniform metrics record evaluated for any routing strategy.
 */
struct StrategyEvaluation {
    std::string strategyName;
    int totalDemand;
    int routedTraffic;
    int originTraffic;
    double totalCostRs;
    double avgLatencyMs;
    double p95LatencyMs;
    int overloadedServersCount; // Number of servers with >= 90% utilization
    std::map<std::string, int> serverAllocations;
    std::map<std::string, double> serverUtilizations;
};

/**
 * Baseline Routing Engine.
 * Evaluates Round-Robin, Nearest-Geographic, and Greedy Cheapest-First
 * on the exact same fleet conditions and demand as MCMF.
 */
class BaselineEvaluator {
private:
    double latencyWeight;
    double bandwidthWeight;
    double originPenaltyCost;
    double originLatency;

public:
    BaselineEvaluator(
        double latWeight = 1.0,
        double bwWeight = 2.0,
        double origPenalty = 250.0,
        double origLat = 120.0
    );

    StrategyEvaluation evaluateStrategy(
        RoutingStrategy strategy,
        const std::vector<CDNServer>& servers,
        int demand
    );

    static void printComparisonTable(
        const std::string& scenarioTitle,
        const std::vector<StrategyEvaluation>& evaluations
    );

    static void printMasterComparisonMatrix(
        const std::vector<std::pair<std::string, std::vector<StrategyEvaluation>>>& scenarioMatrices
    );
};

#endif // BASELINES_H
