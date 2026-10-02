package state

import (
	"context"
	"fmt"
	"strings"
	"time"
)

// DefaultServerNames defines the 4 CDN edge server nodes.
var DefaultServerNames = []string{"pune", "mumbai", "delhi", "bangalore"}

// FormatKey returns the standardized Redis Hash key for an edge server.
func FormatKey(serverName string) string {
	cleanName := strings.ToLower(strings.TrimSpace(serverName))
	return fmt.Sprintf("server:%s", cleanName)
}

// WriteServerState persists the latest dynamic conditions of an edge server into its Redis Hash.
// Executed as: HSET server:<name> capacity <val> current_requests <val> latency <val> ...
func (c *Client) WriteServerState(ctx context.Context, state ServerState) error {
	if c.rdb == nil {
		return fmt.Errorf("redis client is not connected")
	}

	key := FormatKey(state.Name)
	fields := state.ToHashValues()

	// Execute atomic HSet in Redis
	err := c.rdb.HSet(ctx, key, fields).Err()
	if err != nil {
		return fmt.Errorf("failed to write state for %s: %w", key, err)
	}

	return nil
}

// GetServerState retrieves the latest condition of a specific server from its Redis Hash.
// Executed as: HGETALL server:<name>
func (c *Client) GetServerState(ctx context.Context, serverName string) (*ServerState, error) {
	if c.rdb == nil {
		return nil, fmt.Errorf("redis client is not connected")
	}

	key := FormatKey(serverName)
	rawMap, err := c.rdb.HGetAll(ctx, key).Result()
	if err != nil {
		return nil, fmt.Errorf("failed to fetch hash for %s: %w", key, err)
	}

	if len(rawMap) == 0 {
		return nil, fmt.Errorf("no state found in Redis for key %s (server may not have reported yet)", key)
	}

	return FromHashMap(serverName, rawMap)
}

// GetAllServerStates queries the latest state for all 4 CDN edge nodes.
func (c *Client) GetAllServerStates(ctx context.Context) (map[string]*ServerState, error) {
	results := make(map[string]*ServerState)

	for _, name := range DefaultServerNames {
		s, err := c.GetServerState(ctx, name)
		if err != nil {
			// Skip or record as offline
			continue
		}
		results[name] = s
	}

	if len(results) == 0 {
		return nil, fmt.Errorf("no edge server states found in Redis under keys: server:pune, server:mumbai, server:delhi, server:bangalore")
	}

	return results, nil
}

// PollServerStates polls Redis every interval and calls the handler with updated states.
func (c *Client) PollServerStates(ctx context.Context, interval time.Duration, onUpdate func(map[string]*ServerState)) error {
	ticker := time.NewTicker(interval)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			return ctx.Err()
		case <-ticker.C:
			states, err := c.GetAllServerStates(ctx)
			if err == nil && len(states) > 0 {
				onUpdate(states)
			}
		}
	}
}
