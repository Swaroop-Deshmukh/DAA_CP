#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "Graph.h"
#include "MinCostMaxFlow.h"
#include "CDNServer.h"
#include "Router.h"
#include <vector>
#include <string>
#include <chrono>

/**
 * Benchmark record for a specific traffic demand.
 * Microsecond precision with full breakdown and min/max/avg statistics.
 */
struct BenchmarkRecord {
    int demand;
    int flowRouted;
    double totalCostRs;
    int augmentations;
    double graphBuildAvgUs;
    double dijkstraAvgUs;
    double mcmfAvgUs;
    double mcmfMinUs;
    double mcmfMaxUs;
    double totalRoutingAvgUs;
    double totalRoutingMinUs;
    double totalRoutingMaxUs;
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

    // Warmup routines to eliminate cache cold-start artifacts
    void warmup(int iterations = 100);

    // Multi-tier demand MCMF benchmark with batch sampling
    BenchmarkRecord benchmarkDemand(int demand, int batches = 25, int batchSize = 20);
    std::vector<BenchmarkRecord> runFullDemandSuite(const std::vector<int>& demands = {1000, 5000, 10000, 20000, 50000});

    // Formatting & reporting
    void printReport(const std::vector<BenchmarkRecord>& records) const;
    void printTheoreticalComplexityAnalysis() const;

    // File exports to results/ directory
    void exportResultsToCsv(const std::string& filepath, const std::vector<BenchmarkRecord>& records) const;
    void exportComplexityAnalysisMarkdown(const std::string& filepath) const;
};

#endif // BENCHMARK_H
