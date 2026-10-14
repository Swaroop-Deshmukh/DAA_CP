#ifndef TRAFFIC_SIMULATOR_H
#define TRAFFIC_SIMULATOR_H

#include "CDNServer.h"
#include <vector>
#include <memory>
#include <string>

/**
 * Supported simulation scenario states.
 */
enum class ScenarioType {
    NORMAL,
    TRAFFIC_SPIKE,
    PUNE_CONGESTION,
    MUMBAI_FAILURE,
    MULTIPLE_CONGESTION,
    OVERLOAD,
    RECOVERY
};

std::string scenarioToString(ScenarioType scenario);

/**
 * Event log entry recorded by the C++ simulation engine.
 */
struct SimulatorEvent {
    int tick;
    std::string timestamp;
    std::string source;
    std::string message;
    std::string level; // "INFO", "WARNING", "CRITICAL", "SUCCESS"
};

/**
 * TrafficSimulator manages the edge server fleet and models
 * realistic traffic dynamics, surges, queuing congestion, and outages.
 *
 * Single Source of Truth for:
 * - Poisson request arrival fluctuations
 * - Diurnal 24-hour sinusoidal demand cycles
 * - Stochastic random spikes & controlled surges
 * - Server queuing latency (M/M/1), CPU, and bandwidth utilization
 */
class TrafficSimulator {
private:
    std::vector<CDNServer> servers;
    ScenarioType currentScenario;
    int tickCount;

    // Single-source-of-truth physics parameters
    bool diurnalEnabled;
    bool randomSpikesEnabled;
    int diurnalPeriodTicks;
    double diurnalAmplitude;
    std::vector<std::pair<std::string, double>> baseDemands;
    std::vector<SimulatorEvent> eventsLog;
    std::vector<ServerStatus> prevStatuses;

    std::string getCurrentTimeString() const;

public:
    TrafficSimulator();

    void initializeDefaultFleet();
    void applyScenario(ScenarioType scenario);
    void tick(bool applyStochasticDrift = true);

    // Direct controls
    void setServerTraffic(const std::string& name, double requests);
    void triggerSpike(const std::string& name, double targetTraffic);
    void failServer(const std::string& name);
    void recoverServer(const std::string& name, double initialTraffic = -1.0);

    // Physics configuration
    void setDiurnalEnabled(bool enabled) { diurnalEnabled = enabled; }
    bool isDiurnalEnabled() const { return diurnalEnabled; }
    void setRandomSpikesEnabled(bool enabled) { randomSpikesEnabled = enabled; }
    bool isRandomSpikesEnabled() const { return randomSpikesEnabled; }
    double getDiurnalMultiplier(int tick) const;
    void logEvent(const std::string& source, const std::string& message, const std::string& level = "INFO");

    // Getters & Aggregations
    std::vector<CDNServer>& getServers() { return servers; }
    const std::vector<CDNServer>& getServers() const { return servers; }
    CDNServer* getServer(const std::string& name);
    const CDNServer* getServer(const std::string& name) const;
    ScenarioType getCurrentScenario() const { return currentScenario; }
    int getTickCount() const { return tickCount; }

    double getTotalCapacity() const;
    double getTotalTraffic() const;
    double getTotalBandwidth() const;
    double getAverageLatency() const;
    double getAverageCpu() const;
    int getActiveServerCount() const;
    std::string getFleetHealth() const;
    const std::vector<SimulatorEvent>& getEventsLog() const { return eventsLog; }

    // JSON export (Single Source of Truth telemetry representation)
    std::string getTelemetryJson() const;

    void printFleetSummary() const;
};

#endif // TRAFFIC_SIMULATOR_H
