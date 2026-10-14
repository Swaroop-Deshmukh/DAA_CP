CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Iinclude

SRCS = src/GraphConfig.cpp \
       src/CDNServer.cpp \
       src/TrafficSimulator.cpp \
       src/Graph.cpp \
       src/MinCostMaxFlow.cpp \
       src/EdmondsKarp.cpp \
       src/Baselines.cpp \
       src/CacheKnapsack.cpp \
       src/HoltForecast.cpp \
       src/StabilityManager.cpp \
       src/SelfTest.cpp \
       src/Router.cpp \
       src/SimulationScenario.cpp \
       src/AlgorithmTrace.cpp \
       src/Benchmark.cpp \
       src/IpcEngine.cpp \
       src/main.cpp

BINDIR = bin
TARGET = $(BINDIR)/cdn_engine

all: $(TARGET)

$(TARGET): $(SRCS)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -rf $(BINDIR)

test: $(TARGET)
	./$(TARGET) --scenarios

selftest: $(TARGET)
	./$(TARGET) --selftest

counterexample: $(TARGET)
	./$(TARGET) --counterexample

network: $(TARGET)
	./$(TARGET) --network

.PHONY: all clean test selftest counterexample network
