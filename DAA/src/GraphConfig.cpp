#include "GraphConfig.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <numeric>

int NetworkConfig::getTotalDemand() const {
    int sum = 0;
    for (const auto& r : regions) {
        sum += r.demand;
    }
    return sum;
}

int NetworkConfig::getTotalServerCapacity() const {
    int sum = 0;
    for (const auto& s : servers) {
        sum += s.capacity;
    }
    return sum;
}

void NetworkConfig::printSummary() const {
    std::cout << "\n========================================================================================\n";
    std::cout << "               NETWORK TOPOLOGY SPECIFICATION: " << name << "\n";
    std::cout << "========================================================================================\n";
    std::cout << " Regions:  " << regions.size() << " | Total Regional Demand: " << getTotalDemand() << " req/s\n";
    std::cout << " Edge PoPs: " << servers.size() << " | Total Fleet Capacity: " << getTotalServerCapacity() << " req/s\n";
    std::cout << " Links:    " << links.size() << " interconnected regional edge links\n";
    std::cout << " Weights:  alpha (Latency) = " << latencyWeight << ", beta (Bandwidth) = " << bandwidthWeight << "\n";
    std::cout << " Scaled Cost Factor: " << costScaleFactor << " | Origin Fallback Penalty: Rs. " << originPenaltyCost << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << " [REGIONAL INGRESS NODES]:\n";
    for (const auto& r : regions) {
        std::cout << "   - " << std::left << std::setw(14) << r.name
                  << " Demand: " << std::setw(7) << r.demand << " req/s | "
                  << "Origin Latency: " << r.originLatency << " ms\n";
    }
    std::cout << "\n [EDGE SERVER POPS & CONVEX BANDS]:\n";
    for (const auto& s : servers) {
        std::cout << "   - " << std::left << std::setw(14) << s.name
                  << " Capacity: " << std::setw(7) << s.capacity << " req/s | "
                  << "Bw Cost: Rs. " << std::setw(5) << s.bandwidthCost << "/GB | "
                  << "Bands: " << s.bands.size() << " tiers\n";
        for (size_t b = 0; b < s.bands.size(); ++b) {
            std::cout << "       Tier " << (b + 1) << ": ["
                      << static_cast<int>(s.bands[b].lowerPct * 100) << "% - "
                      << static_cast<int>(s.bands[b].upperPct * 100) << "%] -> "
                      << s.bands[b].costMultiplier << "x cost multiplier\n";
        }
    }
    std::cout << "========================================================================================\n";
}

// Standard 4 convex bands for realistic M/M/1 queuing modeling
static std::vector<UtilizationBand> createStandardConvexBands() {
    return {
        UtilizationBand(0.00, 0.50, 1.0), // 0-50% utilization: nominal base cost
        UtilizationBand(0.50, 0.80, 1.5), // 50-80% utilization: moderate queuing multiplier
        UtilizationBand(0.80, 0.95, 3.0), // 80-95% utilization: high congestion multiplier
        UtilizationBand(0.95, 1.00, 8.0)  // 95-100% utilization: severe saturation barrier
    };
}

NetworkConfig NetworkConfig::createDefault8RegionTopology() {
    NetworkConfig cfg;
    cfg.name = "Default-8-Region-6-Server-CDN";
    cfg.latencyWeight = 1.0;
    cfg.bandwidthWeight = 2.0;
    cfg.costScaleFactor = 100;
    cfg.originPenaltyCost = 250.0;
    cfg.baseTransitCost = 5.0;

    // 8 Ingress Regions with realistic traffic demands
    cfg.regions = {
        RegionConfig("Pune",      1500, 110.0, 250.0),
        RegionConfig("Mumbai",    3500, 105.0, 250.0),
        RegionConfig("Delhi",     3000, 120.0, 250.0),
        RegionConfig("Bangalore", 2500, 115.0, 250.0),
        RegionConfig("Chennai",   2000, 125.0, 250.0),
        RegionConfig("Hyderabad", 2000, 118.0, 250.0),
        RegionConfig("Kolkata",   1500, 135.0, 250.0),
        RegionConfig("Ahmedabad", 1200, 115.0, 250.0)
    };

    // 6 Distributed Edge Server PoPs with convex tiers
    auto bands = createStandardConvexBands();
    cfg.servers = {
        EdgeServerConfig("Mumbai",    6000, 6.0, 10.0),
        EdgeServerConfig("Delhi",     5500, 8.5, 12.0),
        EdgeServerConfig("Bangalore", 5000, 7.5, 12.0),
        EdgeServerConfig("Pune",      4000, 8.0, 15.0),
        EdgeServerConfig("Chennai",   4000, 7.0, 14.0),
        EdgeServerConfig("Kolkata",   3500, 9.0, 18.0)
    };
    for (auto& s : cfg.servers) {
        s.bands = bands;
    }

    // Regional interconnect links with bandwidth and realistic geographic latencies
    cfg.links = {
        // From Pune
        LinkConfig("Pune", "Pune",      4000, 5.0),
        LinkConfig("Pune", "Mumbai",    3500, 12.0),
        LinkConfig("Pune", "Bangalore", 2500, 25.0),
        LinkConfig("Pune", "Hyderabad", 2000, 22.0),

        // From Mumbai
        LinkConfig("Mumbai", "Mumbai",    6000, 5.0),
        LinkConfig("Mumbai", "Pune",      3500, 12.0),
        LinkConfig("Mumbai", "Ahmedabad", 2500, 18.0),
        LinkConfig("Mumbai", "Bangalore", 2500, 28.0),
        LinkConfig("Mumbai", "Delhi",     2000, 38.0),

        // From Delhi
        LinkConfig("Delhi", "Delhi",     5500, 5.0),
        LinkConfig("Delhi", "Ahmedabad", 2500, 28.0),
        LinkConfig("Delhi", "Kolkata",   2000, 35.0),
        LinkConfig("Delhi", "Mumbai",    2500, 38.0),

        // From Bangalore
        LinkConfig("Bangalore", "Bangalore", 5000, 5.0),
        LinkConfig("Bangalore", "Chennai",   3500, 12.0),
        LinkConfig("Bangalore", "Hyderabad", 2500, 18.0),
        LinkConfig("Bangalore", "Pune",      2000, 25.0),
        LinkConfig("Bangalore", "Mumbai",    2000, 28.0),

        // From Chennai
        LinkConfig("Chennai", "Chennai",   4000, 5.0),
        LinkConfig("Chennai", "Bangalore", 3500, 12.0),
        LinkConfig("Chennai", "Hyderabad", 2500, 20.0),
        LinkConfig("Chennai", "Kolkata",   2000, 38.0),

        // From Hyderabad
        LinkConfig("Hyderabad", "Bangalore", 3000, 18.0),
        LinkConfig("Hyderabad", "Chennai",   2500, 20.0),
        LinkConfig("Hyderabad", "Pune",      2000, 22.0),
        LinkConfig("Hyderabad", "Mumbai",    2500, 26.0),
        LinkConfig("Hyderabad", "Delhi",     2000, 40.0),
        LinkConfig("Hyderabad", "Kolkata",   2000, 35.0),

        // From Kolkata
        LinkConfig("Kolkata", "Kolkata",   3500, 5.0),
        LinkConfig("Kolkata", "Delhi",     2500, 35.0),
        LinkConfig("Kolkata", "Chennai",   2000, 38.0),
        LinkConfig("Kolkata", "Hyderabad", 2000, 35.0),

        // From Ahmedabad
        LinkConfig("Ahmedabad", "Mumbai",    3000, 18.0),
        LinkConfig("Ahmedabad", "Pune",      2000, 22.0),
        LinkConfig("Ahmedabad", "Delhi",     2500, 28.0)
    };

    return cfg;
}

NetworkConfig NetworkConfig::createCounterexampleTopology() {
    NetworkConfig cfg;
    cfg.name = "Greedy-vs-MCMF-Counterexample";
    cfg.latencyWeight = 1.0;
    cfg.bandwidthWeight = 0.0; // Isolate latency/link costs for clean mathematical demonstration
    cfg.costScaleFactor = 100;
    cfg.originPenaltyCost = 150.0; // High penalty for cache miss / unserved traffic
    cfg.baseTransitCost = 0.0;

    // 2 Client Regions: Region A and Region B, each with demand = 1000
    cfg.regions = {
        RegionConfig("Region_A", 1000, 100.0, 150.0),
        RegionConfig("Region_B", 1000, 100.0, 150.0)
    };

    // 2 Edge Servers: Server X and Server Y, each with capacity = 1000
    // Single 100% band with 1.0x multiplier to isolate routing decisions
    std::vector<UtilizationBand> singleBand = { UtilizationBand(0.0, 1.0, 1.0) };
    cfg.servers = {
        EdgeServerConfig("Server_X", 1000, 0.0, 0.0),
        EdgeServerConfig("Server_Y", 1000, 0.0, 0.0)
    };
    cfg.servers[0].bands = singleBand;
    cfg.servers[1].bands = singleBand;

    // Links:
    // Region A can access Server X (Cost = 10) OR Server Y (Cost = 25)
    // Region B can ONLY access Server X (Cost = 12); NO LINK to Server Y!
    cfg.links = {
        LinkConfig("Region_A", "Server_X", 1000, 10.0),
        LinkConfig("Region_A", "Server_Y", 1000, 25.0),
        LinkConfig("Region_B", "Server_X", 1000, 12.0)
    };

    return cfg;
}

NetworkConfig NetworkConfig::createSmall3Region3ServerTopology() {
    NetworkConfig cfg;
    cfg.name = "Small-3-Region-3-Server-Trace-Example";
    cfg.latencyWeight = 1.0;
    cfg.bandwidthWeight = 2.0;
    cfg.costScaleFactor = 100;
    cfg.originPenaltyCost = 250.0;
    cfg.baseTransitCost = 5.0;

    // 3 Client Ingress Regions (clean demands summing to 370 req/s)
    cfg.regions = {
        RegionConfig("Pune_Reg",   100, 120.0, 250.0),
        RegionConfig("Mumbai_Reg", 150, 120.0, 250.0),
        RegionConfig("Delhi_Reg",  120, 120.0, 250.0)
    };

    // 3 Distributed Edge Servers (capacities summing to 420 req/s > 370)
    std::vector<UtilizationBand> singleBand = { UtilizationBand(0.0, 1.0, 1.0) };
    cfg.servers = {
        EdgeServerConfig("Alpha_S1", 120, 5.0, 10.0),
        EdgeServerConfig("Beta_S2",  150, 8.0, 12.0),
        EdgeServerConfig("Gamma_S3", 150, 10.0, 14.0)
    };
    cfg.servers[0].bands = singleBand;
    cfg.servers[1].bands = singleBand;
    cfg.servers[2].bands = singleBand;

    // Links:
    // Pune_Reg -> Alpha_S1 (cap 100, lat 10.0, bw 5.0 -> unit cost = 1.0*10 + 2.0*5 = 20.0, scaled 2000)
    // Pune_Reg -> Beta_S2  (cap 80,  lat 25.0, bw 8.0 -> unit cost = 1.0*25 + 2.0*8 = 41.0, scaled 4100)
    // Mumbai_Reg -> Alpha_S1 (cap 80, lat 20.0, bw 5.0 -> unit cost = 1.0*20 + 2.0*5 = 30.0, scaled 3000)
    // Mumbai_Reg -> Beta_S2  (cap 150, lat 12.0, bw 8.0 -> unit cost = 1.0*12 + 2.0*8 = 28.0, scaled 2800)
    // Delhi_Reg -> Beta_S2   (cap 80,  lat 22.0, bw 8.0 -> unit cost = 1.0*22 + 2.0*8 = 38.0, scaled 3800)
    // Delhi_Reg -> Gamma_S3  (cap 150, lat 14.0, bw 10.0 -> unit cost = 1.0*14 + 2.0*10 = 34.0, scaled 3400)
    cfg.links = {
        LinkConfig("Pune_Reg",   "Alpha_S1", 100, 10.0),
        LinkConfig("Pune_Reg",   "Beta_S2",   80, 25.0),
        LinkConfig("Mumbai_Reg", "Alpha_S1",  80, 20.0),
        LinkConfig("Mumbai_Reg", "Beta_S2",  150, 12.0),
        LinkConfig("Delhi_Reg",  "Beta_S2",   80, 22.0),
        LinkConfig("Delhi_Reg",  "Gamma_S3", 150, 14.0)
    };

    return cfg;
}

NetworkConfig NetworkConfig::generateRandomNetwork(int numServers, unsigned int seed) {
    std::mt19937 rng(seed);
    NetworkConfig cfg;
    cfg.name = "Random-CDN-" + std::to_string(numServers) + "-Servers";
    cfg.latencyWeight = 1.0;
    cfg.bandwidthWeight = 2.0;
    cfg.costScaleFactor = 100;
    cfg.originPenaltyCost = 250.0;
    cfg.baseTransitCost = 5.0;

    int numRegions = std::max(5, numServers / 2);

    // Regions:
    for (int i = 0; i < numRegions; ++i) {
        std::string rName = "Reg_" + std::to_string(i + 1);
        int demand = 1200 + (rng() % 1600); // 1200 - 2800 req/s
        cfg.regions.emplace_back(rName, demand, 120.0, 250.0);
    }

    // Servers with standard 4 convex bands:
    auto bands = createStandardConvexBands();
    for (int j = 0; j < numServers; ++j) {
        std::string sName = "Srv_" + std::to_string(j + 1);
        int cap = 2500 + (rng() % 2500); // 2500 - 5000 req/s
        double bwCost = 4.0 + (rng() % 60) * 0.1; // 4.0 - 10.0
        double baseLat = 10.0 + (rng() % 150) * 0.1; // 10.0 - 25.0
        EdgeServerConfig sc(sName, cap, bwCost, baseLat);
        sc.bands = bands;
        cfg.servers.push_back(sc);
    }

    // Connect each region to up to 4 distinct servers
    std::vector<int> serverCoverage(numServers, 0);
    int linksPerRegion = std::min(numServers, 4);

    for (int i = 0; i < numRegions; ++i) {
        std::vector<int> candidates(numServers);
        std::iota(candidates.begin(), candidates.end(), 0);
        std::shuffle(candidates.begin(), candidates.end(), rng);

        for (int l = 0; l < linksPerRegion; ++l) {
            int sIdx = candidates[l];
            serverCoverage[sIdx]++;
            int cap = 1000 + (rng() % 1200); // 1000 - 2200 req/s
            double lat = 12.0 + (rng() % 350) * 0.1; // 12.0 - 47.0 ms
            cfg.links.emplace_back(cfg.regions[i].name, cfg.servers[sIdx].name, cap, lat);
        }
    }

    // Ensure every server has at least one incoming link
    for (int j = 0; j < numServers; ++j) {
        if (serverCoverage[j] == 0) {
            int rIdx = rng() % numRegions;
            int cap = 1000 + (rng() % 1200);
            double lat = 15.0 + (rng() % 300) * 0.1;
            cfg.links.emplace_back(cfg.regions[rIdx].name, cfg.servers[j].name, cap, lat);
        }
    }

    return cfg;
}

std::string NetworkConfig::toJsonString(int indent) const {
    std::ostringstream ss;
    std::string sp(indent, ' ');
    std::string sp2(indent * 2, ' ');
    std::string sp3(indent * 3, ' ');

    ss << "{\n";
    ss << sp << "\"name\": \"" << name << "\",\n";
    ss << sp << "\"latencyWeight\": " << latencyWeight << ",\n";
    ss << sp << "\"bandwidthWeight\": " << bandwidthWeight << ",\n";
    ss << sp << "\"costScaleFactor\": " << costScaleFactor << ",\n";
    ss << sp << "\"originPenaltyCost\": " << originPenaltyCost << ",\n";
    ss << sp << "\"baseTransitCost\": " << baseTransitCost << ",\n";

    // Regions
    ss << sp << "\"regions\": [\n";
    for (size_t i = 0; i < regions.size(); ++i) {
        const auto& r = regions[i];
        ss << sp2 << "{\"name\": \"" << r.name << "\", "
           << "\"demand\": " << r.demand << ", "
           << "\"originLatency\": " << r.originLatency << ", "
           << "\"originCost\": " << r.originCost << "}"
           << (i + 1 < regions.size() ? "," : "") << "\n";
    }
    ss << sp << "],\n";

    // Servers
    ss << sp << "\"servers\": [\n";
    for (size_t i = 0; i < servers.size(); ++i) {
        const auto& s = servers[i];
        ss << sp2 << "{\n";
        ss << sp3 << "\"name\": \"" << s.name << "\",\n";
        ss << sp3 << "\"capacity\": " << s.capacity << ",\n";
        ss << sp3 << "\"bandwidthCost\": " << s.bandwidthCost << ",\n";
        ss << sp3 << "\"baseLatency\": " << s.baseLatency << ",\n";
        ss << sp3 << "\"bands\": [\n";
        for (size_t b = 0; b < s.bands.size(); ++b) {
            ss << sp3 << "  {\"lowerPct\": " << s.bands[b].lowerPct
               << ", \"upperPct\": " << s.bands[b].upperPct
               << ", \"costMultiplier\": " << s.bands[b].costMultiplier << "}"
               << (b + 1 < s.bands.size() ? "," : "") << "\n";
        }
        ss << sp3 << "]\n";
        ss << sp2 << "}" << (i + 1 < servers.size() ? "," : "") << "\n";
    }
    ss << sp << "],\n";

    // Links
    ss << sp << "\"links\": [\n";
    for (size_t i = 0; i < links.size(); ++i) {
        const auto& l = links[i];
        ss << sp2 << "{\"fromRegion\": \"" << l.fromRegion << "\", "
           << "\"toServer\": \"" << l.toServer << "\", "
           << "\"capacity\": " << l.capacity << ", "
           << "\"latency\": " << l.latency << "}"
           << (i + 1 < links.size() ? "," : "") << "\n";
    }
    ss << sp << "]\n";
    ss << "}\n";

    return ss.str();
}

bool NetworkConfig::saveToJsonFile(const std::string& filepath) const {
    std::ofstream ofs(filepath);
    if (!ofs.is_open()) return false;
    ofs << toJsonString();
    return true;
}

// Minimal, robust standard C++17 JSON parser
namespace JsonHelper {
    struct Token {
        enum Type { STRING, NUMBER, BOOL, LBRACE, RBRACE, LBRACKET, RBRACKET, COLON, COMMA, END_OF_FILE };
        Type type;
        std::string value;
    };

    static std::vector<Token> tokenize(const std::string& src) {
        std::vector<Token> tokens;
        size_t i = 0;
        while (i < src.size()) {
            if (std::isspace(src[i])) { i++; continue; }
            if (src[i] == '{') { tokens.push_back({Token::LBRACE, "{"}); i++; }
            else if (src[i] == '}') { tokens.push_back({Token::RBRACE, "}"}); i++; }
            else if (src[i] == '[') { tokens.push_back({Token::LBRACKET, "["}); i++; }
            else if (src[i] == ']') { tokens.push_back({Token::RBRACKET, "]"}); i++; }
            else if (src[i] == ':') { tokens.push_back({Token::COLON, ":"}); i++; }
            else if (src[i] == ',') { tokens.push_back({Token::COMMA, ","}); i++; }
            else if (src[i] == '"') {
                std::string s;
                i++;
                while (i < src.size() && src[i] != '"') {
                    if (src[i] == '\\' && i + 1 < src.size()) i++;
                    s += src[i++];
                }
                if (i < src.size()) i++;
                tokens.push_back({Token::STRING, s});
            } else if (std::isdigit(src[i]) || src[i] == '-' || src[i] == '.') {
                std::string num;
                while (i < src.size() && (std::isdigit(src[i]) || src[i] == '.' || src[i] == '-' || src[i] == '+' || src[i] == 'e' || src[i] == 'E')) {
                    num += src[i++];
                }
                tokens.push_back({Token::NUMBER, num});
            } else if (std::isalpha(src[i])) {
                std::string word;
                while (i < src.size() && std::isalpha(src[i])) word += src[i++];
                tokens.push_back({Token::BOOL, word});
            } else {
                i++;
            }
        }
        tokens.push_back({Token::END_OF_FILE, ""});
        return tokens;
    }
}

NetworkConfig NetworkConfig::loadFromJsonFile(const std::string& filepath) {
    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        std::cerr << ">> [CONFIG]: Could not open '" << filepath << "'. Using default 8-region topology.\n";
        return createDefault8RegionTopology();
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    std::string text = buffer.str();

    auto tokens = JsonHelper::tokenize(text);
    if (tokens.empty() || tokens[0].type != JsonHelper::Token::LBRACE) {
        return createDefault8RegionTopology();
    }

    NetworkConfig cfg;
    cfg.name = "Loaded-From-JSON";

    // Parse keys from root object
    size_t idx = 1;
    while (idx < tokens.size() && tokens[idx].type != JsonHelper::Token::RBRACE && tokens[idx].type != JsonHelper::Token::END_OF_FILE) {
        if (tokens[idx].type == JsonHelper::Token::STRING) {
            std::string key = tokens[idx].value;
            idx++; // skip key
            if (idx < tokens.size() && tokens[idx].type == JsonHelper::Token::COLON) idx++; // skip colon

            if (key == "name" && idx < tokens.size() && tokens[idx].type == JsonHelper::Token::STRING) {
                cfg.name = tokens[idx++].value;
            } else if (key == "latencyWeight" && idx < tokens.size() && tokens[idx].type == JsonHelper::Token::NUMBER) {
                cfg.latencyWeight = std::stod(tokens[idx++].value);
            } else if (key == "bandwidthWeight" && idx < tokens.size() && tokens[idx].type == JsonHelper::Token::NUMBER) {
                cfg.bandwidthWeight = std::stod(tokens[idx++].value);
            } else if (key == "costScaleFactor" && idx < tokens.size() && tokens[idx].type == JsonHelper::Token::NUMBER) {
                cfg.costScaleFactor = std::stoi(tokens[idx++].value);
            } else if (key == "originPenaltyCost" && idx < tokens.size() && tokens[idx].type == JsonHelper::Token::NUMBER) {
                cfg.originPenaltyCost = std::stod(tokens[idx++].value);
            } else if (key == "baseTransitCost" && idx < tokens.size() && tokens[idx].type == JsonHelper::Token::NUMBER) {
                cfg.baseTransitCost = std::stod(tokens[idx++].value);
            } else if (key == "regions" && idx < tokens.size() && tokens[idx].type == JsonHelper::Token::LBRACKET) {
                idx++; // skip [
                while (idx < tokens.size() && tokens[idx].type != JsonHelper::Token::RBRACKET) {
                    if (tokens[idx].type == JsonHelper::Token::LBRACE) {
                        idx++;
                        RegionConfig reg;
                        while (idx < tokens.size() && tokens[idx].type != JsonHelper::Token::RBRACE) {
                            if (tokens[idx].type == JsonHelper::Token::STRING) {
                                std::string rkey = tokens[idx++].value;
                                if (tokens[idx].type == JsonHelper::Token::COLON) idx++;
                                if (rkey == "name") reg.name = tokens[idx++].value;
                                else if (rkey == "demand") reg.demand = std::stoi(tokens[idx++].value);
                                else if (rkey == "originLatency") reg.originLatency = std::stod(tokens[idx++].value);
                                else if (rkey == "originCost") reg.originCost = std::stod(tokens[idx++].value);
                                else idx++;
                            } else {
                                idx++;
                            }
                        }
                        if (idx < tokens.size() && tokens[idx].type == JsonHelper::Token::RBRACE) idx++;
                        cfg.regions.push_back(reg);
                    } else {
                        idx++;
                    }
                }
                if (idx < tokens.size() && tokens[idx].type == JsonHelper::Token::RBRACKET) idx++;
            } else if (key == "servers" && idx < tokens.size() && tokens[idx].type == JsonHelper::Token::LBRACKET) {
                idx++; // skip [
                while (idx < tokens.size() && tokens[idx].type != JsonHelper::Token::RBRACKET) {
                    if (tokens[idx].type == JsonHelper::Token::LBRACE) {
                        idx++;
                        EdgeServerConfig srv;
                        while (idx < tokens.size() && tokens[idx].type != JsonHelper::Token::RBRACE) {
                            if (tokens[idx].type == JsonHelper::Token::STRING) {
                                std::string skey = tokens[idx++].value;
                                if (tokens[idx].type == JsonHelper::Token::COLON) idx++;
                                if (skey == "name") srv.name = tokens[idx++].value;
                                else if (skey == "capacity") srv.capacity = std::stoi(tokens[idx++].value);
                                else if (skey == "bandwidthCost") srv.bandwidthCost = std::stod(tokens[idx++].value);
                                else if (skey == "baseLatency") srv.baseLatency = std::stod(tokens[idx++].value);
                                else if (skey == "bands" && tokens[idx].type == JsonHelper::Token::LBRACKET) {
                                    idx++;
                                    while (idx < tokens.size() && tokens[idx].type != JsonHelper::Token::RBRACKET) {
                                        if (tokens[idx].type == JsonHelper::Token::LBRACE) {
                                            idx++;
                                            UtilizationBand ub;
                                            while (idx < tokens.size() && tokens[idx].type != JsonHelper::Token::RBRACE) {
                                                if (tokens[idx].type == JsonHelper::Token::STRING) {
                                                    std::string bkey = tokens[idx++].value;
                                                    if (tokens[idx].type == JsonHelper::Token::COLON) idx++;
                                                    if (bkey == "lowerPct") ub.lowerPct = std::stod(tokens[idx++].value);
                                                    else if (bkey == "upperPct") ub.upperPct = std::stod(tokens[idx++].value);
                                                    else if (bkey == "costMultiplier") ub.costMultiplier = std::stod(tokens[idx++].value);
                                                    else idx++;
                                                } else idx++;
                                            }
                                            if (tokens[idx].type == JsonHelper::Token::RBRACE) idx++;
                                            srv.bands.push_back(ub);
                                        } else idx++;
                                    }
                                    if (tokens[idx].type == JsonHelper::Token::RBRACKET) idx++;
                                } else idx++;
                            } else idx++;
                        }
                        if (tokens[idx].type == JsonHelper::Token::RBRACE) idx++;
                        if (srv.bands.empty()) srv.bands = createStandardConvexBands();
                        cfg.servers.push_back(srv);
                    } else idx++;
                }
                if (idx < tokens.size() && tokens[idx].type == JsonHelper::Token::RBRACKET) idx++;
            } else if (key == "links" && idx < tokens.size() && tokens[idx].type == JsonHelper::Token::LBRACKET) {
                idx++;
                while (idx < tokens.size() && tokens[idx].type != JsonHelper::Token::RBRACKET) {
                    if (tokens[idx].type == JsonHelper::Token::LBRACE) {
                        idx++;
                        LinkConfig lnk;
                        while (idx < tokens.size() && tokens[idx].type != JsonHelper::Token::RBRACE) {
                            if (tokens[idx].type == JsonHelper::Token::STRING) {
                                std::string lkey = tokens[idx++].value;
                                if (tokens[idx].type == JsonHelper::Token::COLON) idx++;
                                if (lkey == "fromRegion") lnk.fromRegion = tokens[idx++].value;
                                else if (lkey == "toServer") lnk.toServer = tokens[idx++].value;
                                else if (lkey == "capacity") lnk.capacity = std::stoi(tokens[idx++].value);
                                else if (lkey == "latency") lnk.latency = std::stod(tokens[idx++].value);
                                else idx++;
                            } else idx++;
                        }
                        if (tokens[idx].type == JsonHelper::Token::RBRACE) idx++;
                        cfg.links.push_back(lnk);
                    } else idx++;
                }
                if (idx < tokens.size() && tokens[idx].type == JsonHelper::Token::RBRACKET) idx++;
            } else {
                idx++;
            }
        } else {
            idx++;
        }
    }

    if (cfg.regions.empty() || cfg.servers.empty()) {
        std::cerr << ">> [CONFIG]: JSON missing regions or servers. Falling back to default.\n";
        return createDefault8RegionTopology();
    }

    return cfg;
}
