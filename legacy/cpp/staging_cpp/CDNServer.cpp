#include "CDNServer.h"
#include <sstream>
#include <random>

std::string statusToString(ServerStatus status) {
    switch (status) {
        case ServerStatus::HEALTHY:   return "HEALTHY";
        case ServerStatus::CONGESTED: return "CONGESTED";
        case ServerStatus::FAILED:    return "FAILED";
        default:                      return "UNKNOWN";
    }
}

CDNServer::CDNServer(
    int serverId,
    const std::string& serverName,
    const std::string& serverLocation,
    double maxCap,
    double baseLat,
    double bwCost,
    double initialRequests,
    double bwCap,
    double mbitsPerReq
) : id(serverId),
    name(serverName),
    location(serverLocation),
    maximumCapacity(maxCap),
    currentRequests(initialRequests),
    baseLatency(baseLat),
    currentLatency(baseLat),
    cpuUtilization(0.0),
    bandwidthUsage(0.0),
    bandwidthCost(bwCost),
    status(ServerStatus::HEALTHY),
    mbitsPerRequest(mbitsPerReq)
{
    // Default bandwidth capacity proportional to max request throughput
    if (bwCap > 0.0) {
        bandwidthCapacity = bwCap;
    } else {
        bandwidthCapacity = maxCap * 0.20; // e.g. 5000 req/s * 0.2 = 1000 Mbps
    }

    updateMetrics(false);
}

double CDNServer::getAvailableCapacity() const {
    if (status == ServerStatus::FAILED) {
        return 0.0;
    }
    return std::max(0.0, maximumCapacity - currentRequests);
}

void CDNServer::setTraffic(double requests) {
    if (status == ServerStatus::FAILED) {
        currentRequests = 0.0;
        return;
    }
    currentRequests = std::max(0.0, requests);
    updateMetrics(false);
}

void CDNServer::fail() {
    status = ServerStatus::FAILED;
    currentRequests = 0.0;
    currentLatency = 9999.0; // Infinite/unreachable penalty
    cpuUtilization = 0.0;
    bandwidthUsage = 0.0;
}

void CDNServer::recover(double initialTraffic) {
    status = ServerStatus::HEALTHY;
    currentLatency = baseLatency;
    if (initialTraffic >= 0.0) {
        currentRequests = initialTraffic;
    } else {
        currentRequests = maximumCapacity * 0.60;
    }
    updateMetrics(false);
}

void CDNServer::updateMetrics(bool addJitter) {
    if (status == ServerStatus::FAILED) {
        currentRequests = 0.0;
        currentLatency = 9999.0;
        cpuUtilization = 0.0;
        bandwidthUsage = 0.0;
        return;
    }

    // Load ratio rho = currentRequests / maximumCapacity
    double rho = currentRequests / std::max(1.0, maximumCapacity);

    // 1. Egress Bandwidth Calculation (proportional to processed requests)
    bandwidthUsage = currentRequests * mbitsPerRequest;

    // 2. CPU Utilization Calculation (linear up to 70%, steeper curve near saturation)
    double cpuVal = 0.0;
    if (rho <= 0.70) {
        cpuVal = rho * 100.0;
    } else if (rho <= 0.90) {
        cpuVal = 70.0 + ((rho - 0.70) / 0.20) * 18.0; // 70% -> 88%
    } else if (rho <= 1.00) {
        cpuVal = 88.0 + ((rho - 0.90) / 0.10) * 10.0; // 88% -> 98%
    } else {
        cpuVal = std::min(100.0, 98.0 + (rho - 1.0) * 10.0);
    }
    cpuUtilization = std::max(0.0, std::min(100.0, cpuVal));

    // 3. Dynamic Queuing Latency (calibrated M/M/1 approximation from Phase 1)
    // rho <= 0.64: baseline latency
    // rho ~ 0.82: 1.6x baseLatency (~40ms for 25ms base)
    // rho ~ 0.93: 3.2x baseLatency (~80ms for 25ms base)
    // rho >= 0.98: 4.8x baseLatency (~120ms+ for 25ms base)
    double calculatedLatency = baseLatency;
    if (rho <= 0.64) {
        calculatedLatency = baseLatency;
    } else if (rho <= 0.82) {
        double prog = (rho - 0.64) / (0.82 - 0.64);
        calculatedLatency = baseLatency + prog * (baseLatency * 0.60);
    } else if (rho <= 0.93) {
        double prog = (rho - 0.82) / (0.93 - 0.82);
        calculatedLatency = (baseLatency * 1.60) + prog * (baseLatency * 1.60);
    } else if (rho <= 0.98) {
        double prog = (rho - 0.93) / (0.98 - 0.93);
        calculatedLatency = (baseLatency * 3.20) + prog * (baseLatency * 1.60);
    } else {
        double excess = rho - 0.98;
        calculatedLatency = (baseLatency * 4.80) + excess * (baseLatency * 20.0);
    }

    if (addJitter) {
        static std::mt19937 gen(1337);
        std::uniform_real_distribution<> dis(-0.5, 0.5);
        calculatedLatency += dis(gen);
    }

    currentLatency = std::max(baseLatency, calculatedLatency);

    // 4. Status Transition
    if (cpuUtilization >= 85.0 || rho >= 0.85 || currentLatency >= (baseLatency * 2.5)) {
        status = ServerStatus::CONGESTED;
    } else {
        status = ServerStatus::HEALTHY;
    }
}

void CDNServer::printDetails() const {
    std::cout << "[" << name << " (" << location << ")] Status: " << statusToString(status)
              << " | Traffic: " << (int)currentRequests << "/" << (int)maximumCapacity << " req/s"
              << " | Avail: " << (int)getAvailableCapacity() << " req/s"
              << " | Latency: " << std::fixed << std::setprecision(1) << currentLatency << " ms"
              << " | CPU: " << std::fixed << std::setprecision(1) << cpuUtilization << "%"
              << " | BW: " << std::fixed << std::setprecision(1) << bandwidthUsage << " Mbps"
              << " | Cost: Rs." << std::fixed << std::setprecision(2) << bandwidthCost << "/GB\n";
}

std::string CDNServer::toJson() const {
    std::stringstream ss;
    ss << "{\n"
       << "  \"id\": " << id << ",\n"
       << "  \"name\": \"" << name << "\",\n"
       << "  \"location\": \"" << location << "\",\n"
       << "  \"maximumCapacity\": " << maximumCapacity << ",\n"
       << "  \"currentRequests\": " << currentRequests << ",\n"
       << "  \"availableCapacity\": " << getAvailableCapacity() << ",\n"
       << "  \"baseLatency\": " << baseLatency << ",\n"
       << "  \"currentLatency\": " << currentLatency << ",\n"
       << "  \"cpuUtilization\": " << cpuUtilization << ",\n"
       << "  \"bandwidthUsage\": " << bandwidthUsage << ",\n"
       << "  \"bandwidthCapacity\": " << bandwidthCapacity << ",\n"
       << "  \"bandwidthCost\": " << bandwidthCost << ",\n"
       << "  \"status\": \"" << statusToString(status) << "\"\n"
       << "}";
    return ss.str();
}
