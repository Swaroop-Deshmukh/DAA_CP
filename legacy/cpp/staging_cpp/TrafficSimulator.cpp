#include "TrafficSimulator.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <random>

std::string scenarioToString(ScenarioType scenario) {
    switch (scenario) {
        case ScenarioType::NORMAL:              return "NORMAL";
        case ScenarioType::TRAFFIC_SPIKE:       return "TRAFFIC_SPIKE";
        case ScenarioType::PUNE_CONGESTION:     return "PUNE_CONGESTION";
        case ScenarioType::MUMBAI_FAILURE:      return "MUMBAI_FAILURE";
        case ScenarioType::MULTIPLE_CONGESTION: return "MULTIPLE_CONGESTION";
        case ScenarioType::OVERLOAD:            return "OVERLOAD";
        case ScenarioType::RECOVERY:            return "RECOVERY";
        default:                                return "UNKNOWN";
    }
}

TrafficSimulator::TrafficSimulator() : currentScenario(ScenarioType::NORMAL), tickCount(0) {
    initializeDefaultFleet();
}

void TrafficSimulator::initializeDefaultFleet() {
    servers.clear();
    // 1: Pune
    servers.emplace_back(1, "Pune", "West India", 5000.0, 25.0, 8.00, 3200.0, 1000.0);
    // 2: Mumbai
    servers.emplace_back(2, "Mumbai", "West Coastal", 6000.0, 18.0, 6.00, 3800.0, 1200.0);
    // 3: Delhi
    servers.emplace_back(3, "Delhi", "North India", 5500.0, 32.0, 9.00, 3400.0, 1100.0);
    // 4: Bangalore
    servers.emplace_back(4, "Bangalore", "South Tech Hub", 5000.0, 24.0, 7.50, 3100.0, 1000.0);

    currentScenario = ScenarioType::NORMAL;
    tickCount = 0;
}

void TrafficSimulator::applyScenario(ScenarioType scenario) {
    currentScenario = scenario;

    switch (scenario) {
        case ScenarioType::NORMAL: {
            for (auto& s : servers) {
                if (s.isFailed()) s.recover();
            }
            getServer("Pune")->setTraffic(3200.0);
            getServer("Mumbai")->setTraffic(3800.0);
            getServer("Delhi")->setTraffic(3400.0);
            getServer("Bangalore")->setTraffic(3100.0);
            break;
        }

        case ScenarioType::TRAFFIC_SPIKE: {
            // Sudden surge pushing Pune close to max capacity (4900 req/s)
            getServer("Pune")->setTraffic(4900.0);
            getServer("Mumbai")->setTraffic(4200.0);
            getServer("Delhi")->setTraffic(3800.0);
            getServer("Bangalore")->setTraffic(3500.0);
            break;
        }

        case ScenarioType::PUNE_CONGESTION: {
            // Pune is heavily congested (>98% load) causing latency to escalate to ~120ms
            getServer("Pune")->setTraffic(4900.0);
            getServer("Pune")->updateMetrics(false);
            break;
        }

        case ScenarioType::MUMBAI_FAILURE: {
            // Mumbai crashes completely
            getServer("Mumbai")->fail();
            break;
        }

        case ScenarioType::MULTIPLE_CONGESTION: {
            // Both Pune and Delhi experience heavy queuing congestion
            getServer("Pune")->setTraffic(4900.0);
            getServer("Delhi")->setTraffic(5400.0);
            getServer("Pune")->updateMetrics(false);
            getServer("Delhi")->updateMetrics(false);
            break;
        }

        case ScenarioType::OVERLOAD: {
            // All servers operate at high capacity; total demand will exceed fleet limit
            for (auto& s : servers) {
                if (s.isFailed()) s.recover();
                s.setTraffic(s.getMaximumCapacity() * 0.95);
            }
            break;
        }

        case ScenarioType::RECOVERY: {
            // All servers recover back to nominal healthy baseline
            for (auto& s : servers) {
                if (s.isFailed()) s.recover();
            }
            getServer("Pune")->setTraffic(3200.0);
            getServer("Mumbai")->setTraffic(3800.0);
            getServer("Delhi")->setTraffic(3400.0);
            getServer("Bangalore")->setTraffic(3100.0);
            break;
        }
    }
}

void TrafficSimulator::tick(bool applyStochasticDrift) {
    tickCount++;

    if (applyStochasticDrift) {
        static std::mt19937 gen(42 + tickCount);
        std::uniform_real_distribution<> dis(-0.02, 0.02); // +/- 2% drift

        for (auto& s : servers) {
            if (!s.isFailed() && !s.isCongested()) {
                double drift = dis(gen);
                double newTraffic = std::max(100.0, s.getCurrentRequests() * (1.0 + drift));
                s.setTraffic(newTraffic);
            } else {
                s.updateMetrics(true);
            }
        }
    } else {
        for (auto& s : servers) {
            s.updateMetrics(false);
        }
    }
}

void TrafficSimulator::setServerTraffic(const std::string& name, double requests) {
    CDNServer* s = getServer(name);
    if (s) {
        s->setTraffic(requests);
    }
}

void TrafficSimulator::triggerSpike(const std::string& name, double targetTraffic) {
    CDNServer* s = getServer(name);
    if (s) {
        s->setTraffic(targetTraffic);
    }
}

void TrafficSimulator::failServer(const std::string& name) {
    CDNServer* s = getServer(name);
    if (s) {
        s->fail();
    }
}

void TrafficSimulator::recoverServer(const std::string& name, double initialTraffic) {
    CDNServer* s = getServer(name);
    if (s) {
        s->recover(initialTraffic);
    }
}

CDNServer* TrafficSimulator::getServer(const std::string& name) {
    for (auto& s : servers) {
        if (s.getName() == name) return &s;
    }
    return nullptr;
}

const CDNServer* TrafficSimulator::getServer(const std::string& name) const {
    for (const auto& s : servers) {
        if (s.getName() == name) return &s;
    }
    return nullptr;
}

double TrafficSimulator::getTotalCapacity() const {
    double total = 0.0;
    for (const auto& s : servers) {
        if (!s.isFailed()) total += s.getMaximumCapacity();
    }
    return total;
}

double TrafficSimulator::getTotalTraffic() const {
    double total = 0.0;
    for (const auto& s : servers) {
        total += s.getCurrentRequests();
    }
    return total;
}

double TrafficSimulator::getTotalBandwidth() const {
    double total = 0.0;
    for (const auto& s : servers) {
        total += s.getBandwidthUsage();
    }
    return total;
}

double TrafficSimulator::getAverageLatency() const {
    double sum = 0.0;
    int count = 0;
    for (const auto& s : servers) {
        if (!s.isFailed()) {
            sum += s.getCurrentLatency();
            count++;
        }
    }
    return count > 0 ? (sum / count) : 0.0;
}

int TrafficSimulator::getActiveServerCount() const {
    int count = 0;
    for (const auto& s : servers) {
        if (!s.isFailed()) count++;
    }
    return count;
}

void TrafficSimulator::printFleetSummary() const {
    std::cout << "\n--- Fleet Telemetry Summary [Tick #" << tickCount << " | Scenario: " 
              << scenarioToString(currentScenario) << "] ---\n";
    std::cout << " Active Nodes: " << getActiveServerCount() << "/" << servers.size()
              << " | Total Capacity: " << (int)getTotalCapacity() << " req/s"
              << " | Total Traffic: " << (int)getTotalTraffic() << " req/s"
              << " | Bandwidth: " << std::fixed << std::setprecision(1) << getTotalBandwidth() << " Mbps"
              << " | Avg Latency: " << std::fixed << std::setprecision(1) << getAverageLatency() << " ms\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    for (const auto& s : servers) {
        s.printDetails();
    }
    std::cout << "----------------------------------------------------------------------------------------\n";
}
