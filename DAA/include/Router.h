#ifndef ROUTER_H
#define ROUTER_H

#include "Graph.h"
#include "MinCostMaxFlow.h"
#include "CDNServer.h"
#include "AlgorithmTrace.h"
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <iomanip>

/**
 * Detailed per-server allocation in a routing decision.
 */
struct ServerAllocationRecord {
    std::string serverName;
    int assignedTraffic;
    int capacity;
    double utilizationPercent;
    double latencyMs;
    double unitCostRs;
    double subtotalCostRs;
    ServerStatus status;
};

/**
 * Canonical Routing Decision record for React Dashboards and API integrations.
 */
struct RoutingDecision {
    int schemaVersion = 2;
    int cycleId;
    std::string timestamp;
    int requestedTraffic;
    int routedTraffic;
    int unroutedTraffic;
    double totalCostRs;
    std::string routingStatus;      // "OPTIMAL", "CONGESTED_REROUTE", "FAILOVER", "INSUFFICIENT_CAPACITY"
    bool changeDetected;
    std::string triggerReason;
    std::map<std::string, int> previousAllocation;
    std::map<std::string, int> newAllocation;
    std::map<std::string, int> allocationDiff;
    std::vector<ServerAllocationRecord> serverDetails;
    double executionTimeMs;
    double engineSolveTimeUs;       // Engine solve time in microseconds (Phase 6)
    std::vector<SSPStepTrace> traceSteps; // Phase 5 Requirement: Algorithm trace array

    std::string toJson() const;
};

/**
 * Adaptive Router.
 * Closes the feedback loop between live server states and MCMF traffic allocation.
 */
class Router {
private:
    int cycleCounter;
    int lastRequestedTraffic;
    std::vector<CDNServer> cachedServers;
    RoutingDecision lastDecision;
    Graph lastGraph;
    int lastAugmentations;
    double latencyWeight;
    double bandwidthWeight;
    int costScaleFactor;

public:
    Router(double latWeight = 1.0, double bwWeight = 2.0, int scaleFactor = 100);

    bool hasSignificantChange(
        const std::vector<CDNServer>& currentServers,
        int requestedTraffic,
        std::string& outReason
    );

    RoutingDecision route(
        const std::vector<CDNServer>& currentServers,
        int requestedTraffic,
        const std::string& timestamp = ""
    );

    void printDecisionTable(const RoutingDecision& d) const;
    const RoutingDecision& getLastDecision() const { return lastDecision; }
    const Graph& getLastGraph() const { return lastGraph; }
    int getLastAugmentations() const { return lastAugmentations; }
};

#endif // ROUTER_H
