#include "Router.h"
#include <cmath>
#include <sstream>

std::string RoutingDecision::toJson() const {
    std::stringstream ss;
    ss << "{\n"
       << "  \"schema_version\": " << schemaVersion << ",\n"
       << "  \"cycle_id\": " << cycleId << ",\n"
       << "  \"timestamp\": \"" << timestamp << "\",\n"
       << "  \"requested_traffic\": " << requestedTraffic << ",\n"
       << "  \"routed_traffic\": " << routedTraffic << ",\n"
       << "  \"unrouted_traffic\": " << unroutedTraffic << ",\n"
       << "  \"total_cost_rs\": " << std::fixed << std::setprecision(2) << totalCostRs << ",\n"
       << "  \"routing_status\": \"" << routingStatus << "\",\n"
       << "  \"change_detected\": " << (changeDetected ? "true" : "false") << ",\n"
       << "  \"trigger_reason\": \"" << triggerReason << "\",\n";

    // Previous Allocation
    ss << "  \"previous_allocation\": {";
    bool first = true;
    for (const auto& kv : previousAllocation) {
        if (!first) ss << ", ";
        ss << "\"" << kv.first << "\": " << kv.second;
        first = false;
    }
    ss << "},\n";

    // New Allocation
    ss << "  \"new_allocation\": {";
    first = true;
    for (const auto& kv : newAllocation) {
        if (!first) ss << ", ";
        ss << "\"" << kv.first << "\": " << kv.second;
        first = false;
    }
    ss << "},\n";

    // Allocation Diff
    ss << "  \"allocation_diff\": {";
    first = true;
    for (const auto& kv : allocationDiff) {
        if (!first) ss << ", ";
        ss << "\"" << kv.first << "\": " << kv.second;
        first = false;
    }
    ss << "},\n";

    // Server Details Array
    ss << "  \"server_details\": [\n";
    for (size_t i = 0; i < serverDetails.size(); i++) {
        const auto& s = serverDetails[i];
        ss << "    {\n"
           << "      \"server\": \"" << s.serverName << "\",\n"
           << "      \"assigned_traffic\": " << s.assignedTraffic << ",\n"
           << "      \"capacity\": " << s.capacity << ",\n"
           << "      \"utilization_pct\": " << std::fixed << std::setprecision(1) << s.utilizationPercent << ",\n"
           << "      \"latency_ms\": " << s.latencyMs << ",\n"
           << "      \"unit_cost_rs\": " << std::fixed << std::setprecision(2) << s.unitCostRs << ",\n"
           << "      \"subtotal_cost_rs\": " << std::fixed << std::setprecision(2) << s.subtotalCostRs << ",\n"
           << "      \"status\": \"" << statusToString(s.status) << "\"\n"
           << "    }" << (i + 1 < serverDetails.size() ? "," : "") << "\n";
    }
    ss << "  ],\n"
       << "  \"execution_time_ms\": " << std::fixed << std::setprecision(3) << executionTimeMs << ",\n"
       << "  \"engine_solve_time_us\": " << std::fixed << std::setprecision(1) << (engineSolveTimeUs > 0.0 ? engineSolveTimeUs : executionTimeMs * 1000.0);

    // Phase 5 Requirement: trace array in the JSON
    if (!traceSteps.empty()) {
        ss << ",\n  \"trace\": [\n";
        for (size_t i = 0; i < traceSteps.size(); ++i) {
            const auto& step = traceSteps[i];
            ss << "    {\n";
            ss << "      \"iteration\": " << step.iteration << ",\n";
            ss << "      \"dist\": [";
            for (size_t d = 0; d < step.dist.size(); ++d) {
                if (d > 0) ss << ", ";
                if (step.dist[d] >= 1000000000000000LL - 1000000) ss << "-1";
                else ss << step.dist[d];
            }
            ss << "],\n";
            ss << "      \"potentials_before\": [";
            for (size_t p = 0; p < step.potentialsBefore.size(); ++p) {
                if (p > 0) ss << ", ";
                ss << step.potentialsBefore[p];
            }
            ss << "],\n";
            ss << "      \"potentials_after\": [";
            for (size_t p = 0; p < step.potentialsAfter.size(); ++p) {
                if (p > 0) ss << ", ";
                ss << step.potentialsAfter[p];
            }
            ss << "],\n";
            ss << "      \"chosen_path_nodes\": [";
            for (size_t n = 0; n < step.pathNodes.size(); ++n) {
                if (n > 0) ss << ", ";
                ss << step.pathNodes[n];
            }
            ss << "],\n";
            ss << "      \"chosen_path_names\": [";
            for (size_t n = 0; n < step.pathNames.size(); ++n) {
                if (n > 0) ss << ", ";
                ss << "\"" << step.pathNames[n] << "\"";
            }
            ss << "],\n";
            ss << "      \"reduced_path_cost\": " << step.reducedPathCost << ",\n";
            ss << "      \"real_path_cost\": " << step.realPathCost << ",\n";
            ss << "      \"bottleneck_delta\": " << step.bottleneckDelta << ",\n";
            ss << "      \"flow_pushed\": " << step.flowPushed << ",\n";
            ss << "      \"cumulative_flow\": " << step.cumulativeFlow << ",\n";
            ss << "      \"cumulative_cost\": " << step.cumulativeCost << ",\n";
            ss << "      \"residual_graph\": [\n";
            for (size_t r = 0; r < step.residualGraph.size(); ++r) {
                const auto& e = step.residualGraph[r];
                ss << "        {\"from\": " << e.from
                   << ", \"to\": " << e.to
                   << ", \"from_name\": \"" << e.fromName << "\""
                   << ", \"to_name\": \"" << e.toName << "\""
                   << ", \"capacity\": " << e.capacity
                   << ", \"flow\": " << e.flow
                   << ", \"residual_capacity\": " << e.residualCapacity
                   << ", \"cost\": " << e.cost
                   << "}" << (r + 1 < step.residualGraph.size() ? "," : "") << "\n";
            }
            ss << "      ]\n";
            ss << "    }" << (i + 1 < traceSteps.size() ? "," : "") << "\n";
        }
        ss << "  ]\n";
    } else {
        ss << "\n";
    }
    ss << "}";
    return ss.str();
}

Router::Router(double latWeight, double bwWeight, int scaleFactor)
    : cycleCounter(0),
      lastRequestedTraffic(0),
      lastGraph(6, latWeight, bwWeight, scaleFactor),
      lastAugmentations(0),
      latencyWeight(latWeight),
      bandwidthWeight(bwWeight),
      costScaleFactor(scaleFactor)
{}

bool Router::hasSignificantChange(
    const std::vector<CDNServer>& currentServers,
    int requestedTraffic,
    std::string& outReason
) {
    if (cachedServers.empty()) {
        outReason = "Initial router cold start";
        return true;
    }

    if (std::abs(requestedTraffic - lastRequestedTraffic) > 20) {
        outReason = "Traffic demand changed: " + std::to_string(lastRequestedTraffic) + " -> " + std::to_string(requestedTraffic) + " req/s";
        return true;
    }

    if (currentServers.size() != cachedServers.size()) {
        outReason = "Server fleet membership changed";
        return true;
    }

    for (size_t i = 0; i < currentServers.size(); i++) {
        const auto& curr = currentServers[i];
        const auto& prev = cachedServers[i];

        // 1. Health Status Changed
        if (curr.getStatus() != prev.getStatus()) {
            outReason = curr.getName() + " status changed: " + statusToString(prev.getStatus()) + " -> " + statusToString(curr.getStatus());
            return true;
        }

        // 2. Latency shifted by > 2.0 ms (affects routing cost)
        if (std::abs(curr.getCurrentLatency() - prev.getCurrentLatency()) > 2.0) {
            outReason = curr.getName() + " latency shifted: " + std::to_string((int)prev.getCurrentLatency()) + "ms -> " + std::to_string((int)curr.getCurrentLatency()) + "ms";
            return true;
        }

        // 3. Available capacity shifted significantly
        if (std::abs(curr.getAvailableCapacity() - prev.getAvailableCapacity()) > 100.0) {
            outReason = curr.getName() + " available capacity changed: " + std::to_string((int)prev.getAvailableCapacity()) + " -> " + std::to_string((int)curr.getAvailableCapacity()) + " req/s";
            return true;
        }
    }

    outReason = "No significant condition changes detected (cached decision reused)";
    return false;
}

RoutingDecision Router::route(
    const std::vector<CDNServer>& currentServers,
    int requestedTraffic,
    const std::string& timestamp
) {
    cycleCounter++;
    std::string reason;
    bool changeDetected = hasSignificantChange(currentServers, requestedTraffic, reason);

    // If condition is steady state, return cached decision instantly
    if (!changeDetected && !cachedServers.empty()) {
        RoutingDecision cached = lastDecision;
        cached.cycleId = cycleCounter;
        cached.changeDetected = false;
        cached.triggerReason = reason;
        if (!timestamp.empty()) cached.timestamp = timestamp;
        return cached;
    }

    // Build flow graph and solve MCMF
    Graph graph(6, latencyWeight, bandwidthWeight, costScaleFactor);
    graph.buildFromServers(currentServers);

    MinCostMaxFlow mcmf(6);
    auto mcmfResult = mcmf.solve(graph, Graph::SOURCE_NODE, Graph::SINK_NODE, requestedTraffic);
    lastGraph = graph;
    lastAugmentations = mcmf.getAugmentationCount();

    int totalFlow = mcmfResult.first;
    long long scaledTotalCost = mcmfResult.second;
    double unscaledTotalCost = static_cast<double>(scaledTotalCost) / costScaleFactor;

    auto newAlloc = mcmf.extractServerAllocations(graph);

    // Build RoutingDecision
    RoutingDecision decision;
    decision.cycleId = cycleCounter;
    decision.timestamp = timestamp.empty() ? "2026-10-01T22:45:00Z" : timestamp;
    decision.requestedTraffic = requestedTraffic;
    decision.routedTraffic = totalFlow;
    decision.unroutedTraffic = std::max(0, requestedTraffic - totalFlow);
    decision.totalCostRs = unscaledTotalCost;
    decision.changeDetected = true;
    decision.triggerReason = reason;
    decision.executionTimeMs = mcmf.getExecutionTimeMs();
    decision.engineSolveTimeUs = mcmf.getExecutionTimeMs() * 1000.0;
    decision.previousAllocation = lastDecision.newAllocation;
    decision.newAllocation = newAlloc;

    // Assess routing status badge
    bool anyFailed = false;
    bool anyCongested = false;
    for (const auto& s : currentServers) {
        if (s.isFailed()) anyFailed = true;
        if (s.isCongested()) anyCongested = true;
    }

    if (totalFlow < requestedTraffic) {
        decision.routingStatus = "INSUFFICIENT_CAPACITY";
    } else if (anyFailed) {
        decision.routingStatus = "FAILOVER";
    } else if (anyCongested) {
        decision.routingStatus = "CONGESTED_REROUTE";
    } else {
        decision.routingStatus = "OPTIMAL";
    }

    // Fill Server Details and Diffs
    for (const auto& s : currentServers) {
        int assigned = newAlloc.count(s.getName()) ? newAlloc[s.getName()] : 0;
        int prev = decision.previousAllocation.count(s.getName()) ? decision.previousAllocation[s.getName()] : 0;
        decision.allocationDiff[s.getName()] = assigned - prev;

        ServerAllocationRecord rec;
        rec.serverName = s.getName();
        rec.assignedTraffic = assigned;
        rec.capacity = s.isFailed() ? 0 : static_cast<int>(s.getMaximumCapacity());
        rec.utilizationPercent = (rec.capacity > 0) ? (static_cast<double>(assigned) / rec.capacity * 100.0) : 0.0;
        rec.latencyMs = s.getCurrentLatency();
        rec.status = s.getStatus();

        int unitScaledCost = graph.calculateScaledCost(s.getCurrentLatency(), s.getBandwidthCost(), s.isFailed());
        rec.unitCostRs = static_cast<double>(unitScaledCost) / costScaleFactor;
        rec.subtotalCostRs = assigned * rec.unitCostRs;

        decision.serverDetails.push_back(rec);
    }

    // Update state cache
    cachedServers = currentServers;
    lastRequestedTraffic = requestedTraffic;
    lastDecision = decision;

    return decision;
}

void Router::printDecisionTable(const RoutingDecision& d) const {
    std::cout << "\n========================================================================================\n";
    std::cout << " [CYCLE #" << d.cycleId << "] ADAPTIVE ROUTING DECISION: " << d.routingStatus 
              << " (" << (d.changeDetected ? "RECOMPUTED via MCMF" : "CACHED / SKIPPED") << ")\n";
    std::cout << "========================================================================================\n";
    std::cout << " Trigger Reason:          " << d.triggerReason << "\n";
    std::cout << " Requested Traffic:       " << d.requestedTraffic << " req/s\n";
    std::cout << " Routed Traffic:          " << d.routedTraffic << " req/s";
    if (d.unroutedTraffic > 0) {
        std::cout << " (DROPPED/UNROUTED: " << d.unroutedTraffic << " req/s - INSUFFICIENT CAPACITY)";
    }
    std::cout << "\n Total Routing Cost:      Rs. " << std::fixed << std::setprecision(2) << d.totalCostRs << "\n";
    std::cout << " MCMF Execution Time:     " << std::fixed << std::setprecision(3) << d.executionTimeMs << " ms\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(12) << "Server"
              << std::setw(12) << "Status"
              << std::setw(10) << "Latency"
              << std::setw(14) << "Prev Alloc"
              << std::setw(14) << "New Alloc"
              << std::setw(14) << "Traffic Diff"
              << std::setw(12) << "Unit Cost"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    for (const auto& s : d.serverDetails) {
        int prev = d.previousAllocation.count(s.serverName) ? d.previousAllocation.at(s.serverName) : 0;
        int diff = d.allocationDiff.count(s.serverName) ? d.allocationDiff.at(s.serverName) : 0;
        std::string diffStr = (diff >= 0 ? "+" : "") + std::to_string(diff) + " req/s";
        std::string latStr = (s.status == ServerStatus::FAILED) ? "INF" : (std::to_string((int)s.latencyMs) + " ms");

        std::cout << std::left
                  << std::setw(12) << s.serverName
                  << std::setw(12) << statusToString(s.status)
                  << std::setw(10) << latStr
                  << std::setw(14) << (std::to_string(prev) + " req/s")
                  << std::setw(14) << (std::to_string(s.assignedTraffic) + " req/s")
                  << std::setw(14) << diffStr
                  << std::setw(12) << std::fixed << std::setprecision(1) << s.unitCostRs
                  << "\n";
    }
    std::cout << "========================================================================================\n";
}
