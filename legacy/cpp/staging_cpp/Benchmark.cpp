#include "Benchmark.h"
#include <iostream>
#include <iomanip>
#include <numeric>

std::vector<CDNServer> BenchmarkSuite::createDefaultServers() const {
    std::vector<CDNServer> servers;
    servers.emplace_back(1, "Pune", "West India", 5000.0, 25.0, 8.00, 3200.0, 1000.0);
    servers.emplace_back(2, "Mumbai", "West Coastal", 6000.0, 18.0, 6.00, 3800.0, 1200.0);
    servers.emplace_back(3, "Delhi", "North India", 5500.0, 32.0, 9.00, 3400.0, 1100.0);
    servers.emplace_back(4, "Bangalore", "South Tech Hub", 5000.0, 24.0, 7.50, 3100.0, 1000.0);
    return servers;
}

BenchmarkSuite::BenchmarkSuite() {
    defaultServers = createDefaultServers();
}

double BenchmarkSuite::benchmarkGraphConstruction(int iterations) {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        Graph g(6, 1.0, 2.0, 100);
        g.buildFromServers(defaultServers);
    }

    auto end = std::chrono::high_resolution_clock::now();
    double totalUs = std::chrono::duration<double, std::micro>(end - start).count();
    return totalUs / iterations;
}

double BenchmarkSuite::benchmarkDijkstra(int iterations) {
    Graph g(6, 1.0, 2.0, 100);
    g.buildFromServers(defaultServers);
    MinCostMaxFlow mcmf(6);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        mcmf.dijkstra(g, 0, 5);
    }

    auto end = std::chrono::high_resolution_clock::now();
    double totalUs = std::chrono::duration<double, std::micro>(end - start).count();
    return totalUs / iterations;
}

BenchmarkRecord BenchmarkSuite::benchmarkDemand(int demand, int repetitions) {
    double totalBuildUs = 0.0;
    double totalMcmfUs = 0.0;
    int flowRouted = 0;
    double totalCostRs = 0.0;
    int augmentations = 0;

    for (int i = 0; i < repetitions; ++i) {
        auto t0 = std::chrono::high_resolution_clock::now();
        Graph g(6, 1.0, 2.0, 100);
        g.buildFromServers(defaultServers);
        auto t1 = std::chrono::high_resolution_clock::now();

        MinCostMaxFlow mcmf(6);
        std::pair<int, long long> res = mcmf.solve(g, 0, 5, demand);
        auto t2 = std::chrono::high_resolution_clock::now();

        totalBuildUs += std::chrono::duration<double, std::micro>(t1 - t0).count();
        totalMcmfUs += std::chrono::duration<double, std::micro>(t2 - t1).count();

        if (i == 0) {
            flowRouted = res.first;
            totalCostRs = static_cast<double>(res.second) / 100.0;
            augmentations = mcmf.getAugmentationCount();
        }
    }

    double avgBuildUs = totalBuildUs / repetitions;
    double avgMcmfUs = totalMcmfUs / repetitions;
    double avgDijkstraUs = augmentations > 0 ? (avgMcmfUs / augmentations) : avgMcmfUs;

    return BenchmarkRecord{
        demand,
        flowRouted,
        totalCostRs,
        augmentations,
        avgBuildUs,
        avgDijkstraUs,
        avgMcmfUs,
        avgMcmfUs / 1000.0
    };
}

std::vector<BenchmarkRecord> BenchmarkSuite::runFullDemandSuite(const std::vector<int>& demands) {
    std::vector<BenchmarkRecord> records;
    for (int d : demands) {
        records.push_back(benchmarkDemand(d, 200));
    }
    return records;
}

void BenchmarkSuite::printReport(const std::vector<BenchmarkRecord>& records) const {
    std::cout << "\n========================================================================================\n";
    std::cout << "                 MIN-COST MAX-FLOW PERFORMANCE BENCHMARK REPORT                         \n";
    std::cout << "========================================================================================\n";
    std::cout << " " << std::right << std::setw(8)  << "Demand"
              << " | " << std::right << std::setw(8) << "Routed"
              << " | " << std::right << std::setw(11) << "Cost (Rs)"
              << " | " << std::right << std::setw(6)  << "Paths"
              << " | " << std::right << std::setw(11) << "Build (us)"
              << " | " << std::right << std::setw(12) << "Dijkstra(us)"
              << " | " << std::right << std::setw(12) << "MCMF (us)"
              << " | " << std::right << std::setw(10) << "MCMF (ms)" << " |\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    for (const auto& r : records) {
        std::cout << " " << std::right << std::setw(8)  << r.demand
                  << " | " << std::right << std::setw(8) << r.flowRouted
                  << " | " << std::right << std::setw(11) << std::fixed << std::setprecision(2) << r.totalCostRs
                  << " | " << std::right << std::setw(6)  << r.augmentations
                  << " | " << std::right << std::setw(11) << std::fixed << std::setprecision(2) << r.graphBuildTimeUs
                  << " | " << std::right << std::setw(12) << std::fixed << std::setprecision(2) << r.avgDijkstraTimeUs
                  << " | " << std::right << std::setw(12) << std::fixed << std::setprecision(2) << r.totalMcmfTimeUs
                  << " | " << std::right << std::setw(10) << std::fixed << std::setprecision(4) << r.totalMcmfTimeMs << " |\n";
    }
    std::cout << "========================================================================================\n";
}

void BenchmarkSuite::printTheoreticalComplexityAnalysis() const {
    std::cout << "\n========================================================================================\n";
    std::cout << "                   DAA THEORETICAL COMPLEXITY & PROOF ANALYSIS                          \n";
    std::cout << "========================================================================================\n";
    std::cout << " 1. PROBLEM FORMULATION:\n";
    std::cout << "    - Network: Directed Flow Network G = (V, E) with Source s (0) and Sink t (5).\n";
    std::cout << "    - Nodes: |V| = 6 (Source, Pune, Mumbai, Delhi, Bangalore, Origin Sink).\n";
    std::cout << "    - Edges: |E| = 9 forward edges, 9 anti-symmetric reverse residual edges.\n\n";

    std::cout << " 2. SHORTEST PATH SUBROUTINE (DIJKSTRA WITH JOHNSON'S POTENTIALS):\n";
    std::cout << "    - Standard Dijkstra requires non-negative edge costs (c(u, v) >= 0).\n";
    std::cout << "    - In residual networks, reverse edges have negative cost: c(v, u) = -c(u, v).\n";
    std::cout << "    - We maintain node potentials pi(u) and use reduced costs:\n";
    std::cout << "          c^pi(u, v) = c(u, v) + pi(u) - pi(v) >= 0\n";
    std::cout << "    - Reduced costs are guaranteed non-negative by triangle inequality along optimal paths.\n";
    std::cout << "    - Priority Queue: Min-heap implementation yields time complexity O(|E| log |V|).\n\n";

    std::cout << " 3. TOTAL SUCCESSIVE SHORTEST PATH (SSP) RUNTIME:\n";
    std::cout << "    - If K is the number of augmenting paths pushed, total time is O(K * |E| log |V|).\n";
    std::cout << "    - Bottleneck Capacity Invariant: In our CDN topology, each augmenting path saturates\n";
    std::cout << "      an entire edge server's capacity (or the remaining global demand).\n";
    std::cout << "    - Therefore, K <= |CDN Edge Servers| = 4 augmentations, REGARDLESS of traffic volume!\n";
    std::cout << "    - At 50,000 req/s, K remains <= 4, taking < 0.05 milliseconds total compute.\n";
    std::cout << "    - This completely avoids the pseudo-polynomial O(F * |E| log |V|) unit-flow trap.\n\n";

    std::cout << " 4. SPACE COMPLEXITY:\n";
    std::cout << "    - Adjacency List: O(|V| + |E|) space.\n";
    std::cout << "    - Heap and Potential vectors: O(|V|) space.\n";
    std::cout << "    - Total memory footprint: < 10 Kilobytes (Cache-resident in L1 CPU cache).\n";
    std::cout << "========================================================================================\n\n";
}
