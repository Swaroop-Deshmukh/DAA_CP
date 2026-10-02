"""
Rich Terminal Display & Formatting for CDN Simulator.
Provides real-time terminal UI with color-coded status badges, gauges, and event tickers.
"""

import sys
from typing import Dict, Any, List

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(encoding="utf-8")

from rich.console import Console, Group
from rich.table import Table
from rich.panel import Panel
from rich.text import Text
from rich.layout import Layout
from rich import box


def make_load_bar(current: float, capacity: float, width: int = 12) -> str:
    """Generates a text progress bar representing capacity utilization."""
    if capacity <= 0:
        return "[red]N/A[/]"
    ratio = min(1.0, max(0.0, current / capacity))
    filled = int(round(ratio * width))
    empty = width - filled
    
    if ratio < 0.70:
        color = "green"
    elif ratio < 0.88:
        color = "yellow"
    else:
        color = "red"
        
    bar = "█" * filled + "░" * empty
    pct = int(ratio * 100)
    return f"[{color}]{bar}[/] {pct}%"


def format_status(status_str: str) -> Text:
    """Returns styled Rich Text badge for server status."""
    if status_str == "HEALTHY":
        return Text("● HEALTHY", style="bold green")
    elif status_str == "CONGESTED":
        return Text("▲ CONGESTED", style="bold yellow")
    elif status_str == "DOWN":
        return Text("✖ DOWN", style="bold white on red")
    return Text(status_str, style="white")


def format_latency(latency_val: Any) -> Text:
    """Color codes latency based on degradation thresholds."""
    if latency_val == "INF" or latency_val is None:
        return Text("∞ (DOWN)", style="bold red")
    try:
        val = float(latency_val)
        if val < 30.0:
            return Text(f"{val:.1f} ms", style="green")
        elif val < 60.0:
            return Text(f"{val:.1f} ms", style="yellow")
        else:
            return Text(f"{val:.1f} ms", style="bold red")
    except (ValueError, TypeError):
        return Text(str(latency_val), style="red")


def format_cpu(cpu_val: float, is_down: bool = False) -> Text:
    """Color codes CPU percentage."""
    if is_down:
        return Text("0.0%", style="dim")
    if cpu_val < 70.0:
        return Text(f"{cpu_val:.1f}%", style="green")
    elif cpu_val < 85.0:
        return Text(f"{cpu_val:.1f}%", style="yellow")
    else:
        return Text(f"{cpu_val:.1f}%", style="bold red")


def build_fleet_dashboard(snapshot: Dict[str, Any]) -> Panel:
    """Constructs the full Rich Dashboard renderable for terminal view."""
    # 1. Server Metrics Table
    table = Table(
        title="CDN Edge Servers Telemetry (Phase 1)",
        box=box.ROUNDED,
        header_style="bold cyan",
        show_lines=True,
        expand=True
    )

    table.add_column("Edge Server", style="bold white", width=14)
    table.add_column("Status", justify="center", width=14)
    table.add_column("Capacity", justify="right", width=12)
    table.add_column("Current Traffic", justify="right", width=14)
    table.add_column("Load Gauge", justify="center", width=18)
    table.add_column("Latency", justify="right", width=12)
    table.add_column("CPU", justify="right", width=10)
    table.add_column("Bandwidth", justify="right", width=14)
    table.add_column("Cost (₹/GB)", justify="right", width=12)

    for s in snapshot["servers"]:
        is_down = (s["status"] == "DOWN")
        table.add_row(
            f"📍 {s['name']}",
            format_status(s["status"]),
            f"{s['capacity']:,.0f} req/s",
            f"{s['current_requests']:,.0f} req/s" if not is_down else "[dim]0 req/s[/]",
            make_load_bar(s["current_requests"], s["capacity"]) if not is_down else "[red]OFFLINE[/]",
            format_latency(s["latency"]),
            format_cpu(s["cpu_utilization"], is_down),
            f"{s['bandwidth_usage']:,.1f} Mbps" if not is_down else "[dim]0.0 Mbps[/]",
            f"₹{s['bandwidth_cost']:.2f}"
        )

    # 2. Global Fleet Summary bar
    health_style = "bold green" if snapshot["fleet_health"] == "OPTIMAL" else (
        "bold yellow" if snapshot["fleet_health"] == "CONGESTED" else "bold white on red"
    )

    summary_text = Text()
    summary_text.append(f" Fleet Health: ", style="bold")
    summary_text.append(f" {snapshot['fleet_health']} ", style=health_style)
    summary_text.append("  |  ")
    summary_text.append(f"Active Nodes: {snapshot['active_count']}/{len(snapshot['servers'])}", style="cyan")
    summary_text.append("  |  ")
    summary_text.append(f"Total Traffic: {snapshot['total_traffic']:,.0f} req/s", style="magenta")
    summary_text.append("  |  ")
    summary_text.append(f"Total Bandwidth: {snapshot['total_bandwidth']:,.1f} Mbps", style="blue")
    summary_text.append("  |  ")
    avg_lat = snapshot['avg_latency']
    summary_text.append(f"Avg Latency: {avg_lat if isinstance(avg_lat, str) else f'{avg_lat:.1f}ms'}", style="yellow")
    summary_text.append("  |  ")
    redis_conn = snapshot.get("redis_connected", False)
    if redis_conn:
        summary_text.append("Redis: SYNCED (HSET)", style="bold green")
    else:
        summary_text.append("Redis: OFFLINE", style="dim yellow")

    summary_panel = Panel(summary_text, style="blue", box=box.HORIZONTALS)

    # 3. Recent Activity Log Table
    event_table = Table(
        title="Recent Simulator Events & Telemetry Transitions",
        box=box.SIMPLE_HEAD,
        header_style="bold magenta",
        expand=True
    )
    event_table.add_column("Time", style="dim", width=10)
    event_table.add_column("Source", style="bold cyan", width=12)
    event_table.add_column("Level", justify="center", width=10)
    event_table.add_column("Event Message", style="white")

    for ev in reversed(snapshot.get("recent_events", [])[-5:]):
        lvl = ev.get("level", "INFO")
        if lvl == "CRITICAL":
            lvl_text = Text("CRIT", style="bold red")
        elif lvl == "WARNING":
            lvl_text = Text("WARN", style="bold yellow")
        elif lvl == "SUCCESS":
            lvl_text = Text("OK", style="bold green")
        else:
            lvl_text = Text("INFO", style="blue")

        event_table.add_row(
            ev.get("timestamp", "--"),
            ev.get("source", "SIM"),
            lvl_text,
            ev.get("message", "")
        )

    # Combine into unified layout
    group = Group(
        summary_panel,
        table,
        event_table
    )

    return Panel(
        group,
        title="[bold yellow]⚡ CDN Edge Server Simulator — Phase 1[/]",
        subtitle="[dim]Press Ctrl+C to stop simulation | Phase 2 will introduce Min-Cost Max-Flow[/]",
        border_style="cyan"
    )
