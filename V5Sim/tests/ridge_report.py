#!/usr/bin/env python3
"""Summarise a run_sim.py --harness log: per-thread loop timing, memory, errors, robot state.

  python3 V5Sim/run_sim.py --script Custom_python/py/main_ridge.py --harness \
      --robot V5Sim/tests/ridge_driver.json > run.log
  python3 V5Sim/tests/ridge_report.py run.log
"""
import re
import sys
from collections import defaultdict


def main(path):
    threads = defaultdict(lambda: {"secs": 0, "waits": 0, "work_avg": [], "work_max": 0, "late_avg": [], "late_max": 0, "late2ms": 0, "err": 0, "count": set()})
    mem = []
    errors = []
    state = []
    for line in open(path, errors="replace"):
        line = line.rstrip()
        if line.startswith("[H]"):
            kv = dict(re.findall(r"(\w+)=(-?\d+)", line))
            name = line.split()[2]
            if int(kv["waits"]) == 0:
                continue
            t = threads[name]
            t["secs"] += 5
            t["waits"] += int(kv["waits"])
            t["work_avg"].append(int(kv["work_avg"]))
            t["work_max"] = max(t["work_max"], int(kv["work_max"]))
            t["late_avg"].append(int(kv["late_avg"]))
            t["late_max"] = max(t["late_max"], int(kv["late_max"]))
            t["late2ms"] += int(kv["late2ms"])
            t["err"] = max(t["err"], int(kv["err"]))
        elif line.startswith("[S]"):
            kv = dict(re.findall(r"(\w+)=(-?\d+)", line))
            mem.append((int(kv["t"]), int(kv["free"]), int(kv["threads"])))
        elif line.startswith("[R]"):
            state.append(line[4:])
        elif line.startswith("[E]") or "Traceback" in line or "Error" in line:
            errors.append(line)
    print("thread | active s | wakes/s | work avg us | work max us | late avg us | late max us | waits >2ms late")
    for name, t in sorted(threads.items()):
        n = t["secs"]
        print("%s | %d | %.0f | %d | %d | %d | %d | %d" % (
            name, n, t["waits"] / max(n, 1), sum(t["work_avg"]) // max(n, 1), t["work_max"],
            sum(t["late_avg"]) // max(n, 1), t["late_max"], t["late2ms"]))
    if mem:
        print("memory free: start %d, min %d, end %d bytes; threads %d" % (mem[0][1], min(m[1] for m in mem), mem[-1][1], mem[-1][2]))
    print("errors:", len(errors))
    for e in errors[:20]:
        print("  ", e)
    print("state per second:")
    for s in state:
        print("  ", s)


if __name__ == "__main__":
    main(sys.argv[1])
