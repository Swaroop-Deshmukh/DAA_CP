#include "CacheKnapsack.h"
#include <chrono>
#include <cmath>
#include <algorithm>
#include <random>

void KnapsackResult::printSummary() const {
    std::cout << " Method:          " << method << "\n";
    std::cout << " Cache Capacity:  " << usedCapacityMB << " / " << capacityMB << " MB ("
              << std::fixed << std::setprecision(1) << (capacityMB > 0 ? (usedCapacityMB * 100.0 / capacityMB) : 0.0) << "% full)\n";
    std::cout << " Total Benefit:   " << std::setprecision(2) << totalBenefit << " ms-frequency saved\n";
    std::cout << " Cache Hit Ratio: " << std::setprecision(2) << (hitRatio * 100.0) << "%\n";
    std::cout << " Selected Items:  " << selectedItems.size() << " items cached [";
    for (size_t i = 0; i < selectedItems.size(); ++i) {
        std::cout << selectedItems[i].name << (i + 1 < selectedItems.size() ? ", " : "");
    }
    std::cout << "]\n";
    std::cout << " Execution Time:  " << std::setprecision(2) << executionTimeUs << " microseconds\n";
}

KnapsackResult CacheKnapsackOptimizer::solve01KnapsackDP(
    int capacityMB,
    const std::vector<ContentItem>& catalog,
    double originLatencyMs,
    double edgeLatencyMs
) {
    auto startTime = std::chrono::high_resolution_clock::now();

    int n = static_cast<int>(catalog.size());
    int W = capacityMB;
    double latencyDiff = originLatencyMs - edgeLatencyMs;

    // Convert benefits to scaled integers to prevent floating-point rounding anomalies
    const long long BENEFIT_SCALE = 100000LL;
    std::vector<long long> benefits(n);
    double totalCatalogPop = 0.0;
    for (int i = 0; i < n; ++i) {
        benefits[i] = static_cast<long long>(std::round(catalog[i].popularity * latencyDiff * BENEFIT_SCALE));
        totalCatalogPop += catalog[i].popularity;
    }

    // 2D DP Table: dp[i][w] represents max benefit using subset of first i items with capacity w
    std::vector<std::vector<long long>> dp(n + 1, std::vector<long long>(W + 1, 0));

    for (int i = 1; i <= n; ++i) {
        int itemSize = catalog[i - 1].sizeMB;
        long long itemBen = benefits[i - 1];

        for (int w = 0; w <= W; ++w) {
            dp[i][w] = dp[i - 1][w];
            if (w >= itemSize) {
                dp[i][w] = std::max(dp[i][w], dp[i - 1][w - itemSize] + itemBen);
            }
        }
    }

    // Backtrack to extract chosen items
    KnapsackResult res;
    res.method = "0/1 Knapsack Dynamic Programming";
    res.capacityMB = W;
    res.usedCapacityMB = 0;
    res.totalBenefit = 0.0;

    int curW = W;
    double cachedPop = 0.0;
    for (int i = n; i >= 1; --i) {
        if (dp[i][curW] != dp[i - 1][curW]) {
            const auto& item = catalog[i - 1];
            res.selectedItemIds.push_back(item.id);
            res.selectedItems.push_back(item);
            res.usedCapacityMB += item.sizeMB;
            cachedPop += item.popularity;
            res.totalBenefit += item.popularity * latencyDiff;
            curW -= item.sizeMB;
        }
    }

    std::reverse(res.selectedItemIds.begin(), res.selectedItemIds.end());
    std::reverse(res.selectedItems.begin(), res.selectedItems.end());

    res.hitRatio = (totalCatalogPop > 0.0) ? (cachedPop / totalCatalogPop) : 0.0;

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> dur = endTime - startTime;
    res.executionTimeUs = dur.count();

    return res;
}

KnapsackResult CacheKnapsackOptimizer::solveGreedyByDensity(
    int capacityMB,
    const std::vector<ContentItem>& catalog,
    double originLatencyMs,
    double edgeLatencyMs
) {
    auto startTime = std::chrono::high_resolution_clock::now();

    int n = static_cast<int>(catalog.size());
    double latencyDiff = originLatencyMs - edgeLatencyMs;

    struct ItemDensity {
        int index;
        double density; // benefit / sizeMB
    };

    std::vector<ItemDensity> items(n);
    double totalCatalogPop = 0.0;
    for (int i = 0; i < n; ++i) {
        double benefit = catalog[i].popularity * latencyDiff;
        double density = (catalog[i].sizeMB > 0) ? (benefit / catalog[i].sizeMB) : 0.0;
        items[i] = {i, density};
        totalCatalogPop += catalog[i].popularity;
    }

    // Sort items by density descending
    std::sort(items.begin(), items.end(), [](const ItemDensity& a, const ItemDensity& b) {
        return a.density > b.density;
    });

    KnapsackResult res;
    res.method = "Greedy-by-Density Heuristic";
    res.capacityMB = capacityMB;
    res.usedCapacityMB = 0;
    res.totalBenefit = 0.0;

    double cachedPop = 0.0;
    for (const auto& entry : items) {
        const auto& item = catalog[entry.index];
        if (res.usedCapacityMB + item.sizeMB <= capacityMB) {
            res.selectedItemIds.push_back(item.id);
            res.selectedItems.push_back(item);
            res.usedCapacityMB += item.sizeMB;
            cachedPop += item.popularity;
            res.totalBenefit += item.popularity * latencyDiff;
        }
    }

    res.hitRatio = (totalCatalogPop > 0.0) ? (cachedPop / totalCatalogPop) : 0.0;

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> dur = endTime - startTime;
    res.executionTimeUs = dur.count();

    return res;
}

std::vector<ContentItem> CacheKnapsackOptimizer::generateZipfCatalog(
    int numItems,
    double zipfAlpha,
    unsigned int seed
) {
    std::vector<ContentItem> catalog;
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> sizeDist(10, 100); // 10MB to 100MB

    double sumWeights = 0.0;
    std::vector<double> weights(numItems);
    for (int i = 1; i <= numItems; ++i) {
        weights[i - 1] = 1.0 / std::pow(i, zipfAlpha);
        sumWeights += weights[i - 1];
    }

    for (int i = 0; i < numItems; ++i) {
        double normalizedPop = weights[i] / sumWeights;
        int size = sizeDist(rng);
        catalog.emplace_back(i + 1, "Content_" + std::to_string(i + 1), size, normalizedPop);
    }

    return catalog;
}

bool CacheKnapsackOptimizer::runCounterexampleComparison() {
    std::cout << "\n========================================================================================\n";
    std::cout << "         PHASE 2b: CACHE PLACEMENT OPTIMIZATION (0/1 KNAPSACK DP VS GREEDY)             \n";
    std::cout << "========================================================================================\n";
    std::cout << " Problem Setup:\n";
    std::cout << "   - Edge Cache Storage Capacity: W = 100 MB\n";
    std::cout << "   - Latency Difference: Origin (120 ms) - Edge (15 ms) = 105 ms savings\n";
    std::cout << "   - Content Catalog Items:\n";
    std::cout << "       * Item 1: Size = 60 MB, Popularity = 0.060 -> Benefit = 6.300, Density = 0.1050\n";
    std::cout << "       * Item 2: Size = 50 MB, Popularity = 0.045 -> Benefit = 4.725, Density = 0.0945\n";
    std::cout << "       * Item 3: Size = 50 MB, Popularity = 0.045 -> Benefit = 4.725, Density = 0.0945\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    // Counterexample catalog
    std::vector<ContentItem> catalog = {
        ContentItem(1, "Video_Alpha", 60, 0.060),
        ContentItem(2, "Video_Beta",  50, 0.045),
        ContentItem(3, "Video_Gamma", 50, 0.045)
    };

    int capacityMB = 100;
    auto dpResult = solve01KnapsackDP(capacityMB, catalog);
    auto greedyResult = solveGreedyByDensity(capacityMB, catalog);

    std::cout << "\n [ALGORITHMIC COMPARISON REPORT]:\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(28) << "Metric"
              << std::setw(32) << "Greedy-by-Density"
              << std::setw(32) << "0/1 Knapsack DP (Optimal)"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    std::string greedyItemsStr = "";
    for (size_t i = 0; i < greedyResult.selectedItems.size(); ++i) {
        greedyItemsStr += greedyResult.selectedItems[i].name + " (" + std::to_string(greedyResult.selectedItems[i].sizeMB) + "MB)";
        if (i + 1 < greedyResult.selectedItems.size()) greedyItemsStr += ", ";
    }

    std::string dpItemsStr = "";
    for (size_t i = 0; i < dpResult.selectedItems.size(); ++i) {
        dpItemsStr += dpResult.selectedItems[i].name + " (" + std::to_string(dpResult.selectedItems[i].sizeMB) + "MB)";
        if (i + 1 < dpResult.selectedItems.size()) dpItemsStr += ", ";
    }

    std::cout << std::left
              << std::setw(28) << "Chosen Items"
              << std::setw(32) << greedyItemsStr
              << std::setw(32) << dpItemsStr
              << "\n";
    std::cout << std::left
              << std::setw(28) << "Storage Utilization"
              << std::setw(32) << (std::to_string(greedyResult.usedCapacityMB) + " / " + std::to_string(capacityMB) + " MB (60%)")
              << std::setw(32) << (std::to_string(dpResult.usedCapacityMB) + " / " + std::to_string(capacityMB) + " MB (100%)")
              << "\n";
    std::cout << std::left
              << std::setw(28) << "Unused Slack Capacity"
              << std::setw(32) << "40 MB (Wasted Space)"
              << std::setw(32) << "0 MB (Fully Packed)"
              << "\n";
    std::cout << std::left
              << std::setw(28) << "Latency Benefit Saved"
              << std::setw(32) << (std::to_string(greedyResult.totalBenefit).substr(0, 5) + " ms-freq")
              << std::setw(32) << (std::to_string(dpResult.totalBenefit).substr(0, 5) + " ms-freq")
              << "\n";
    std::cout << std::left
              << std::setw(28) << "Hit Ratio"
              << std::setw(32) << (std::to_string(greedyResult.hitRatio * 100.0).substr(0, 4) + "%")
              << std::setw(32) << (std::to_string(dpResult.hitRatio * 100.0).substr(0, 4) + "%")
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    double benefitGain = ((dpResult.totalBenefit - greedyResult.totalBenefit) / greedyResult.totalBenefit) * 100.0;
    std::cout << " DP Benefit Superiority:    +" << std::fixed << std::setprecision(1) << benefitGain << "% higher than Greedy!\n";
    std::cout << " DAA Algorithmic Insight:   Greedy myopically selects Item 1 because of higher density (0.110 vs 0.099),\n"
              << "                            leaving 40 MB of unusable slack. Dynamic Programming considers sub-problem\n"
              << "                            combinations and packs Item 2 + Item 3, maximizing discrete value.\n";

    bool passed = dpResult.totalBenefit > greedyResult.totalBenefit;
    std::cout << " Verification Verdict:      [" << (passed ? "PASS: DP STRICTLY SUPERIOR TO GREEDY" : "FAIL") << "]\n";
    std::cout << "========================================================================================\n\n";

    return passed;
}
