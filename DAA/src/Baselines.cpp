#include "Baselines.h"
#include <iomanip>
#include <algorithm>
#include <cmath>

std::string strategyToString(RoutingStrategy strat) {
    switch (strat) {
        case RoutingStrategy::MCMF_OPTIMAL:           return "MCMF (Dijkstra+Potentials)";
        case RoutingStrategy::ROUND_ROBIN:            return "Round-Robin";
        case RoutingStrategy::NEAREST_GEOGRAPHIC:     return "Nearest-Geographic";
        case RoutingStrategy::GREEDY_CHEAPEST:        return "Greedy Cheapest-First";
        case RoutingStrategy::BELLMAN_FORD_REFERENCE: return "Bellman-Ford (Reference)";
        default: return "Unknown";
    }
}

BaselineEvaluator::BaselineEvaluator(
    double latWeight,
    double bwWeight,
    double origPenalty,
    double origLat
)
    : latencyWeight(latWeight),
      bandwidthWeight(bwWeight),
      originPenaltyCost(origPenalty),
      originLatency(origLat)
{}

StrategyEvaluation BaselineEvaluator::evaluateStrategy(
    RoutingStrategy strategy,
    const std::vector<CDNServer>& servers,
    int demand
) {
    StrategyEvaluation eval;
    eval.strategyName = strategyToString(strategy);
    eval.totalDemand = demand;
    eval.routedTraffic = 0;
    eval.originTraffic = 0;
    eval.totalCostRs = 0.0;
    eval.avgLatencyMs = 0.0;
    eval.p95LatencyMs = 0.0;
    eval.overloadedServersCount = 0;

    for (const auto& s : servers) {
        eval.serverAllocations[s.getName()] = 0;
        eval.serverUtilizations[s.getName()] = 0.0;
    }

    // Filter healthy servers
    std::vector<CDNServer> activeServers;
    for (const auto& s : servers) {
        if (!s.isFailed() && s.getMaximumCapacity() > 0) {
            activeServers.push_back(s);
        }
    }

    if (strategy == RoutingStrategy::MCMF_OPTIMAL || strategy == RoutingStrategy::BELLMAN_FORD_REFERENCE) {
        Graph g;
        g.buildFromServers(servers);
        MinCostMaxFlow solver(g.getNodeCount());

        std::pair<int, long long> res;
        if (strategy == RoutingStrategy::MCMF_OPTIMAL) {
            res = solver.solve(g, 0, 5, demand);
        } else {
            res = solver.solveBellmanFord(g, 0, 5, demand);
        }

        eval.routedTraffic = res.first;
        eval.originTraffic = std::max(0, demand - eval.routedTraffic);

        auto allocs = solver.extractServerAllocations(g);
        for (const auto& kv : allocs) {
            eval.serverAllocations[kv.first] = kv.second;
        }
    } else if (strategy == RoutingStrategy::ROUND_ROBIN) {
        int remainingDemand = demand;
        int activeCount = static_cast<int>(activeServers.size());

        if (activeCount > 0) {
            bool progressMade = true;
            while (remainingDemand > 0 && progressMade) {
                progressMade = false;
                for (auto& s : activeServers) {
                    if (remainingDemand <= 0) break;
                    int curAlloc = eval.serverAllocations[s.getName()];
                    int cap = static_cast<int>(s.getMaximumCapacity());
                    if (curAlloc < cap) {
                        int chunk = std::min(100, std::min(remainingDemand, cap - curAlloc));
                        eval.serverAllocations[s.getName()] += chunk;
                        remainingDemand -= chunk;
                        progressMade = true;
                    }
                }
            }
        }
        eval.routedTraffic = demand - remainingDemand;
        eval.originTraffic = remainingDemand;
    } else if (strategy == RoutingStrategy::NEAREST_GEOGRAPHIC) {
        // Sort active servers by geographic latency ascending
        std::vector<CDNServer> sorted = activeServers;
        std::sort(sorted.begin(), sorted.end(), [](const CDNServer& a, const CDNServer& b) {
            return a.getCurrentLatency() < b.getCurrentLatency();
        });

        int remainingDemand = demand;
        for (const auto& s : sorted) {
            if (remainingDemand <= 0) break;
            int cap = static_cast<int>(s.getMaximumCapacity());
            int alloc = std::min(remainingDemand, cap);
            eval.serverAllocations[s.getName()] = alloc;
            remainingDemand -= alloc;
        }
        eval.routedTraffic = demand - remainingDemand;
        eval.originTraffic = remainingDemand;
    } else if (strategy == RoutingStrategy::GREEDY_CHEAPEST) {
        // Sort active servers by cost: alpha * latency + beta * bwCost ascending
        std::vector<CDNServer> sorted = activeServers;
        std::sort(sorted.begin(), sorted.end(), [this](const CDNServer& a, const CDNServer& b) {
            double costA = (latencyWeight * a.getCurrentLatency()) + (bandwidthWeight * a.getBandwidthCost());
            double costB = (latencyWeight * b.getCurrentLatency()) + (bandwidthWeight * b.getBandwidthCost());
            return costA < costB;
        });

        int remainingDemand = demand;
        for (const auto& s : sorted) {
            if (remainingDemand <= 0) break;
            int cap = static_cast<int>(s.getMaximumCapacity());
            int alloc = std::min(remainingDemand, cap);
            eval.serverAllocations[s.getName()] = alloc;
            remainingDemand -= alloc;
        }
        eval.routedTraffic = demand - remainingDemand;
        eval.originTraffic = remainingDemand;
    }

    // Compute Metrics: Cost, Utilization, Overload count, Latencies
    double costAccum = 0.0;
    double weightedLatencySum = 0.0;
    std::vector<std::pair<double, int>> latencyDistribution;

    for (const auto& s : servers) {
        std::string sName = s.getName();
        int alloc = eval.serverAllocations[sName];
        double cap = s.getMaximumCapacity();
        double util = (cap > 0) ? (alloc / cap) : 0.0;
        eval.serverUtilizations[sName] = util * 100.0;

        if (!s.isFailed() && util >= 0.90) {
            eval.overloadedServersCount++;
        }

        double unitCost = (latencyWeight * s.getCurrentLatency()) + (bandwidthWeight * s.getBandwidthCost());
        costAccum += alloc * unitCost;
        weightedLatencySum += alloc * s.getCurrentLatency();

        if (alloc > 0) {
            latencyDistribution.push_back({s.getCurrentLatency(), alloc});
        }
    }

    // Origin penalty for unserved / overflow traffic
    if (eval.originTraffic > 0) {
        double unitOriginCost = (latencyWeight * originLatency) + originPenaltyCost;
        costAccum += eval.originTraffic * unitOriginCost;
        weightedLatencySum += eval.originTraffic * originLatency;
        latencyDistribution.push_back({originLatency, eval.originTraffic});
    }

    eval.totalCostRs = costAccum;
    eval.avgLatencyMs = (demand > 0) ? (weightedLatencySum / demand) : 0.0;

    // Demand-weighted P95 Latency Calculation
    if (demand > 0 && !latencyDistribution.empty()) {
        std::sort(latencyDistribution.begin(), latencyDistribution.end(),
                  [](const std::pair<double, int>& a, const std::pair<double, int>& b) {
                      return a.first < b.first;
                  });

        double p95Threshold = 0.95 * demand;
        int runningSum = 0;
        eval.p95LatencyMs = latencyDistribution.back().first;
        for (const auto& item : latencyDistribution) {
            runningSum += item.second;
            if (runningSum >= p95Threshold) {
                eval.p95LatencyMs = item.first;
                break;
            }
        }
    }

    return eval;
}

void BaselineEvaluator::printComparisonTable(
    const std::string& scenarioTitle,
    const std::vector<StrategyEvaluation>& evaluations
) {
    std::cout << "\n========================================================================================================================\n";
    std::cout << "                 STRATEGY COMPARISON REPORT: " << scenarioTitle << "\n";
    std::cout << "========================================================================================================================\n";
    std::cout << std::left
              << std::setw(28) << "Routing Strategy"
              << std::setw(15) << "Total Cost(Rs)"
              << std::setw(14) << "Avg Lat(ms)"
              << std::setw(14) << "P95 Lat(ms)"
              << std::setw(18) << "Overloaded Nodes"
              << std::setw(16) << "Origin Miss"
              << std::setw(14) << "Edge Served"
              << "\n";
    std::cout << "------------------------------------------------------------------------------------------------------------------------\n";

    for (const auto& ev : evaluations) {
        std::cout << std::left
                  << std::setw(28) << ev.strategyName
                  << std::setw(15) << std::fixed << std::setprecision(2) << ev.totalCostRs
                  << std::setw(14) << std::setprecision(2) << ev.avgLatencyMs
                  << std::setw(14) << std::setprecision(2) << ev.p95LatencyMs
                  << std::setw(18) << (std::to_string(ev.overloadedServersCount) + " servers")
                  << std::setw(16) << (std::to_string(ev.originTraffic) + " req/s")
                  << std::setw(14) << (std::to_string(ev.routedTraffic) + " req/s")
                  << "\n";
    }
    std::cout << "========================================================================================================================\n";
}

void BaselineEvaluator::printMasterComparisonMatrix(
    const std::vector<std::pair<std::string, std::vector<StrategyEvaluation>>>& scenarioMatrices
) {
    std::cout << "\n====================================================================================================================================\n";
    std::cout << "                     MASTER STRATEGY EVALUATION MATRIX ACROSS ALL TEST SCENARIOS                                    \n";
    std::cout << "====================================================================================================================================\n";
    std::cout << std::left
              << std::setw(24) << "Scenario"
              << std::setw(26) << "Strategy"
              << std::setw(16) << "Total Cost(Rs)"
              << std::setw(14) << "Avg Lat(ms)"
              << std::setw(14) << "P95 Lat(ms)"
              << std::setw(18) << "Overloaded(>90%)"
              << std::setw(16) << "Origin Drop"
              << "\n";
    std::cout << "------------------------------------------------------------------------------------------------------------------------------------\n";

    for (const auto& entry : scenarioMatrices) {
        const std::string& scName = entry.first;
        bool isFirst = true;
        for (const auto& ev : entry.second) {
            std::cout << std::left
                      << std::setw(24) << (isFirst ? scName : "")
                      << std::setw(26) << ev.strategyName
                      << std::setw(16) << std::fixed << std::setprecision(2) << ev.totalCostRs
                      << std::setw(14) << std::setprecision(2) << ev.avgLatencyMs
                      << std::setw(14) << std::setprecision(2) << ev.p95LatencyMs
                      << std::setw(18) << (std::to_string(ev.overloadedServersCount) + " servers")
                      << std::setw(16) << (std::to_string(ev.originTraffic) + " req/s")
                      << "\n";
            isFirst = false;
        }
        std::cout << "------------------------------------------------------------------------------------------------------------------------------------\n";
    }
    std::cout << "====================================================================================================================================\n\n";
}
