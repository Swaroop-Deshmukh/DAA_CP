#ifndef HOLT_FORECAST_H
#define HOLT_FORECAST_H

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>

/**
 * Holt's Linear Exponential Smoother (Double Exponential Smoothing).
 * Tracks Level and Trend with O(1) state updates per observation.
 */
class HoltLinearSmoother {
private:
    double alpha;       // Level smoothing parameter (0 < alpha < 1)
    double beta;        // Trend smoothing parameter (0 < beta < 1)
    double level;       // Current smoothed level L_t
    double trend;       // Current smoothed trend T_t
    bool initialized;

public:
    HoltLinearSmoother(double a = 0.4, double b = 0.3);

    // O(1) state update with newly observed traffic demand
    void update(double observedDemand);

    // k-step-ahead future forecast: forecast_k = level + k * trend
    double forecast(int k = 1) const;

    double getLevel() const { return level; }
    double getTrend() const { return trend; }
    bool isInitialized() const { return initialized; }
    void reset();
};

/**
 * Result record for Reactive vs Proactive spike comparison.
 */
struct SpikeComparisonRecord {
    std::string mode;
    double peakUtilizationPct;
    int ticksOverloaded;            // Number of cycles with server load >= 90%
    int ticksToRecover;             // Cycles from peak until fleet returns to safe load
    double cumulativeCostRs;
    int totalOriginDropped;
};

/**
 * Spike Scenario Simulation Runner comparing Reactive vs Proactive routing.
 */
class SpikeForecastRunner {
public:
    static std::pair<SpikeComparisonRecord, SpikeComparisonRecord> runSpikeComparison();
};

#endif // HOLT_FORECAST_H
