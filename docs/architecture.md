# CDN Adaptive Traffic Router Architecture

## Project Phases
1. **Phase 1: CDN Edge Simulator**: Physics modeling of capacity, queuing delay, egress bandwidth, CPU load, and server outages.
2. **Phase 2: Redis State Synchronization**: Real-time distributed state store.
3. **Phase 3: CDN Flow Graph Construction**: Directed flow network with forward edges, reverse residual edges, and scaled integer costs.
4. **Phase 4: Min-Cost Max-Flow Engine**: Successive Shortest Path (SSP) with Dijkstra and Johnson's Node Potentials.
5. **Phase 5: Closed-Loop Adaptive Router**: Automated reaction to server condition drift, congestion, and failures.

## Directory Mapping
- **`DAA/`**: Core Design and Analysis of Algorithms implementation in pure C++17.
- **`integration/`**: Web dashboard, Redis connectors, and Python simulator.
- **`legacy/`**: Historic Go and prototype C++ code retained for reference.
- **`data/`**: JSON snapshots and telemetry exports.
