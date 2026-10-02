package state

import (
	"fmt"
	"strings"
)

// NodeType classifies nodes in the CDN flow network.
type NodeType string

const (
	NodeSource NodeType = "SOURCE"
	NodeEdge   NodeType = "EDGE_POP"
	NodeSink   NodeType = "SINK"
)

// Node represents a vertex in the CDN flow graph.
type Node struct {
	ID   int      `json:"id"`
	Name string   `json:"name"`
	Type NodeType `json:"type"`
}

// Edge represents a directed connection in the residual network.
type Edge struct {
	From        int     `json:"from"`
	To          int     `json:"to"`
	Capacity    float64 `json:"capacity"`
	Flow        float64 `json:"flow"`
	Cost        float64 `json:"cost"`
	RevIdx      int     `json:"rev_idx"`     // Index of reverse edge in Adj[To]
	IsResidual  bool    `json:"is_residual"` // True if backward edge
}

// ResidualCapacity returns remaining headroom (Capacity - Flow).
func (e *Edge) ResidualCapacity() float64 {
	return e.Capacity - e.Flow
}

// CostWeights defines the alpha and beta multipliers for composite routing cost.
type CostWeights struct {
	Alpha double // Latency weight (cost per ms)
	Beta  double // Bandwidth cost weight (cost per ₹/GB)
}

type double = float64

// DefaultCostWeights sets alpha=1.0, beta=2.0.
var DefaultCostWeights = CostWeights{Alpha: 1.0, Beta: 2.0}

// Graph represents the CDN Flow Network with residual adjacency list.
type Graph struct {
	NumNodes int
	Nodes    []Node
	Adj      [][]Edge
	Weights  CostWeights
}

// NewCDNGraph creates a new CDN flow graph with 6 default nodes.
// 0: Source (User Traffic)
// 1: Pune
// 2: Mumbai
// 3: Delhi
// 4: Bangalore
// 5: Sink (Origin Data Center)
func NewCDNGraph(weights CostWeights) *Graph {
	g := &Graph{
		NumNodes: 6,
		Nodes: []Node{
			{ID: 0, Name: "Source (User Traffic)", Type: NodeSource},
			{ID: 1, Name: "Pune", Type: NodeEdge},
			{ID: 2, Name: "Mumbai", Type: NodeEdge},
			{ID: 3, Name: "Delhi", Type: NodeEdge},
			{ID: 4, Name: "Bangalore", Type: NodeEdge},
			{ID: 5, Name: "Sink (Origin)", Type: NodeSink},
		},
		Adj:     make([][]Edge, 6),
		Weights: weights,
	}
	return g
}

// AddEdge adds both a forward edge and its corresponding backward residual edge.
// Forward edge: capacity = C, cost = W, flow = 0
// Backward edge: capacity = 0, cost = -W, flow = 0
func (g *Graph) AddEdge(from, to int, capacity, cost float64) {
	forwardEdge := Edge{
		From:       from,
		To:         to,
		Capacity:   capacity,
		Flow:       0.0,
		Cost:       cost,
		RevIdx:     len(g.Adj[to]),
		IsResidual: false,
	}

	backwardEdge := Edge{
		From:       to,
		To:         from,
		Capacity:   0.0,
		Flow:       0.0,
		Cost:       -cost,
		RevIdx:     len(g.Adj[from]),
		IsResidual: true,
	}

	g.Adj[from] = append(g.Adj[from], forwardEdge)
	g.Adj[to] = append(g.Adj[to], backwardEdge)
}

// CalculateCost computes: Cost = (Alpha * Latency) + (Beta * BandwidthCost)
func (g *Graph) CalculateCost(latencyMs, bwCostPerGB float64, isDown bool) float64 {
	if isDown {
		return 9999.0
	}
	return (g.Weights.Alpha * latencyMs) + (g.Weights.Beta * bwCostPerGB)
}

// BuildFromStates dynamically constructs the graph from edge server states.
func (g *Graph) BuildFromStates(serverStates map[string]*ServerState) {
	// Clear previous adjacency lists
	for i := 0; i < g.NumNodes; i++ {
		g.Adj[i] = []Edge{}
	}

	cityToNode := map[string]int{
		"pune":      1,
		"mumbai":    2,
		"delhi":     3,
		"bangalore": 4,
	}

	const sourceID = 0
	const sinkID = 5

	for city, nodeID := range cityToNode {
		state, exists := serverStates[city]
		if !exists {
			continue
		}

		isDown := (strings.ToUpper(state.Status) == "DOWN")
		
		// Parse latency
		latVal := 25.0
		if state.Latency != "INF" && state.Latency != "" {
			fmt.Sscanf(state.Latency, "%f", &latVal)
		} else if isDown {
			latVal = 999.0
		}

		// Calculate routing cost
		cost := g.CalculateCost(latVal, state.BandwidthCost, isDown)
		capacity := state.Capacity
		if isDown {
			capacity = 0.0
		}

		// 1. Edge: Source -> Edge Server
		g.AddEdge(sourceID, nodeID, capacity, cost)

		// 2. Edge: Edge Server -> Origin Sink
		backhaulCap := capacity
		backhaulCost := 5.0
		g.AddEdge(nodeID, sinkID, backhaulCap, backhaulCost)
	}
}

// DisplayGraph prints the complete graph structure for verification.
func (g *Graph) DisplayGraph() {
	fmt.Printf("\n========================================================================================\n")
	fmt.Printf("                   CDN FLOW NETWORK GRAPH (Phase 3 Go Representation)                   \n")
	fmt.Printf("========================================================================================\n")
	fmt.Printf(" Cost Formula: Cost = (%.1f * Latency) + (%.1f * BandwidthCost)\n", g.Weights.Alpha, g.Weights.Beta)
	fmt.Printf(" Nodes: %d [0: Source, 1: Pune, 2: Mumbai, 3: Delhi, 4: Bangalore, 5: Sink]\n", g.NumNodes)
	fmt.Printf("----------------------------------------------------------------------------------------\n")
	fmt.Printf("%-24s | %-24s | %-10s | %-8s | %-12s | %-8s | %-10s\n",
		"FROM NODE", "TO NODE", "CAPACITY", "FLOW", "RESIDUAL CAP", "COST", "EDGE TYPE")
	fmt.Printf("----------------------------------------------------------------------------------------\n")

	for u := 0; u < g.NumNodes; u++ {
		for _, e := range g.Adj[u] {
			fromName := fmt.Sprintf("%s (%d)", g.Nodes[e.From].Name, e.From)
			toName := fmt.Sprintf("%s (%d)", g.Nodes[e.To].Name, e.To)
			edgeType := "FORWARD"
			if e.IsResidual {
				edgeType = "BACKWARD"
			}

			fmt.Printf("%-24s | %-24s | %-10.1f | %-8.1f | %-12.1f | %-8.2f | %-10s\n",
				fromName, toName, e.Capacity, e.Flow, e.ResidualCapacity(), e.Cost, edgeType)
		}
	}
	fmt.Printf("========================================================================================\n\n")
}
