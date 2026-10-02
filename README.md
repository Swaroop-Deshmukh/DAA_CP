# Adaptive CDN Traffic Router with Min-Cost Max-Flow

> **Design and Analysis of Algorithms (DAA) Capstone Project**  
> Dynamic CDN Edge Simulation and Traffic Optimization using Min-Cost Max-Flow (MCMF)

---

## 🏛️ Project Directory Structure

This repository is strictly organized into decoupled modules for easy academic evaluation:

```
DAA_CP/
│
├── DAA/                      ◄── [EVALUATOR/TEACHER: OPEN THIS FOLDER]
│   ├── src/                  # Core C++ implementation files (.cpp)
│   │   ├── CDNServer.cpp
│   │   ├── TrafficSimulator.cpp
│   │   ├── Graph.cpp
│   │   ├── MinCostMaxFlow.cpp
│   │   ├── Router.cpp
│   │   ├── SimulationScenario.cpp
│   │   ├── Benchmark.cpp
│   │   └── main.cpp
│   ├── include/              # Modular C++ header files (.h)
│   │   ├── CDNServer.h
│   │   ├── TrafficSimulator.h
│   │   ├── Graph.h
│   │   ├── MinCostMaxFlow.h
│   │   ├── Router.h
│   │   ├── SimulationScenario.h
│   │   └── Benchmark.h
│   ├── bin/                  # Compiled binary executables (cdn_engine.exe)
│   ├── build/                # Intermediate build artifacts
│   ├── tests/                # Unit test files and documentation
│   ├── README.md             # Detailed DAA evaluator documentation
│   └── build.bat             # One-click Windows build script
│
├── integration/              # Real-time infrastructure & Web Dashboards
│   ├── python/               # Python physics simulator (cdn_sim/) & CLI scripts
│   ├── redis/                # Redis state-sync connectors (redis_syncer.py)
│   └── dashboard/            # FastAPI server & real-time glassmorphism web UI
│
├── legacy/                   # Historical reference implementations (Retained)
│   ├── go/                   # Previous Phase 1–5 Go implementation (state/, cmd/)
│   └── cpp/                  # Early monolithic C++ prototypes (mcmf.hpp, etc.)
│
├── data/                     # Canonical outputs & generated routing decisions
│   └── routing_decision.json # Current routing state exported for UI dashboards
│
├── docs/                     # Specifications, notes, and architecture diagrams
│
└── README.md                 # This overview guide
```

---

## 👨‍🏫 Evaluator Quick Start Guide

To inspect and test the **DAA Algorithm Implementation**, navigate to the `DAA/` directory:

```cmd
cd DAA
build.bat
```

This compiles all C++17 sources and executes the full verification suite (all 7 scenarios and the performance benchmarks).

### Running Specific Modes:
```cmd
cd DAA
bin\cdn_engine.exe --scenarios   # Run the 7 verification scenarios
bin\cdn_engine.exe --bench       # Run high-resolution benchmark suite
bin\cdn_engine.exe --loop        # Run live adaptive simulation daemon
bin\cdn_engine.exe --demo        # Run full demonstration & export JSON
```

For complete mathematical formulation, algorithmic proofs (Johnson's potentials, bottleneck capacity push), and complexity analysis, please review:
👉 **[DAA/README.md](file:///c:/Users/swaro/DAA_CP/DAA/README.md)**

---

## 🌐 Other Modules

- **`integration/`**:
  - `integration/dashboard/`: FastAPI web app (`web_app.py`) and HTML/CSS web dashboard (`static/index.html`).
  - `integration/redis/`: Redis synchronization client.
  - `integration/python/`: Python package (`cdn_sim`) and CLI demo (`demo.py`).
- **`legacy/`**:
  - Contains all earlier prototypes and reference implementations in Go and monolithic C++ headers, retained to preserve project history.
- **`data/`**:
  - `data/routing_decision.json`: The latest computed routing decision serialized in canonical JSON for dashboard consumption.
