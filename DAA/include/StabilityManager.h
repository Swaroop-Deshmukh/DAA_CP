#ifndef STABILITY_MANAGER_H
#define STABILITY_MANAGER_H

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

/**
 * Result record from an individual routing stability decision.
 */
struct StabilityDecision {
    bool rerouted;
    double improvementPct;
    int churnVolumeReqs;
    double finalCostRs;
    std::string decisionReason;
    std::map<std::string, int> chosenAllocation;
};

/**
 * Result summary across a 100-tick stability benchmark.
 */
struct StabilityExperimentRecord {
    std::string mode;
    int totalCycles;
    int routeChangesCount;          // Number of cycles where traffic shifted
    long long totalTrafficMigrated; // Cumulative sum of |new_alloc - prev_alloc| (req/s)
    double cumulativeCostRs;
    double averageChurnPerTick;     // Average req/s displaced per tick
};

/**
 * Hysteresis and Migration Penalty Controller.
 * Suppresses route chattering and connection flapping under stochastic jitter.
 */
class HysteresisController {
private:
    double hysteresisThreshold;     // e.g. 0.05 (requires >5% improvement to reroute)
    double migrationPenaltyWeight;   // e.g. 0.02 Rs per unit of migrated flow
    bool hasPrevious;
    std::map<std::string, int> previousAllocation;
    double previousCost;

public:
    HysteresisController(double threshold = 0.05, double migrationWeight = 0.01);

    void reset();

    StabilityDecision processCandidateAllocation(
        const std::map<std::string, int>& candidateAlloc,
        double candidateCost,
        double currentCostUnderPrevAlloc,
        bool applyHysteresis = true
    );

    static int computeAllocationChurn(
        const std::map<std::string, int>& allocA,
        const std::map<std::string, int>& allocB
    );
};

/**
 * 100-Tick Experiment Runner for verifying Router Stability.
 */
class StabilityExperimentRunner {
public:
    static std::pair<StabilityExperimentRecord, StabilityExperimentRecord> run100TickStabilityComparison(
        int totalTicks = 100,
        unsigned int seed = 42
    );
};

#endif // STABILITY_MANAGER_H
