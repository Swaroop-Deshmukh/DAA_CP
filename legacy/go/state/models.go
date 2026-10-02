package state

import (
	"fmt"
	"strconv"
	"time"
)

// ServerState represents the dynamic condition of a CDN Edge Server in Redis.
// Maps directly to Redis Hash fields: server:<city_name_lower>
type ServerState struct {
	Name              string  `redis:"-" json:"name"`
	Capacity          float64 `redis:"capacity" json:"capacity"`
	CurrentRequests   float64 `redis:"current_requests" json:"current_requests"`
	Latency           string  `redis:"latency" json:"latency"` // e.g. "25.0" or "INF" when DOWN
	CPU               float64 `redis:"cpu" json:"cpu"`
	BandwidthUsage    float64 `redis:"bandwidth_usage" json:"bandwidth_usage"`
	BandwidthCapacity float64 `redis:"bandwidth_capacity" json:"bandwidth_capacity"`
	BandwidthCost     float64 `redis:"bandwidth_cost" json:"bandwidth_cost"`
	Status            string  `redis:"status" json:"status"` // "HEALTHY", "CONGESTED", "DOWN"
	Timestamp         string  `redis:"timestamp" json:"timestamp"`
}

// ToHashValues converts the struct to a map[string]interface{} for Redis HSET.
func (s *ServerState) ToHashValues() map[string]interface{} {
	ts := s.Timestamp
	if ts == "" {
		ts = time.Now().UTC().Format(time.RFC3339)
	}

	return map[string]interface{}{
		"capacity":           fmt.Sprintf("%.1f", s.Capacity),
		"current_requests":   fmt.Sprintf("%.1f", s.CurrentRequests),
		"latency":            s.Latency,
		"cpu":                fmt.Sprintf("%.1f", s.CPU),
		"bandwidth_usage":    fmt.Sprintf("%.1f", s.BandwidthUsage),
		"bandwidth_capacity": fmt.Sprintf("%.1f", s.BandwidthCapacity),
		"bandwidth_cost":     fmt.Sprintf("%.2f", s.BandwidthCost),
		"status":             s.Status,
		"timestamp":          ts,
	}
}

// FromHashMap parses a map of string values from Redis HGETALL into a ServerState struct.
func FromHashMap(name string, m map[string]string) (*ServerState, error) {
	if len(m) == 0 {
		return nil, fmt.Errorf("empty server hash for %s", name)
	}

	capVal, _ := strconv.ParseFloat(m["capacity"], 64)
	reqVal, _ := strconv.ParseFloat(m["current_requests"], 64)
	cpuVal, _ := strconv.ParseFloat(m["cpu"], 64)
	bwVal, _ := strconv.ParseFloat(m["bandwidth_usage"], 64)
	bwCapVal, _ := strconv.ParseFloat(m["bandwidth_capacity"], 64)
	costVal, _ := strconv.ParseFloat(m["bandwidth_cost"], 64)

	return &ServerState{
		Name:              name,
		Capacity:          capVal,
		CurrentRequests:   reqVal,
		Latency:           m["latency"],
		CPU:               cpuVal,
		BandwidthUsage:    bwVal,
		BandwidthCapacity: bwCapVal,
		BandwidthCost:     costVal,
		Status:            m["status"],
		Timestamp:         m["timestamp"],
	}, nil
}
