package state

import (
	"context"
	"fmt"
	"time"

	"github.com/redis/go-redis/v9"
)

// Client wraps the official go-redis Client with connection health checks and error handling.
type Client struct {
	rdb  *redis.Client
	addr string
}

// Config holds Redis connection parameters.
type Config struct {
	Addr     string // e.g. "localhost:6379"
	Password string // empty if no auth
	DB       int    // default 0
	Timeout  time.Duration
}

// NewClient initializes a new Go Redis client and verifies connection with PING.
func NewClient(cfg Config) (*Client, error) {
	if cfg.Addr == "" {
		cfg.Addr = "localhost:6379"
	}
	if cfg.Timeout == 0 {
		cfg.Timeout = 2 * time.Second
	}

	rdb := redis.NewClient(&redis.Options{
		Addr:         cfg.Addr,
		Password:     cfg.Password,
		DB:           cfg.DB,
		DialTimeout:  cfg.Timeout,
		ReadTimeout:  cfg.Timeout,
		WriteTimeout: cfg.Timeout,
	})

	// Perform Ping verification with context timeout
	ctx, cancel := context.WithTimeout(context.Background(), cfg.Timeout)
	defer cancel()

	if err := rdb.Ping(ctx).Err(); err != nil {
		rdb.Close()
		return nil, fmt.Errorf("redis connection failed at %s: %w (ensure Redis is running)", cfg.Addr, err)
	}

	return &Client{
		rdb:  rdb,
		addr: cfg.Addr,
	}, nil
}

// Ping checks if the Redis server is currently reachable.
func (c *Client) Ping(ctx context.Context) error {
	if c.rdb == nil {
		return fmt.Errorf("redis client is uninitialized")
	}
	return c.rdb.Ping(ctx).Err()
}

// GetUnderlyingClient exposes the raw go-redis client if needed.
func (c *Client) GetUnderlyingClient() *redis.Client {
	return c.rdb
}

// Close gracefully closes the Redis connection.
func (c *Client) Close() error {
	if c.rdb != nil {
		return c.rdb.Close()
	}
	return nil
}
