#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "Graph.h"
#include "MinCostMaxFlow.h"
#include "CDNServer.h"
#include <vector>
#include <string>
#include <chrono>

/**
 * Benchmark record for a specific traffic demand.
 */
struct BenchmarkRecord {
    int demand;
    int flowRouted;
    double totalCostRs;
    int augmentations;
    double graphBuildTimeUs;
    double avgDijkstraTimeUs;
    double totalMcmfTimeUs;
    double totalMcmfTimeMs;
};

/**
 * Performance benchmarking engine for DAA analysis.
 */
class BenchmarkSuite {
private:
    std::vector<CDNServer> defaultServers;

    std::vector<CDNServer> createDefaultServers() const;

public:
    BenchmarkSuite();

    // Micro-benchmarks
    double benchmarkGraphConstruction(int iterations = 10000);
    double benchmarkDijkstra(int iterations = 10000);

    // Multi-tier demand MCMF benchmark
    BenchmarkRecord benchmarkDemand(int demand, int repetitions = 100);
    std::vector<BenchmarkRecord> runFullDemandSuite(const std::vector<int>& demands = {1000, 5000, 10000, 20000, 50000});

    // Formatting & reporting
    void printReport(const std::vector<BenchmarkRecord>& records) const;
    void printTheoreticalComplexityAnalysis() const;
};

#endif // BENCHMARK_H
