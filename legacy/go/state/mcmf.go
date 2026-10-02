package state

import (
	"container/heap"
	"fmt"
	"math"
	"time"
)

// Item represents a node entry in the Dijkstra priority queue.
type Item struct {
	node int
	dist float64
}

type PriorityQueue []*Item

func (pq PriorityQueue) Len() int           { return len(pq) }
func (pq PriorityQueue) Less(i, j int) bool { return pq[i].dist < pq[j].dist }
func (pq PriorityQueue) Swap(i, j int)      { pq[i], pq[j] = pq[j], pq[i] }
func (pq *PriorityQueue) Push(x interface{}) {
	*pq = append(*pq, x.(*Item))
}
func (pq *PriorityQueue) Pop() interface{} {
	old := *pq
	n := len(old)
	item := old[n-1]
	*pq = old[0 : n-1]
	return item
}

// MCMFResult stores the output of the Min-Cost Max-Flow algorithm.
type MCMFResult struct {
	RequestedTraffic float64            `json:"requested_traffic"`
	RoutedTraffic    float64            `json:"routed_traffic"`
	TotalCost        float64            `json:"total_cost"`
	NumAugmentations int                `json:"num_augmentations"`
	ExecutionTimeMs  float64            `json:"execution_time_ms"`
	Allocation       map[string]float64 `json:"allocation"`
	ServerCost       map[string]float64 `json:"server_cost"`
	FullyRouted      bool               `json:"fully_routed"`
}

const (
	MCMFInf = 1e18
	MCMFEps = 1e-9
)

// SolveMCMF solves the Min-Cost Max-Flow problem for a given requested traffic demand.
// Uses Successive Shortest Path (SSP) with Dijkstra and Johnson's Node Potentials.
func (g *Graph) SolveMCMF(source, sink int, targetTraffic float64) MCMFResult {
	startTime := time.Now()

	n := g.NumNodes
	potential := make([]float64, n)
	dist := make([]float64, n)
	parentEdge := make([]int, n)
	parentNode := make([]int, n)

	var totalFlow, totalCost float64
	var augmentations int

	// Dijkstra with reduced costs
	dijkstra := func() bool {
		for i := 0; i < n; i++ {
			dist[i] = MCMFInf
			parentNode[i] = -1
			parentEdge[i] = -1
		}

		pq := &PriorityQueue{}
		heap.Init(pq)

		dist[source] = 0.0
		heap.Push(pq, &Item{node: source, dist: 0.0})

		for pq.Len() > 0 {
			top := heap.Pop(pq).(*Item)
			u := top.node
			d := top.dist

			if d > dist[u]+MCMFEps {
				continue
			}

			for i, edge := range g.Adj[u] {
				if edge.ResidualCapacity() <= MCMFEps {
					continue
				}

				reducedCost := edge.Cost + potential[u] - potential[edge.To]
				if reducedCost < -1e-6 {
					reducedCost = 0.0
				}

				if dist[edge.To] > dist[u]+reducedCost+MCMFEps {
					dist[edge.To] = dist[u] + reducedCost
					parentNode[edge.To] = u
					parentEdge[edge.To] = i
					heap.Push(pq, &Item{node: edge.To, dist: dist[edge.To]})
				}
			}
		}

		return dist[sink] < MCMFInf-1000.0
	}

	for totalFlow < targetTraffic-MCMFEps && dijkstra() {
		// Update node potentials
		for i := 0; i < n; i++ {
			if dist[i] < MCMFInf-1000.0 {
				potential[i] += dist[i]
			}
		}

		// Find bottleneck capacity along augmenting path
		bottleneck := targetTraffic - totalFlow
		for curr := sink; curr != source; curr = parentNode[curr] {
			p := parentNode[curr]
			edgeIdx := parentEdge[curr]
			bottleneck = math.Min(bottleneck, g.Adj[p][edgeIdx].ResidualCapacity())
		}

		if bottleneck <= MCMFEps {
			break
		}

		// Push flow and update residual capacities
		for curr := sink; curr != source; curr = parentNode[curr] {
			p := parentNode[curr]
			edgeIdx := parentEdge[curr]
			revIdx := g.Adj[p][edgeIdx].RevIdx

			g.Adj[p][edgeIdx].Flow += bottleneck
			g.Adj[curr][revIdx].Flow -= bottleneck
			totalCost += bottleneck * g.Adj[p][edgeIdx].Cost
		}

		totalFlow += bottleneck
		augmentations++
	}

	duration := time.Since(startTime).Seconds() * 1000.0

	// Extract allocations
	allocation := make(map[string]float64)
	serverCost := make(map[string]float64)
	nodeToCity := map[int]string{
		1: "Pune", 2: "Mumbai", 3: "Delhi", 4: "Bangalore",
	}

	for _, edge := range g.Adj[source] {
		if !edge.IsResidual && nodeToCity[edge.To] != "" {
			city := nodeToCity[edge.To]
			allocation[city] = edge.Flow
			serverCost[city] = edge.Flow * edge.Cost
		}
	}

	return MCMFResult{
		RequestedTraffic: targetTraffic,
		RoutedTraffic:    totalFlow,
		TotalCost:        totalCost,
		NumAugmentations: augmentations,
		ExecutionTimeMs:  duration,
		Allocation:       allocation,
		ServerCost:       serverCost,
		FullyRouted:      math.Abs(totalFlow-targetTraffic) <= MCMFEps,
	}
}
