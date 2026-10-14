#include "TrafficSimulator.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <random>
#include <ctime>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

std::string TrafficSimulator::getCurrentTimeString() const {
    std::time_t now = std::time(nullptr);
    std::tm* tmNow = std::localtime(&now);
    char buf[32];
    if (tmNow) {
        std::strftime(buf, sizeof(buf), "%H:%M:%S", tmNow);
    } else {
        return "12:00:00";
    }
    return std::string(buf);
}

TrafficSimulator::TrafficSimulator()
    : currentScenario(ScenarioType::NORMAL),
      tickCount(0),
      diurnalEnabled(true),
      randomSpikesEnabled(false),
      diurnalPeriodTicks(24),
      diurnalAmplitude(0.20)
{
    initializeDefaultFleet();
}

void TrafficSimulator::initializeDefaultFleet() {
    servers.clear();
    baseDemands.clear();
    prevStatuses.clear();
    eventsLog.clear();

    // 1: Pune
    servers.emplace_back(1, "Pune", "West India", 5000.0, 25.0, 8.00, 3200.0, 1000.0);
    baseDemands.push_back({"Pune", 3200.0});

    // 2: Mumbai
    servers.emplace_back(2, "Mumbai", "West Coastal", 6000.0, 18.0, 6.00, 3800.0, 1200.0);
    baseDemands.push_back({"Mumbai", 3800.0});

    // 3: Delhi
    servers.emplace_back(3, "Delhi", "North India", 5500.0, 32.0, 9.00, 3400.0, 1100.0);
    baseDemands.push_back({"Delhi", 3400.0});

    // 4: Bangalore
    servers.emplace_back(4, "Bangalore", "South Tech Hub", 5000.0, 24.0, 7.50, 3100.0, 1000.0);
    baseDemands.push_back({"Bangalore", 3100.0});

    for (const auto& s : servers) {
        prevStatuses.push_back(s.getStatus());
    }

    currentScenario = ScenarioType::NORMAL;
    tickCount = 0;

    logEvent("SYSTEM", "C++ Physics Engine Initialized 4 Edge PoPs: Pune, Mumbai, Delhi, Bangalore", "INFO");
}

double TrafficSimulator::getDiurnalMultiplier(int tick) const {
    if (!diurnalEnabled) return 1.0;
    // Diurnal sine wave cycle over 24 ticks (trough at t=0, peak at t=12)
    double angle = (2.0 * M_PI * (tick % diurnalPeriodTicks)) / static_cast<double>(diurnalPeriodTicks) - (M_PI / 2.0);
    return 1.0 + (diurnalAmplitude * std::sin(angle));
}

void TrafficSimulator::logEvent(const std::string& source, const std::string& message, const std::string& level) {
    SimulatorEvent ev;
    ev.tick = tickCount;
    ev.timestamp = getCurrentTimeString();
    ev.source = source;
    ev.message = message;
    ev.level = level;

    eventsLog.push_back(ev);
    if (eventsLog.size() > 100) {
        eventsLog.erase(eventsLog.begin());
    }
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
            logEvent("SCENARIO", "Scenario reset to NORMAL traffic conditions", "INFO");
            break;
        }

        case ScenarioType::TRAFFIC_SPIKE: {
            getServer("Pune")->setTraffic(4900.0);
            getServer("Mumbai")->setTraffic(4200.0);
            getServer("Delhi")->setTraffic(3800.0);
            getServer("Bangalore")->setTraffic(3500.0);
            logEvent("SCENARIO", "Traffic Spike triggered across fleet", "WARNING");
            break;
        }

        case ScenarioType::PUNE_CONGESTION: {
            getServer("Pune")->setTraffic(4900.0);
            getServer("Pune")->updateMetrics(false);
            logEvent("Pune", "Severe Congestion (>98% load) triggered on Pune", "WARNING");
            break;
        }

        case ScenarioType::MUMBAI_FAILURE: {
            getServer("Mumbai")->fail();
            logEvent("Mumbai", "SERVER OUTAGE: Mumbai edge node crashed (DOWN)", "CRITICAL");
            break;
        }

        case ScenarioType::MULTIPLE_CONGESTION: {
            getServer("Pune")->setTraffic(4900.0);
            getServer("Delhi")->setTraffic(5400.0);
            getServer("Pune")->updateMetrics(false);
            getServer("Delhi")->updateMetrics(false);
            logEvent("SCENARIO", "Multi-point Congestion triggered on Pune and Delhi", "WARNING");
            break;
        }

        case ScenarioType::OVERLOAD: {
            for (auto& s : servers) {
                if (s.isFailed()) s.recover();
                s.setTraffic(s.getMaximumCapacity() * 0.95);
            }
            logEvent("SCENARIO", "Fleet Overload (95% saturation) triggered", "CRITICAL");
            break;
        }

        case ScenarioType::RECOVERY: {
            for (auto& s : servers) {
                if (s.isFailed()) s.recover();
            }
            getServer("Pune")->setTraffic(3200.0);
            getServer("Mumbai")->setTraffic(3800.0);
            getServer("Delhi")->setTraffic(3400.0);
            getServer("Bangalore")->setTraffic(3100.0);
            logEvent("SCENARIO", "Fleet fully recovered to baseline healthy state", "SUCCESS");
            break;
        }
    }
}

void TrafficSimulator::tick(bool applyStochasticDrift) {
    tickCount++;

    double diurnalMult = getDiurnalMultiplier(tickCount);

    if (applyStochasticDrift) {
        static std::mt19937 gen(1337 + tickCount);
        // Poisson approximation via Gaussian noise (mean 0, stddev 0.02 = 2% variance)
        std::normal_distribution<> noiseDist(0.0, 0.02);
        std::uniform_real_distribution<> spikeCheck(0.0, 1.0);

        // Check for spontaneous random spike (5% chance if enabled)
        int spikeServerIdx = -1;
        if (randomSpikesEnabled && spikeCheck(gen) < 0.05) {
            std::uniform_int_distribution<> srvDist(0, (int)servers.size() - 1);
            spikeServerIdx = srvDist(gen);
        }

        for (size_t i = 0; i < servers.size(); ++i) {
            auto& s = servers[i];
            if (s.isFailed()) continue;

            double baseDem = 3200.0;
            for (const auto& bd : baseDemands) {
                if (bd.first == s.getName()) {
                    baseDem = bd.second;
                    break;
                }
            }

            double noise = noiseDist(gen);
            double newTraffic = baseDem * diurnalMult * (1.0 + noise);

            if (static_cast<int>(i) == spikeServerIdx) {
                newTraffic = std::min(s.getMaximumCapacity() * 0.98, newTraffic + 1500.0);
                logEvent(s.getName(), "Spontaneous Poisson surge detected (+1500 req/s)", "WARNING");
            }

            s.setTraffic(std::max(100.0, newTraffic));
        }
    } else {
        for (auto& s : servers) {
            s.updateMetrics(false);
        }
    }

    // Detect state transitions and log events
    for (size_t i = 0; i < servers.size(); ++i) {
        ServerStatus curr = servers[i].getStatus();
        if (i < prevStatuses.size()) {
            ServerStatus prev = prevStatuses[i];
            if (curr != prev) {
                std::string lvl = (curr == ServerStatus::CONGESTED) ? "WARNING" :
                                  ((curr == ServerStatus::FAILED) ? "CRITICAL" : "SUCCESS");
                logEvent(servers[i].getName(),
                         "Status transition: " + statusToString(prev) + " -> " + statusToString(curr) +
                         " (Traffic: " + std::to_string((int)servers[i].getCurrentRequests()) + " req/s)",
                         lvl);
                prevStatuses[i] = curr;
            }
        }
    }
}

void TrafficSimulator::setServerTraffic(const std::string& name, double requests) {
    CDNServer* s = getServer(name);
    if (s) {
        s->setTraffic(requests);
        logEvent(name, "Traffic set to " + std::to_string((int)requests) + " req/s", "INFO");
    }
}

void TrafficSimulator::triggerSpike(const std::string& name, double targetTraffic) {
    CDNServer* s = getServer(name);
    if (s) {
        s->setTraffic(targetTraffic);
        logEvent(name, "Traffic spike injected: targeting " + std::to_string((int)targetTraffic) + " req/s", "WARNING");
    }
}

void TrafficSimulator::failServer(const std::string& name) {
    CDNServer* s = getServer(name);
    if (s) {
        s->fail();
        logEvent(name, "SERVER OUTAGE: " + name + " marked FAILED", "CRITICAL");
    }
}

void TrafficSimulator::recoverServer(const std::string& name, double initialTraffic) {
    CDNServer* s = getServer(name);
    if (s) {
        s->recover(initialTraffic);
        logEvent(name, "Server recovered: " + name + " back online (HEALTHY)", "SUCCESS");
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

double TrafficSimulator::getAverageCpu() const {
    double sum = 0.0;
    int count = 0;
    for (const auto& s : servers) {
        if (!s.isFailed()) {
            sum += s.getCpuUtilization();
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

std::string TrafficSimulator::getFleetHealth() const {
    int downCount = 0;
    int congestedCount = 0;
    for (const auto& s : servers) {
        if (s.isFailed()) downCount++;
        else if (s.isCongested()) congestedCount++;
    }

    if (downCount > 0) {
        return (downCount < static_cast<int>(servers.size())) ? "DEGRADED" : "CRITICAL";
    }
    if (congestedCount > 0) {
        return "CONGESTED";
    }
    return "OPTIMAL";
}

std::string TrafficSimulator::getTelemetryJson() const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);

    int downCount = 0;
    int congestedCount = 0;
    for (const auto& s : servers) {
        if (s.isFailed()) downCount++;
        else if (s.isCongested()) congestedCount++;
    }

    ss << "{\n"
       << "    \"tick\": " << tickCount << ",\n"
       << "    \"timestamp\": \"" << getCurrentTimeString() << "\",\n"
       << "    \"fleet_health\": \"" << getFleetHealth() << "\",\n"
       << "    \"total_capacity\": " << getTotalCapacity() << ",\n"
       << "    \"total_traffic\": " << getTotalTraffic() << ",\n"
       << "    \"total_bandwidth\": " << getTotalBandwidth() << ",\n"
       << "    \"avg_latency\": " << getAverageLatency() << ",\n"
       << "    \"avg_cpu\": " << getAverageCpu() << ",\n"
       << "    \"active_count\": " << getActiveServerCount() << ",\n"
       << "    \"down_count\": " << downCount << ",\n"
       << "    \"congested_count\": " << congestedCount << ",\n"
       << "    \"redis_connected\": false,\n"
       << "    \"diurnal_multiplier\": " << std::setprecision(3) << getDiurnalMultiplier(tickCount) << ",\n"
       << "    \"servers\": [\n";

    for (size_t i = 0; i < servers.size(); ++i) {
        const auto& s = servers[i];
        ss << "      {\n"
           << "        \"id\": " << s.getId() << ",\n"
           << "        \"name\": \"" << s.getName() << "\",\n"
           << "        \"location\": \"" << s.getLocation() << "\",\n"
           << "        \"capacity\": " << s.getMaximumCapacity() << ",\n"
           << "        \"current_requests\": " << s.getCurrentRequests() << ",\n"
           << "        \"assigned_traffic\": " << s.getCurrentRequests() << ",\n"
           << "        \"routed_traffic\": " << s.getCurrentRequests() << ",\n"
           << "        \"available_capacity\": " << s.getAvailableCapacity() << ",\n"
           << "        \"base_latency\": " << s.getBaseLatency() << ",\n"
           << "        \"latency\": " << s.getCurrentLatency() << ",\n"
           << "        \"bandwidth_cost\": " << std::setprecision(2) << s.getBandwidthCost() << ",\n"
           << "        \"cpu_utilization\": " << std::setprecision(1) << s.getCpuUtilization() << ",\n"
           << "        \"bandwidth_usage\": " << s.getBandwidthUsage() << ",\n"
           << "        \"bandwidth_capacity\": " << s.getBandwidthCapacity() << ",\n"
           << "        \"status\": \"" << (s.isFailed() ? "DOWN" : (s.isCongested() ? "CONGESTED" : "HEALTHY")) << "\",\n"
           << "        \"is_failed\": " << (s.isFailed() ? "true" : "false") << ",\n"
           << "        \"is_congested\": " << (s.isCongested() ? "true" : "false") << "\n"
           << "      }" << (i + 1 < servers.size() ? "," : "") << "\n";
    }
    ss << "    ],\n";

    // Recent events log
    ss << "    \"recent_events\": [\n";
    size_t startIdx = (eventsLog.size() > 8) ? (eventsLog.size() - 8) : 0;
    for (size_t i = startIdx; i < eventsLog.size(); ++i) {
        const auto& ev = eventsLog[i];
        ss << "      {\n"
           << "        \"tick\": " << ev.tick << ",\n"
           << "        \"timestamp\": \"" << ev.timestamp << "\",\n"
           << "        \"source\": \"" << ev.source << "\",\n"
           << "        \"message\": \"" << ev.message << "\",\n"
           << "        \"level\": \"" << ev.level << "\"\n"
           << "      }" << (i + 1 < eventsLog.size() ? "," : "") << "\n";
    }
    ss << "    ]\n";
    ss << "  }";

    return ss.str();
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
