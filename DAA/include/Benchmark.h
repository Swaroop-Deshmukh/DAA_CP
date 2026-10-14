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
 * Phase 4 Requirement: Scalability Benchmark Record on Seeded Random Networks.
 * Captures graph size (V, E), augmentations (K), Dijkstra vs Bellman-Ford,
 * and theoretical asymptotic curve overlay O(K * E log V).
 */
struct ScalabilityRecord {
    int serverCount;
    int regionCount;
    int vertexCount;
    int edgeCount;
    int augmentationsK;
    int dijkstraRuns;
    int totalDemand;
    int routedFlow;
    double medianBuildUs;
    double medianDijkstraSolveUs;
    double medianBellmanFordSolveUs;
    double medianPrimalDualSolveUs;
    double bfVsDijkstraRatio;          // Bellman-Ford time / Dijkstra time
    double theoreticalComplexity;      // K * E * log2(V)
    double measuredTheoreticalRatio;    // medianDijkstraSolveUs / (K * E * log2(V))
    double ratioNsPerOp;               // (medianDijkstraSolveUs * 1000.0) / (K * E * log2(V))
};

struct AdversarialRecord {
    int vertexCount;
    int edgeCount;
    int augmentationsK;
    double dijkstraSolveUs;
    double bellmanFordSolveUs;
    double bfVsDijkstraRatio;
    std::string description;
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

    // Phase 4: Random-Graph Scalability Benchmark Suite (10, 50, 100, 500, 1000 servers)
    ScalabilityRecord benchmarkRandomNetwork(int numServers, int repetitions = 5, unsigned int seed = 42);
    AdversarialRecord benchmarkAdversarialNetwork(int numStages = 25, int width = 4);
    std::vector<ScalabilityRecord> runScalabilityBenchmark(
        const std::vector<int>& serverSizes = {10, 50, 100, 500, 1000},
        int repetitions = 5
    );
    void printScalabilityReport(const std::vector<ScalabilityRecord>& records, const AdversarialRecord& advRecord = {}) const;
    void exportScalabilityCsv(const std::string& filepath, const std::vector<ScalabilityRecord>& records, const AdversarialRecord& advRecord = {}) const;


    // Relabeled Legacy Fixed-Topology Demand Benchmark (1k-50k req/s)
    BenchmarkRecord benchmarkDemand(int demand, int batches = 25, int batchSize = 20);
    std::vector<BenchmarkRecord> runFullDemandSuite(const std::vector<int>& demands = {1000, 5000, 10000, 20000, 50000});

    // Formatting & reporting
    void printReport(const std::vector<BenchmarkRecord>& records) const;
    void printTheoreticalComplexityAnalysis() const;

    // File exports to results/ directory
    void exportResultsToCsv(const std::string& filepath, const std::vector<BenchmarkRecord>& records) const;
    void exportComplexityAnalysisMarkdown(const std::string& filepath, const std::vector<ScalabilityRecord>& scalRecords = {}) const;
};

#endif // BENCHMARK_H
