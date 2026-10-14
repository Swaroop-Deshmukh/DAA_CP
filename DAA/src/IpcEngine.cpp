#include "IpcEngine.h"
#include "Baselines.h"
#include "AlgorithmTrace.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstdio>
#include <algorithm>
#include <cmath>
#ifdef _WIN32
#include <windows.h>
#endif

std::string IpcEngine::extractJsonString(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";

    size_t start = json.find('"', pos);
    if (start == std::string::npos) return "";
    start++; // skip quote

    size_t end = json.find('"', start);
    if (end == std::string::npos) return "";

    return json.substr(start, end - start);
}

double IpcEngine::extractJsonNumber(const std::string& json, const std::string& key, double defaultVal) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return defaultVal;

    pos = json.find(':', pos);
    if (pos == std::string::npos) return defaultVal;

    pos++; // move past ':'
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;

    if (pos >= json.size()) return defaultVal;

    size_t end = pos;
    while (end < json.size() && (std::isdigit(json[end]) || json[end] == '.' || json[end] == '-' || json[end] == '+' || json[end] == 'e' || json[end] == 'E')) {
        end++;
    }

    if (end > pos) {
        try {
            return std::stod(json.substr(pos, end - pos));
        } catch (...) {
            return defaultVal;
        }
    }
    return defaultVal;
}

bool IpcEngine::extractJsonBool(const std::string& json, const std::string& key, bool defaultVal) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return defaultVal;

    pos = json.find(':', pos);
    if (pos == std::string::npos) return defaultVal;

    std::string sub = json.substr(pos);
    if (sub.find("true") != std::string::npos && sub.find("true") < 15) return true;
    if (sub.find("false") != std::string::npos && sub.find("false") < 15) return false;
    return defaultVal;
}

static double getHighResTimestampUs() {
#ifdef _WIN32
    static LARGE_INTEGER freq;
    static bool init = false;
    if (!init) {
        QueryPerformanceFrequency(&freq);
        init = true;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart * 1000000.0 / (double)freq.QuadPart;
#else
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::micro>(now.time_since_epoch()).count();
#endif
}

IpcEngine::IpcEngine()
    : activeStrategy("MCMF"),
      cycleCounter(0)
{
    double initialSolveUs = 0.0;
    lastDecision = computeCurrentRouting(initialSolveUs);
    std::string snap = buildSnapshotJson(lastDecision, initialSolveUs, initialSolveUs, "init");
    saveDecisionToStandardPaths(snap);
}

RoutingDecision IpcEngine::computeCurrentRouting(double& outSolveTimeUs) {
    cycleCounter++;
    int totalDemand = static_cast<int>(simulator.getTotalTraffic());

    double t0 = getHighResTimestampUs();
    RoutingDecision dec = router.route(simulator.getServers(), totalDemand);
    double t1 = getHighResTimestampUs();

    outSolveTimeUs = (t1 - t0);
    dec.engineSolveTimeUs = outSolveTimeUs;
    dec.executionTimeMs = outSolveTimeUs / 1000.0;
    dec.schemaVersion = 2;

    lastDecision = dec;
    return dec;
}

std::string IpcEngine::buildSnapshotJson(
    const RoutingDecision& decision,
    double solveTimeUs,
    double totalCmdTimeUs,
    const std::string& cmdName
) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);

    ss << "{\n"
       << "  \"schema_version\": 2,\n"
       << "  \"status\": \"ok\",\n"
       << "  \"cmd\": \"" << cmdName << "\",\n"
       << "  \"cycle_id\": " << decision.cycleId << ",\n"
       << "  \"active_strategy\": \"" << activeStrategy << "\",\n"
       << "  \"engine_solve_time_us\": " << solveTimeUs << ",\n"
       << "  \"execution_time_ms\": " << std::setprecision(3) << (solveTimeUs / 1000.0) << ",\n"
       << "  \"e2e_engine_latency_ms\": " << std::setprecision(3) << (totalCmdTimeUs / 1000.0) << ",\n"
       << "  \"telemetry\": " << simulator.getTelemetryJson() << ",\n"
       << "  \"routing\": " << decision.toJson() << "\n"
       << "}";

    return ss.str();
}

bool IpcEngine::saveAtomicJson(const std::string& targetPath, const std::string& content) {
    try {
        std::string tmpPath = targetPath + ".tmp";
        std::ofstream outFile(tmpPath, std::ios::trunc);
        if (!outFile.is_open()) return false;

        outFile << content;
        outFile.flush();
        outFile.close();

#ifdef _WIN32
        if (MoveFileExA(tmpPath.c_str(), targetPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            return true;
        }
#endif
        std::remove(targetPath.c_str());
        return (std::rename(tmpPath.c_str(), targetPath.c_str()) == 0);
    } catch (...) {
        return false;
    }
}

void IpcEngine::saveDecisionToStandardPaths(const std::string& jsonContent) {
    const std::vector<std::string> targetPaths = {
        "../data/routing_decision.json",
        "../../data/routing_decision.json",
        "data/routing_decision.json"
    };

    for (const auto& path : targetPaths) {
        if (saveAtomicJson(path, jsonContent)) {
            break;
        }
    }
}

std::string IpcEngine::handleCommand(const std::string& line) {
    double tStart = getHighResTimestampUs();
    std::string cmd = extractJsonString(line, "cmd");
    if (cmd.empty()) {
        cmd = extractJsonString(line, "action");
    }
    if (cmd.empty()) cmd = "tick";

    // 1. PING
    if (cmd == "ping") {
        return "{\"schema_version\": 2, \"status\": \"ok\", \"pong\": true}";
    }

    // 2. QUIT / EXIT
    if (cmd == "quit" || cmd == "exit") {
        return "{\"schema_version\": 2, \"status\": \"ok\", \"message\": \"Engine shutting down cleanly\"}";
    }

    // 3. RUN TRACE (Phase 5 integration)
    if (cmd == "run_trace" || cmd == "trace") {
        SimulationScenarioRunner runner;
        auto traceReport = runner.runTrace3x3Demo(false);
        std::string traceJson = traceReport.toJsonString();
        saveDecisionToStandardPaths(traceJson);

        double tEnd = getHighResTimestampUs();
        double cmdTimeUs = tEnd - tStart;

        std::stringstream ss;
        ss << "{\n"
           << "  \"schema_version\": 2,\n"
           << "  \"status\": \"ok\",\n"
           << "  \"cmd\": \"run_trace\",\n"
           << "  \"engine_solve_time_us\": " << cmdTimeUs << ",\n"
           << "  \"routing\": " << traceJson << "\n"
           << "}";
        return ss.str();
    }

    // 4. SCENARIO
    if (cmd == "run_scenario" || cmd == "scenario") {
        std::string scenStr = extractJsonString(line, "scenario");
        if (scenStr.empty()) scenStr = extractJsonString(line, "name");
        std::transform(scenStr.begin(), scenStr.end(), scenStr.begin(), ::tolower);

        if (scenStr == "normal" || scenStr == "reset") {
            simulator.applyScenario(ScenarioType::NORMAL);
        } else if (scenStr == "spike") {
            simulator.applyScenario(ScenarioType::TRAFFIC_SPIKE);
        } else if (scenStr == "pune" || scenStr == "pune_congestion") {
            simulator.applyScenario(ScenarioType::PUNE_CONGESTION);
        } else if (scenStr == "mumbai" || scenStr == "mumbai_fail" || scenStr == "mumbai_failure") {
            simulator.applyScenario(ScenarioType::MUMBAI_FAILURE);
        } else if (scenStr == "multi" || scenStr == "multiple_congestion") {
            simulator.applyScenario(ScenarioType::MULTIPLE_CONGESTION);
        } else if (scenStr == "overload" || scenStr == "fleet_overload") {
            simulator.applyScenario(ScenarioType::OVERLOAD);
        } else if (scenStr == "recovery" || scenStr == "fleet_recovery") {
            simulator.applyScenario(ScenarioType::RECOVERY);
        }

        double solveUs = 0.0;
        RoutingDecision dec = computeCurrentRouting(solveUs);
        double tEnd = getHighResTimestampUs();
        double cmdTimeUs = tEnd - tStart;

        std::string snap = buildSnapshotJson(dec, solveUs, cmdTimeUs, "run_scenario");
        saveDecisionToStandardPaths(snap);
        return snap;
    }

    // 5. SPIKE
    if (cmd == "spike") {
        std::string srv = extractJsonString(line, "server");
        double target = extractJsonNumber(line, "target_traffic", 4900.0);
        if (srv.empty()) srv = "Pune";

        simulator.triggerSpike(srv, target);

        double solveUs = 0.0;
        RoutingDecision dec = computeCurrentRouting(solveUs);
        double tEnd = getHighResTimestampUs();
        double cmdTimeUs = tEnd - tStart;

        std::string snap = buildSnapshotJson(dec, solveUs, cmdTimeUs, "spike");
        saveDecisionToStandardPaths(snap);
        return snap;
    }

    // 6. FAIL
    if (cmd == "fail") {
        std::string srv = extractJsonString(line, "server");
        if (srv.empty()) srv = "Mumbai";

        simulator.failServer(srv);

        double solveUs = 0.0;
        RoutingDecision dec = computeCurrentRouting(solveUs);
        double tEnd = getHighResTimestampUs();
        double cmdTimeUs = tEnd - tStart;

        std::string snap = buildSnapshotJson(dec, solveUs, cmdTimeUs, "fail");
        saveDecisionToStandardPaths(snap);
        return snap;
    }

    // 7. RECOVER
    if (cmd == "recover") {
        std::string srv = extractJsonString(line, "server");
        double initTraffic = extractJsonNumber(line, "initial_traffic", -1.0);
        if (srv.empty()) srv = "Mumbai";

        simulator.recoverServer(srv, initTraffic);

        double solveUs = 0.0;
        RoutingDecision dec = computeCurrentRouting(solveUs);
        double tEnd = getHighResTimestampUs();
        double cmdTimeUs = tEnd - tStart;

        std::string snap = buildSnapshotJson(dec, solveUs, cmdTimeUs, "recover");
        saveDecisionToStandardPaths(snap);
        return snap;
    }

    // 8. SET TRAFFIC
    if (cmd == "set_traffic") {
        std::string srv = extractJsonString(line, "server");
        double traffic = extractJsonNumber(line, "traffic", 3200.0);
        if (!srv.empty()) {
            simulator.setServerTraffic(srv, traffic);
        }

        double solveUs = 0.0;
        RoutingDecision dec = computeCurrentRouting(solveUs);
        double tEnd = getHighResTimestampUs();
        double cmdTimeUs = tEnd - tStart;

        std::string snap = buildSnapshotJson(dec, solveUs, cmdTimeUs, "set_traffic");
        saveDecisionToStandardPaths(snap);
        return snap;
    }

    // 9. SET STRATEGY
    if (cmd == "set_strategy" || cmd == "strategy") {
        std::string strat = extractJsonString(line, "strategy");
        if (!strat.empty()) activeStrategy = strat;

        double solveUs = 0.0;
        RoutingDecision dec = computeCurrentRouting(solveUs);
        double tEnd = getHighResTimestampUs();
        double cmdTimeUs = tEnd - tStart;

        std::string snap = buildSnapshotJson(dec, solveUs, cmdTimeUs, "set_strategy");
        saveDecisionToStandardPaths(snap);
        return snap;
    }

    // 10. SNAPSHOT
    if (cmd == "snapshot") {
        double tEnd = getHighResTimestampUs();
        double cmdTimeUs = tEnd - tStart;
        return buildSnapshotJson(lastDecision, lastDecision.engineSolveTimeUs, cmdTimeUs, "snapshot");
    }

    // 11. TICK (Default)
    bool drift = extractJsonBool(line, "drift", true);
    simulator.tick(drift);

    double solveUs = 0.0;
    RoutingDecision dec = computeCurrentRouting(solveUs);
    double tEnd = getHighResTimestampUs();
    double cmdTimeUs = tEnd - tStart;

    std::string snap = buildSnapshotJson(dec, solveUs, cmdTimeUs, "tick");
    saveDecisionToStandardPaths(snap);
    return snap;
}

static std::string toSingleLineJson(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    bool inQuotes = false;
    bool escape = false;
    for (char c : input) {
        if (c == '"' && !escape) {
            inQuotes = !inQuotes;
            out += c;
        } else if (c == '\\' && inQuotes) {
            escape = !escape;
            out += c;
        } else {
            escape = false;
            if (inQuotes) {
                out += c;
            } else if (c == '\n' || c == '\r' || c == '\t') {
                // skip newlines and tabs outside string literals
            } else if (c == ' ' && !out.empty() && out.back() == ' ') {
                // collapse consecutive spaces
            } else {
                out += c;
            }
        }
    }
    return out;
}

void IpcEngine::runInteractive() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // Initial greeting / ready confirmation (exactly 1 line)
    std::cout << "{\"schema_version\": 2, \"status\": \"ready\", \"engine\": \"DAA MCMF Engine\", \"mode\": \"ipc_daemon\"}\n";
    std::cout.flush();

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        std::string response = handleCommand(line);
        std::string singleLine = toSingleLineJson(response);
        std::cout << singleLine << "\n";
        std::cout.flush();

        std::string cmd = extractJsonString(line, "cmd");
        if (cmd == "quit" || cmd == "exit") {
            break;
        }
    }
}
