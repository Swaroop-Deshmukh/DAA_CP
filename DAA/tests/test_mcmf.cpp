#include "Graph.h"
#include "MinCostMaxFlow.h"
#include <cassert>
#include <iostream>

int main() {
    std::cout << "Testing MinCostMaxFlow component...\n";

    // 1. User Benchmark Problem
    // Demand = 10,000 req/s
    // Pune (cap 5000, cost 800)
    // Mumbai (cap 4000, cost 500)
    // Delhi (cap 3000, cost 300)
    // Bangalore (cap 3000, cost 600)
    Graph g(6, 1.0, 2.0, 100);
    g.addEdge(0, 1, 5000, 800); // Pune
    g.addEdge(0, 2, 4000, 500); // Mumbai
    g.addEdge(0, 3, 3000, 300); // Delhi
    g.addEdge(0, 4, 3000, 600); // Bangalore

    g.addEdge(1, 5, 5000, 0);
    g.addEdge(2, 5, 4000, 0);
    g.addEdge(3, 5, 3000, 0);
    g.addEdge(4, 5, 3000, 0);

    MinCostMaxFlow mcmf(6);
    auto res = mcmf.solve(g, 0, 5, 10000);

    int totalFlow = res.first;
    long long totalCost = res.second;

    assert(totalFlow == 10000);
    // Cost: 3000*300 + 4000*500 + 3000*600 = 900,000 + 2,000,000 + 1,800,000 = 4,700,000 (scaled by 100 -> Rs. 47,000)
    assert(totalCost == 4700000);

    auto alloc = mcmf.extractServerAllocations(g);
    assert(alloc["Delhi"] == 3000);
    assert(alloc["Mumbai"] == 4000);
    assert(alloc["Bangalore"] == 3000);
    assert(alloc["Pune"] == 0);
    std::cout << " [PASS] User Benchmark: Delhi=3000, Mumbai=4000, Bangalore=3000, Pune=0 (Cost: Rs. 47,000)\n";

    // 2. Overload Test (18,000 req/s vs 15,000 total capacity)
    Graph gOverload(6, 1.0, 2.0, 100);
    gOverload.addEdge(0, 1, 5000, 800);
    gOverload.addEdge(0, 2, 4000, 500);
    gOverload.addEdge(0, 3, 3000, 300);
    gOverload.addEdge(0, 4, 3000, 600);

    gOverload.addEdge(1, 5, 5000, 0);
    gOverload.addEdge(2, 5, 4000, 0);
    gOverload.addEdge(3, 5, 3000, 0);
    gOverload.addEdge(4, 5, 3000, 0);

    MinCostMaxFlow mcmfOverload(6);
    auto resOverload = mcmfOverload.solve(gOverload, 0, 5, 18000);

    assert(resOverload.first == 15000); // Bounded by max fleet capacity
    auto allocOverload = mcmfOverload.extractServerAllocations(gOverload);
    assert(allocOverload["Delhi"] == 3000);
    assert(allocOverload["Mumbai"] == 4000);
    assert(allocOverload["Bangalore"] == 3000);
    assert(allocOverload["Pune"] == 5000);
    std::cout << " [PASS] Insufficient Capacity Handling (15,000 routed, all nodes saturated)\n";

    std::cout << "\nAll MinCostMaxFlow unit tests passed successfully!\n";
    return 0;
}
