#!/usr/bin/env python3
"""Version 1: xorshift32, paired periods/weights, integer nearest rounding."""
import csv
from pathlib import Path

PERIODS = (50, 100, 200, 400, 800)
LOADS = (20, 40, 60, 70, 80, 90, 100, 110)
TASK_HEADER = ('task_id','period','deadline','cost','offset','reserved_bytes')


class RNG:
    def __init__(self, seed):
        self.state = seed & 0xffffffff or 1

    def next(self):
        x = self.state
        x ^= (x << 13) & 0xffffffff
        x ^= x >> 17
        x ^= (x << 5) & 0xffffffff
        self.state = x & 0xffffffff
        return self.state


def family(n, seed):
    rng = RNG(seed*1009 + n*9176)
    # Bounded retries choose ONE family that meets tolerance at every load.
    for attempt in range(1000):
        periods = [PERIODS[rng.next() % len(PERIODS)] for _ in range(n)]
        weights = [1 + rng.next() % 1000 for _ in range(n)]
        total = sum(weights)
        sets = {}
        for load in LOADS:
            tasks = []
            for i, (period, weight) in enumerate(zip(periods, weights)):
                numerator, denominator = load*weight*period, 100*total
                cost = max(1, (2*numerator+denominator)//(2*denominator))
                tasks.append([i+1, period, period, cost, 0, 256])
            actual = sum(t[3]/t[1] for t in tasks)
            if abs(actual-load/100) > 0.02 + 1e-12:
                break
            sets[load] = tasks
        if len(sets) == len(LOADS):
            return sets, attempt
    raise ValueError('workload family unavailable: n=%s seed=%s' % (n, seed))


def write_csv(path, header, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('w', newline='') as f:
        writer = csv.writer(f, lineterminator='\n')
        writer.writerow(header)
        writer.writerows(data)


def write_tasks(path, tasks):
    write_csv(path, TASK_HEADER, tasks)
