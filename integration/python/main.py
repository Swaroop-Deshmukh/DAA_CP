"""
CDN Simulator Main Entrypoint (Phase 1).

Usage:
    python main.py          # Runs live interactive terminal simulation
    python main.py --demo   # Runs deterministic Phase 1 verification demo
    python main.py --web    # Launches modern web dashboard on http://localhost:8000
"""

import sys
import time
import argparse
import random

# Ensure UTF-8 output encoding on Windows consoles
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(encoding="utf-8")

from rich.console import Console
from rich.live import Live
from cdn_sim.simulator import CDNSimulator
from cdn_sim.display import build_fleet_dashboard
import demo


def run_live_simulation(interval: float = 1.0, auto_events: bool = True):
    """Runs a live auto-updating terminal simulation dashboard."""
    console = Console()
    sim = CDNSimulator()
    console.clear()

    # Pre-program some interesting natural events to showcase changing conditions
    scheduled_events = [
        (4, "Spike Pune to 4600 req/s", lambda: sim.trigger_spike("Pune", 4600.0)),
        (8, "Spike Pune to 4900 req/s (Severe Congestion)", lambda: sim.trigger_spike("Pune", 4900.0)),
        (13, "Normalize Pune traffic back to 3200 req/s", lambda: sim.set_traffic("Pune", 3200.0, smooth=True)),
        (17, "Fail Mumbai edge server (Outage)", lambda: sim.fail_server("Mumbai")),
        (22, "Surge on Delhi to 4800 req/s", lambda: sim.trigger_spike("Delhi", 4800.0)),
        (26, "Recover Mumbai edge server", lambda: sim.recover_server("Mumbai", initial_traffic=3800.0)),
        (30, "Normalize Delhi traffic", lambda: sim.set_traffic("Delhi", 3400.0, smooth=True)),
    ]
    event_idx = 0

    with Live(build_fleet_dashboard(sim.get_snapshot()), console=console, refresh_per_second=4, screen=False) as live:
        try:
            while True:
                # Check scheduled dynamic event
                if auto_events and event_idx < len(scheduled_events):
                    target_tick, desc, action = scheduled_events[event_idx]
                    if sim.tick_count >= target_tick:
                        action()
                        event_idx += 1

                # Advance simulation by 1 tick with realistic stochastic drift
                snapshot = sim.tick(stochastic_drift=True)
                live.update(build_fleet_dashboard(snapshot))
                time.sleep(interval)
        except KeyboardInterrupt:
            console.print("\n[yellow]Simulation paused by user (Ctrl+C).[/]")


def main():
    parser = argparse.ArgumentParser(description="CDN Edge Server Simulator — Phase 1")
    parser.add_argument("--demo", action="store_true", help="Run deterministic Phase 1 walkthrough demo")
    parser.add_argument("--web", action="store_true", help="Launch interactive web dashboard")
    parser.add_argument("--port", type=int, default=8000, help="Port for web dashboard (default: 8000)")
    parser.add_argument("--speed", type=float, default=1.0, help="Tick interval in seconds (default: 1.0s)")
    parser.add_argument("--no-events", action="store_true", help="Disable automated scenario events in live mode")

    args = parser.parse_args()

    if args.demo:
        demo.main()
    elif args.web:
        try:
            import uvicorn
            from web_app import app
            print(f"Starting CDN Simulator Web Dashboard at http://127.0.0.1:{args.port}...")
            uvicorn.run(app, host="127.0.0.1", port=args.port)
        except ImportError as e:
            print(f"Web server dependencies missing: {e}. Run 'python main.py' for terminal simulation.")
    else:
        run_live_simulation(interval=args.speed, auto_events=not args.no_events)


if __name__ == "__main__":
    main()
