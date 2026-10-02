package main

import (
	"flag"
	"fmt"

	"cdn_router/state"
)

func main() {
	demand := flag.Float64("demand", 10000.0, "Target CDN traffic demand in req/s")
	flag.Parse()

	fmt.Println("========================================================================================")
	fmt.Println("        ADAPTIVE CDN TRAFFIC ROUTER -- PHASE 4: MIN-COST MAX-FLOW (Go)                 ")
	fmt.Println("========================================================================================")

	weights := state.CostWeights{Alpha: 1.0, Beta: 2.0}
	g := state.NewCDNGraph(weights)

	// User benchmark setup:
	// Pune: cap 5000, cost 8
	// Mumbai: cap 4000, cost 5
	// Delhi: cap 3000, cost 3
	// Bangalore: cap 3000, cost 6
	g.AddEdge(0, 1, 5000.0, 8.0)
	g.AddEdge(0, 2, 4000.0, 5.0)
	g.AddEdge(0, 3, 3000.0, 3.0)
	g.AddEdge(0, 4, 3000.0, 6.0)

	g.AddEdge(1, 5, 5000.0, 0.0)
	g.AddEdge(2, 5, 4000.0, 0.0)
	g.AddEdge(3, 5, 3000.0, 0.0)
	g.AddEdge(4, 5, 3000.0, 0.0)

	res := g.SolveMCMF(0, 5, *demand)

	fmt.Printf(" Requested Traffic:   %.0f req/s\n", res.RequestedTraffic)
	fmt.Printf(" Successfully Routed: %.0f req/s\n", res.RoutedTraffic)
	fmt.Printf(" Total Cost:          ₹%.2f\n", res.TotalCost)
	fmt.Printf(" Augmentations:       %d\n", res.NumAugmentations)
	fmt.Printf(" Execution Time:      %.3f ms\n", res.ExecutionTimeMs)
	fmt.Printf(" Fully Satisfied:     %t\n", res.FullyRouted)
	fmt.Println("----------------------------------------------------------------------------------------")
	fmt.Printf("%-14s | %-16s | %-16s\n", "SERVER", "ASSIGNED FLOW", "SUBTOTAL COST")
	fmt.Println("----------------------------------------------------------------------------------------")
	for server, flow := range res.Allocation {
		cost := res.ServerCost[server]
		fmt.Printf("%-14s | %-16.0f | ₹%-16.2f\n", server, flow, cost)
	}
	fmt.Println("========================================================================================")
}
