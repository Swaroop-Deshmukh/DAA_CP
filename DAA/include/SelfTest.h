#ifndef SELF_TEST_H
#define SELF_TEST_H

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

/**
 * Result record for an individual Phase 3 invariant check.
 */
struct CheckResult {
    std::string id;
    std::string name;
    bool passed;
    std::string details;
    double executionTimeMs;

    CheckResult(
        const std::string& checkId = "",
        const std::string& checkName = "",
        bool pass = false,
        const std::string& d = "",
        double t = 0.0
    ) : id(checkId), name(checkName), passed(pass), details(d), executionTimeMs(t) {}
};

/**
 * Phase 3 Correctness & Automated Invariant Verification Engine.
 * 
 * Implements rigorous self-tests matching every requirement in UPGRADE_BRIEF.md:
 * 1. Flow conservation at all non-terminal nodes & 0 <= flow <= capacity on every edge.
 * 2. Reduced costs strictly non-negative (c^pi >= 0) after every potential update.
 * 3. Dijkstra+potentials total cost == Bellman-Ford total cost on 1000+ random instances.
 * 4. Brute-force integer flow enumeration comparison on tiny instances.
 * 5. Edmonds-Karp max-flow == min-cut capacity == MCMF flow (without origin fallback).
 * 6. 0/1 Knapsack DP result == brute-force subset enumeration on small catalogs.
 * 7. Total demand equals edge-served traffic plus origin fallback traffic.
 */
class SelfTestRunner {
public:
    static bool runAllSelfTests();

    static CheckResult check1_FlowConservationAndCapacityBounds();
    static CheckResult check2_ReducedCostsNonNegative();
    static CheckResult check3_DijkstraVsBellmanFord1000Instances();
    static CheckResult check4_BruteForceFlowEnumerationTinyInstances();
    static CheckResult check5_MaxFlowMinCutEquivalence();
    static CheckResult check6_KnapsackDPVsBruteForceSubsets();
    static CheckResult check7_TotalDemandConservation();
};

#endif // SELF_TEST_H
