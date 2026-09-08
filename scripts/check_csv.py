#!/usr/bin/env python3
"""Check saved outcomes independently of scheduler counters."""
import argparse
import csv
from pathlib import Path


def rows(path):
    with Path(path).open(newline='') as f:
        return list(csv.DictReader(f))


def check_schedule(folder):
    run, = rows(folder / 'runs.csv')
    tasks = {r['task_id']: r for r in rows(folder / 'tasks.csv')}
    metrics = {r['task_id']: r for r in rows(folder / 'task_metrics.csv')}
    jobs = rows(folder / 'jobs.csv')
    if run['status'] == 'memory_rejected':
        assert int(run['released']) == 0 and not jobs
        assert run['miss_rate'] == 'NA' and run['cpu_utilisation'] == 'NA'
        return run['status']
    assert run['status'] == 'complete', (folder, run['status'])
    h = int(run['horizon'])
    assert int(run['busy']) + int(run['idle']) == h
    assert int(run['released']) == int(run['completed']) == int(run['dispatches'])
    assert int(run['on_time']) + int(run['missed']) == int(run['released'])
    for field in ('released', 'completed', 'missed'):
        assert sum(int(t[field]) for t in metrics.values()) == int(run[field])
    expected_count = expected_work = 0
    for task in tasks.values():
        offset, period = int(task['offset']), int(task['period'])
        count = 0 if offset >= h else 1 + (h-1-offset)//period
        expected_count += count
        expected_work += count * int(task['cost'])
        assert int(metrics[task['task_id']]['released']) == count
    assert expected_count == int(run['released'])
    assert expected_work == int(run['busy']) + int(run['drain_busy'])
    if run['detailed_jobs'] == '0':
        assert not jobs
        return run['status']
    assert len(jobs) == expected_count
    previous_end = 0
    seen = set()
    responses, waits, misses = {}, {}, {}
    for j in jobs:
        tid = j['task_id']; task = tasks[tid]
        seq = int(j['sequence']); key = (tid, seq)
        assert key not in seen; seen.add(key)
        release, start, end, deadline = (int(j[k]) for k in ('release','first_start','completion','deadline'))
        assert release == int(task['offset']) + seq*int(task['period']) < h
        assert deadline == release + int(task['deadline'])
        assert start >= release and start >= previous_end
        assert end-start == int(task['cost'])
        previous_end = end
        assert int(j['response']) == end-release
        assert int(j['waiting']) == start-release
        assert int(j['tardiness']) == max(0, end-deadline)
        assert int(j['missed']) == (end > deadline)
        responses.setdefault(tid, []).append(end-release)
        waits.setdefault(tid, []).append(start-release)
        misses[tid] = misses.get(tid, 0) + (end > deadline)
    for tid, tm in metrics.items():
        r, w = responses.get(tid, []), waits.get(tid, [])
        assert int(tm['response_sum']) == sum(r) and int(tm['waiting_sum']) == sum(w)
        assert int(tm['missed']) == misses.get(tid, 0)
        assert tm['max_response'] == (str(max(r)) if r else 'NA')
    assert run['max_response'] == (str(max(int(j['response']) for j in jobs)) if jobs else 'NA')
    assert int(run['backlog']) == sum(int(j['completion']) > h for j in jobs)
    assert int(run['drain_ticks']) == max(0, previous_end-h)
    return run['status']


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    files = sorted(args.directory.rglob('runs.csv'))
    assert files, 'no runs found'
    counts = {}
    for path in files:
        status = check_schedule(path.parent)
        counts[status] = counts.get(status, 0)+1
    print('Checked schedule CSVs:', counts)


if __name__ == '__main__':
    main()
