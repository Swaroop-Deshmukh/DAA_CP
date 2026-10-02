#include "CDNServer.h"
#include <cassert>
#include <iostream>

int main() {
    std::cout << "Testing CDNServer component...\n";

    // 1. Initialization test
    CDNServer pune(1, "Pune", "West India", 5000.0, 25.0, 8.0, 3200.0);
    assert(pune.getId() == 1);
    assert(pune.getName() == "Pune");
    assert(pune.getMaximumCapacity() == 5000.0);
    assert(pune.getCurrentRequests() == 3200.0);
    assert(pune.getAvailableCapacity() == 1800.0);
    assert(pune.getCurrentLatency() == 25.0);
    assert(pune.getStatus() == ServerStatus::HEALTHY);
    assert(pune.getBandwidthUsage() == 600.0); // 3200 * 0.1875
    std::cout << " [PASS] Initialization & Baseline Metrics\n";

    // 2. Normal traffic fluctuation
    pune.setTraffic(3000.0);
    assert(pune.getCurrentRequests() == 3000.0);
    assert(pune.getStatus() == ServerStatus::HEALTHY);
    std::cout << " [PASS] Normal Traffic Fluctuation\n";

    // 3. Traffic spike & congestion
    pune.setTraffic(4900.0);
    assert(pune.getCurrentRequests() == 4900.0);
    assert(pune.getStatus() == ServerStatus::CONGESTED);
    assert(pune.getCurrentLatency() >= 115.0);
    assert(pune.getCpuUtilization() >= 95.0);
    std::cout << " [PASS] Spike & Congestion Degradation (Latency >= 115ms, Status = CONGESTED)\n";

    // 4. Failure test
    pune.fail();
    assert(pune.getStatus() == ServerStatus::FAILED);
    assert(pune.getAvailableCapacity() == 0.0);
    assert(pune.getCurrentLatency() >= 9999.0);
    std::cout << " [PASS] Server Failure (Capacity Clamped to 0, Latency = 9999ms, Status = FAILED)\n";

    // 5. Recovery test
    pune.recover(3200.0);
    assert(pune.getStatus() == ServerStatus::HEALTHY);
    assert(pune.getAvailableCapacity() == 1800.0);
    assert(pune.getCurrentLatency() == 25.0);
    std::cout << " [PASS] Server Recovery\n";

    std::cout << "\nAll CDNServer unit tests passed successfully!\n";
    return 0;
}
