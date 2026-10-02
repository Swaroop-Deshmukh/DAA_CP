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
 * TrafficSimulator manages the edge server fleet and models
 * realistic traffic dynamics, surges, queuing congestion, and outages.
 */
class TrafficSimulator {
private:
    std::vector<CDNServer> servers;
    ScenarioType currentScenario;
    int tickCount;

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
    int getActiveServerCount() const;

    void printFleetSummary() const;
};

#endif // TRAFFIC_SIMULATOR_H
