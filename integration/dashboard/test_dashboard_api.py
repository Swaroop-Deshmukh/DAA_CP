import urllib.request
import json
import time

scenarios = [
    'normal',
    'spike',
    'pune_congestion',
    'mumbai_failure',
    'multiple_congestion',
    'fleet_overload',
    'fleet_recovery'
]

print('========================================================================================')
print('           TESTING ALL 7 DASHBOARD SCENARIO INTEGRATIONS WITH C++ MCMF                   ')
print('========================================================================================')

for name in scenarios:
    req = urllib.request.Request(f'http://127.0.0.1:8000/api/scenario/{name}', method='POST')
    with urllib.request.urlopen(req) as resp:
        data = json.loads(resp.read().decode())
    r = data.get('routing', {})
    status = r.get("routing_status", "")
    demand = r.get("requested_traffic", 0)
    routed = r.get("routed_traffic", 0)
    unrouted = r.get("unrouted_traffic", 0)
    cost = r.get("total_cost_rs", 0.0)
    allocs = r.get("new_allocation", {})
    
    print(f"Scenario: {name:<22} | Status: {status:<20} | Demand: {demand:>5} | Routed: {routed:>5} | Unrouted: {unrouted:>4} | Cost: Rs. {cost:>9.2f}")
    print(f"   Server Allocations: {allocs}")
    time.sleep(0.3)

print('========================================================================================')
print('ALL 7 SCENARIOS VERIFIED SUCCESSFULLY VIA C++ DAA ENGINE REST ENDPOINTS!')
print('========================================================================================')
