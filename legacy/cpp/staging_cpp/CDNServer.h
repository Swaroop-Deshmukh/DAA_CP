#ifndef CDN_SERVER_H
#define CDN_SERVER_H

#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>

/**
 * Server operational health status.
 */
enum class ServerStatus {
    HEALTHY,
    CONGESTED,
    FAILED
};

/**
 * Converts ServerStatus to human-readable string.
 */
std::string statusToString(ServerStatus status);

/**
 * Represents an individual CDN Edge Server (PoP).
 * Models realistic queuing dynamics, compute saturation, and failure mechanics.
 */
class CDNServer {
private:
    int id;
    std::string name;
    std::string location;
    double maximumCapacity;    // Max sustainable request throughput (req/s)
    double currentRequests;    // Current assigned incoming request rate (req/s)
    double baseLatency;        // Baseline geographic network latency (ms)
    double currentLatency;     // Dynamic response time under queuing load (ms)
    double cpuUtilization;     // Compute load (0.0 - 100.0%)
    double bandwidthUsage;     // Network egress throughput (Mbps)
    double bandwidthCapacity;  // Uplink network limit (Mbps)
    double bandwidthCost;      // Transit financial cost (₹/GB)
    ServerStatus status;       // HEALTHY, CONGESTED, or FAILED

    // Internal physics calibration factor (~0.1875 Mbps per req/s: 3200 req/s ≈ 600 Mbps)
    double mbitsPerRequest;

public:
    CDNServer(
        int serverId,
        const std::string& serverName,
        const std::string& serverLocation,
        double maxCap,
        double baseLat,
        double bwCost,
        double initialRequests = 0.0,
        double bwCap = -1.0,
        double mbitsPerReq = 0.1875
    );

    // Dynamic Physics & State Transitions
    void setTraffic(double requests);
    void updateMetrics(bool addJitter = false);
    void fail();
    void recover(double initialTraffic = -1.0);

    // Getters
    int getId() const { return id; }
    std::string getName() const { return name; }
    std::string getLocation() const { return location; }
    double getMaximumCapacity() const { return maximumCapacity; }
    double getCurrentRequests() const { return currentRequests; }
    double getAvailableCapacity() const;
    double getBaseLatency() const { return baseLatency; }
    double getCurrentLatency() const { return currentLatency; }
    double getCpuUtilization() const { return cpuUtilization; }
    double getBandwidthUsage() const { return bandwidthUsage; }
    double getBandwidthCapacity() const { return bandwidthCapacity; }
    double getBandwidthCost() const { return bandwidthCost; }
    ServerStatus getStatus() const { return status; }
    bool isFailed() const { return status == ServerStatus::FAILED; }
    bool isCongested() const { return status == ServerStatus::CONGESTED; }

    // Serialization & Display
    void printDetails() const;
    std::string toJson() const;
};

#endif // CDN_SERVER_H
