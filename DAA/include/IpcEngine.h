#ifndef IPC_ENGINE_H
#define IPC_ENGINE_H

#include "TrafficSimulator.h"
#include "Router.h"
#include "SimulationScenario.h"
#include <string>
#include <chrono>

/**
 * High-performance Long-Lived IPC Engine (Phase 6).
 *
 * Replaces per-event executable spawning with persistent newline-delimited
 * JSON IPC over standard input/output.
 *
 * Benefits:
 * - Sub-millisecond latency (<0.1ms vs 15-30ms process spawning on Windows)
 * - Single source of truth for CDN physics (Poisson, diurnal, queuing)
 * - Atomic file updates ("schema_version": 2)
 */
class IpcEngine {
private:
    TrafficSimulator simulator;
    Router router;
    std::string activeStrategy; // "MCMF", "ROUND_ROBIN", "NEAREST", "GREEDY"
    int cycleCounter;
    RoutingDecision lastDecision;

    // Helper JSON parsers
    static std::string extractJsonString(const std::string& json, const std::string& key);
    static double extractJsonNumber(const std::string& json, const std::string& key, double defaultVal = 0.0);
    static bool extractJsonBool(const std::string& json, const std::string& key, bool defaultVal = false);

public:
    IpcEngine();

    // Runs persistent interactive loop on std::cin / std::cout
    void runInteractive();

    // Process a single JSON command string and return JSON response
    std::string handleCommand(const std::string& line);

    // Compute routing under active strategy and record decision
    RoutingDecision computeCurrentRouting(double& outSolveTimeUs);

    // Build combined telemetry + routing JSON representation
    std::string buildSnapshotJson(
        const RoutingDecision& decision,
        double solveTimeUs,
        double totalCmdTimeUs,
        const std::string& cmdName = "tick"
    ) const;

    // Atomic file writer for schema_version 2 contract
    static bool saveAtomicJson(const std::string& targetPath, const std::string& content);
    static void saveDecisionToStandardPaths(const std::string& jsonContent);

    // Direct accessors
    TrafficSimulator& getSimulator() { return simulator; }
    Router& getRouter() { return router; }
};

#endif // IPC_ENGINE_H
