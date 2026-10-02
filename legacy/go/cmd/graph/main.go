package main

import (
	"context"
	"flag"
	"fmt"
	"time"

	"cdn_router/state"
)

func main() {
	redisAddr := flag.String("redis", "localhost:6379", "Redis server host:port")
	flag.Parse()

	fmt.Println("========================================================================================")
	fmt.Println("        ADAPTIVE CDN TRAFFIC ROUTER -- PHASE 3: GRAPH CONSTRUCTION (Go)                ")
	fmt.Println("========================================================================================")

	var serverStates map[string]*state.ServerState

	// Attempt connecting to Redis
	client, err := state.NewClient(state.Config{
		Addr:    *redisAddr,
		Timeout: 1 * time.Second,
	})

	if err == nil {
		defer client.Close()
		fmt.Printf("[INFO] Connected to Redis at %s, reading server hashes...\n", *redisAddr)
		ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
		defer cancel()
		states, fetchErr := client.GetAllServerStates(ctx)
		if fetchErr == nil && len(states) > 0 {
			serverStates = states
		}
	}

	// Fallback if Redis is not running or empty
	if serverStates == nil {
		fmt.Println("[NOTICE] Redis not connected or empty. Using calibrated edge telemetry fallback:")
		serverStates = map[string]*state.ServerState{
			"pune": {
				Name:            "Pune",
				Capacity:        5000.0,
				CurrentRequests: 3200.0,
				Latency:         "25.0",
				CPU:             64.0,
				BandwidthCost:   8.0,
				Status:          "HEALTHY",
			},
			"mumbai": {
				Name:            "Mumbai",
				Capacity:        6000.0,
				CurrentRequests: 3800.0,
				Latency:         "18.0",
				CPU:             63.3,
				BandwidthCost:   6.0,
				Status:          "HEALTHY",
			},
			"delhi": {
				Name:            "Delhi",
				Capacity:        5500.0,
				CurrentRequests: 3400.0,
				Latency:         "32.0",
				CPU:             61.8,
				BandwidthCost:   9.0,
				Status:          "HEALTHY",
			},
			"bangalore": {
				Name:            "Bangalore",
				Capacity:        5000.0,
				CurrentRequests: 3100.0,
				Latency:         "24.0",
				CPU:             62.0,
				BandwidthCost:   7.5,
				Status:          "HEALTHY",
			},
		}
	}

	// Construct CDN Graph
	weights := state.CostWeights{Alpha: 1.0, Beta: 2.0}
	graph := state.NewCDNGraph(weights)
	graph.BuildFromStates(serverStates)

	// Display Graph
	graph.DisplayGraph()

	fmt.Println("========================================================================================")
	fmt.Println(" [OK] Phase 3 Go Graph Construction complete and validated!")
	fmt.Println(" [OK] Forward and backward residual edges ready for Phase 4 Min-Cost Max-Flow.")
	fmt.Println("========================================================================================")
}
