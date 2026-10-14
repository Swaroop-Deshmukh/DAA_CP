#ifndef CACHE_KNAPSACK_H
#define CACHE_KNAPSACK_H

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>

/**
 * Content Item in CDN catalog.
 */
struct ContentItem {
    int id;
    std::string name;
    int sizeMB;             // Size in Megabytes
    double popularity;      // Zipf request probability (sum to 1.0 or frequency)

    ContentItem(int itemId, const std::string& n, int size, double pop)
        : id(itemId), name(n), sizeMB(size), popularity(pop) {}
};

/**
 * Result record from Knapsack optimization.
 */
struct KnapsackResult {
    std::string method;             // "0/1 Knapsack DP" or "Greedy-by-Density"
    int capacityMB;
    int usedCapacityMB;
    double totalBenefit;            // Sum of popularity * (originLatency - edgeLatency)
    double hitRatio;                // Cached popularity / Total catalog popularity
    std::vector<int> selectedItemIds;
    std::vector<ContentItem> selectedItems;
    double executionTimeUs;

    void printSummary() const;
};

/**
 * 0/1 Knapsack Dynamic Programming vs Greedy-by-Density Engine
 * for CDN Edge Server Cache Placement.
 */
class CacheKnapsackOptimizer {
public:
    /**
     * Exact 0/1 Knapsack using Dynamic Programming with full backtracking.
     * Time Complexity: O(N * W), Space Complexity: O(N * W).
     */
    static KnapsackResult solve01KnapsackDP(
        int capacityMB,
        const std::vector<ContentItem>& catalog,
        double originLatencyMs = 120.0,
        double edgeLatencyMs = 15.0
    );

    /**
     * Greedy-by-Density heuristic: Sorts by benefit / sizeMB and fills greedily.
     * Time Complexity: O(N log N).
     */
    static KnapsackResult solveGreedyByDensity(
        int capacityMB,
        const std::vector<ContentItem>& catalog,
        double originLatencyMs = 120.0,
        double edgeLatencyMs = 15.0
    );

    /**
     * Generates a realistic Zipf-distributed content catalog.
     * P(i) proportional to 1 / (i ^ alpha) with seeded RNG.
     */
    static std::vector<ContentItem> generateZipfCatalog(
        int numItems = 50,
        double zipfAlpha = 0.8,
        unsigned int seed = 42
    );

    /**
     * Phase 2b Acceptance Requirement:
     * Demonstrates a concrete instance where Greedy-by-Density is strictly
     * sub-optimal compared to the exact 0/1 Knapsack DP.
     */
    static bool runCounterexampleComparison();
};

#endif // CACHE_KNAPSACK_H
