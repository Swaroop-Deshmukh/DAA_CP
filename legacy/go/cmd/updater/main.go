package main

import (
	"context"
	"flag"
	"fmt"
	"os"
	"time"

	"cdn_router/state"
)

func main() {
	redisAddr := flag.String("redis", "localhost:6379", "Redis server host:port")
	flag.Parse()

	fmt.Println("Connecting to Redis at", *redisAddr)
	client, err := state.NewClient(state.Config{
		Addr:    *redisAddr,
		Timeout: 2 * time.Second,
	})
	if err != nil {
		fmt.Printf("Error: %v\n", err)
		os.Exit(1)
	}
	defer client.Close()

	ctx := context.Background()

	// Example: write state for Pune
	puneState := state.ServerState{
		Name:              "pune",
		Capacity:          5000.0,
		CurrentRequests:   3200.0,
		Latency:           "25.0",
		CPU:               64.0,
		BandwidthUsage:    600.0,
		BandwidthCapacity: 1000.0,
		BandwidthCost:     8.0,
		Status:            "HEALTHY",
		Timestamp:         time.Now().UTC().Format(time.RFC3339),
	}

	fmt.Println("Writing Pune state to Redis key: server:pune...")
	if err := client.WriteServerState(ctx, puneState); err != nil {
		fmt.Printf("Failed: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("Successfully wrote Pune state!")

	// Read back to verify
	fmt.Println("Reading back from Redis...")
	retrieved, err := client.GetServerState(ctx, "pune")
	if err != nil {
		fmt.Printf("Failed to read: %v\n", err)
		os.Exit(1)
	}

	fmt.Printf("Retrieved from Redis Hash:\n")
	fmt.Printf("  Capacity:         %.0f req/s\n", retrieved.Capacity)
	fmt.Printf("  Current Requests: %.0f req/s\n", retrieved.CurrentRequests)
	fmt.Printf("  Latency:          %s ms\n", retrieved.Latency)
	fmt.Printf("  CPU:              %.1f%%\n", retrieved.CPU)
	fmt.Printf("  Bandwidth:        %.1f Mbps\n", retrieved.BandwidthUsage)
	fmt.Printf("  Status:           %s\n", retrieved.Status)
	fmt.Printf("  Timestamp:        %s\n", retrieved.Timestamp)
}
