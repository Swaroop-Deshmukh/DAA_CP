package main

import (
	"context"
	"flag"
	"fmt"
	"os"
	"os/signal"
	"syscall"
	"time"

	"cdn_router/state"
)

func main() {
	redisAddr := flag.String("redis", "localhost:6379", "Redis server host:port")
	pollSec := flag.Int("interval", 2, "Polling interval in seconds")
	flag.Parse()

	fmt.Printf("=====================================================\n")
	fmt.Printf("  Go Redis Real-Time State Monitor (Phase 2)         \n")
	fmt.Printf("=====================================================\n")
	fmt.Printf("Connecting to Redis at: %s...\n", *redisAddr)

	client, err := state.NewClient(state.Config{
		Addr:    *redisAddr,
		Timeout: 2 * time.Second,
	})
	if err != nil {
		fmt.Printf("\n[ERROR] Connection failed: %v\n", err)
		fmt.Printf("Tip: Ensure Redis is running via Docker:\n")
		fmt.Printf("     docker run -d --name cdn-redis -p 6379:6379 redis:alpine\n\n")
		os.Exit(1)
	}
	defer client.Close()

	fmt.Printf("[OK] Connected to Redis! Monitoring keys: server:pune, server:mumbai, server:delhi, server:bangalore\n\n")

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	ticker := time.NewTicker(time.Duration(*pollSec) * time.Second)
	defer ticker.Stop()

	// Initial print
	printLatestStates(ctx, client)

	for {
		select {
		case <-sigChan:
			fmt.Println("\nStopping Redis monitor...")
			return
		case <-ticker.C:
			printLatestStates(ctx, client)
		}
	}
}

func printLatestStates(ctx context.Context, client *state.Client) {
	states, err := client.GetAllServerStates(ctx)
	if err != nil {
		fmt.Printf("[%s] Waiting for CDN simulator to publish states: %v\n", time.Now().Format("15:04:05"), err)
		return
	}

	fmt.Printf("\n--- Live State from Redis [%s] ---\n", time.Now().Format("15:04:05"))
	fmt.Printf("%-12s | %-10s | %-14s | %-10s | %-8s | %-12s | %-10s | %-10s\n",
		"SERVER", "STATUS", "TRAFFIC", "LATENCY", "CPU", "BANDWIDTH", "BW CAP", "COST (₹/GB)")
	fmt.Printf("------------------------------------------------------------------------------------------------------\n")

	for _, name := range state.DefaultServerNames {
		s, ok := states[name]
		if !ok {
			fmt.Printf("%-12s | %-10s | %-14s | %-10s | %-8s | %-12s | %-10s | %-10s\n",
				stringsToUpper(name), "OFFLINE", "--", "--", "--", "--", "--", "--")
			continue
		}

		trafficStr := fmt.Sprintf("%.0f / %.0f", s.CurrentRequests, s.Capacity)
		latStr := s.Latency
		if latStr != "INF" {
			latStr += " ms"
		}
		cpuStr := fmt.Sprintf("%.1f%%", s.CPU)
		bwStr := fmt.Sprintf("%.1f Mbps", s.BandwidthUsage)
		bwCapStr := fmt.Sprintf("%.1f Mbps", s.BandwidthCapacity)
		costStr := fmt.Sprintf("₹%.2f", s.BandwidthCost)

		fmt.Printf("%-12s | %-10s | %-14s | %-10s | %-8s | %-12s | %-10s | %-10s\n",
			stringsToUpper(s.Name), s.Status, trafficStr, latStr, cpuStr, bwStr, bwCapStr, costStr)
	}
	fmt.Printf("------------------------------------------------------------------------------------------------------\n")
}

func stringsToUpper(s string) string {
	if s == "" {
		return ""
	}
	// Basic uppercase converter without importing strings for small utility
	b := []byte(s)
	for i, c := range b {
		if c >= 'a' && c <= 'z' {
			b[i] = c - 32
		}
	}
	return string(b)
}
