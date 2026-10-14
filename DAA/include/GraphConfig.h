#ifndef GRAPH_CONFIG_H
#define GRAPH_CONFIG_H

#include <string>
#include <vector>
#include <map>
#include <iostream>

/**
 * Utilization Band for modeling non-linear queuing cost
 * using convex piecewise-linear parallel edges.
 */
struct UtilizationBand {
    double lowerPct;        // e.g. 0.0 (0%)
    double upperPct;        // e.g. 0.50 (50%)
    double costMultiplier;  // e.g. 1.0x

    UtilizationBand(double low = 0.0, double high = 1.0, double mult = 1.0)
        : lowerPct(low), upperPct(high), costMultiplier(mult) {}
};

/**
 * Configuration for an individual CDN Edge Server (PoP).
 */
struct EdgeServerConfig {
    std::string name;
    int capacity;                       // Sustainable request capacity (req/s)
    double bandwidthCost;               // Transit cost (₹/GB)
    double baseLatency;                 // Baseline ping latency (ms)
    std::vector<UtilizationBand> bands; // Convex utilization tiers

    EdgeServerConfig()
        : name(""), capacity(0), bandwidthCost(0.0), baseLatency(0.0) {}

    EdgeServerConfig(const std::string& n, int cap, double bwCost, double baseLat)
        : name(n), capacity(cap), bandwidthCost(bwCost), baseLatency(baseLat) {}
};

/**
 * Configuration for a client request ingress region.
 */
struct RegionConfig {
    std::string name;
    int demand;             // Ingress traffic demand (req/s)
    double originLatency;   // Latency to origin data center (ms)
    double originCost;      // Financial penalty/cost if unserved by edge fleet (₹/req)

    RegionConfig()
        : name(""), demand(0), originLatency(120.0), originCost(250.0) {}

    RegionConfig(const std::string& n, int dem, double oLat = 120.0, double oCost = 250.0)
        : name(n), demand(dem), originLatency(oLat), originCost(oCost) {}
};

/**
 * Configuration for an edge link between a client Region and an Edge Server.
 */
struct LinkConfig {
    std::string fromRegion;
    std::string toServer;
    int capacity;           // Link bandwidth throughput limit (req/s)
    double latency;         // Geographic link network latency (ms)

    LinkConfig()
        : fromRegion(""), toServer(""), capacity(0), latency(0.0) {}

    LinkConfig(const std::string& r, const std::string& s, int cap, double lat)
        : fromRegion(r), toServer(s), capacity(cap), latency(lat) {}
};

/**
 * Complete Network Configuration for config-driven flow graph generation.
 */
struct NetworkConfig {
    std::string name;
    std::vector<RegionConfig> regions;
    std::vector<EdgeServerConfig> servers;
    std::vector<LinkConfig> links;

    // Optimization weights & constants
    double latencyWeight = 1.0;
    double bandwidthWeight = 2.0;
    int costScaleFactor = 100;
    double originPenaltyCost = 250.0;   // ₹ penalty for unserved edge traffic
    double baseTransitCost = 5.0;       // Base transit cost for server -> sink
    bool enableOriginFallback = true;   // Flag to toggle origin fallback link

    // Helpers & Presets
    int getTotalDemand() const;
    int getTotalServerCapacity() const;
    int getTotalNodeCount() const { return 1 + (int)regions.size() + (int)servers.size() + 2; }
    void printSummary() const;

    // Standard factory presets
    static NetworkConfig createDefault8RegionTopology();
    static NetworkConfig createCounterexampleTopology();
    static NetworkConfig createSmall3Region3ServerTopology();
    static NetworkConfig generateRandomNetwork(int numServers, unsigned int seed = 42);

    // Pure standard C++17 JSON serialization & loading
    static NetworkConfig loadFromJsonFile(const std::string& filepath);
    bool saveToJsonFile(const std::string& filepath) const;
    std::string toJsonString(int indent = 2) const;
};

#endif // GRAPH_CONFIG_H
