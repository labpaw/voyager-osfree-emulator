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


def check_memory(folder):
    run, = rows(folder / 'memory_runs.csv')
    events = rows(folder / 'memory_events.csv')
    trace = rows(folder / 'memory_trace.csv')
    if run['status'] == 'memory_rejected':
        assert run['policy'] == 'static_reservation' and not events
        assert int(run['requests']) == 0 and run['peak_live'] == 'NA'
        maxima = {t['owner']: int(t['owner_maximum']) for t in trace}
        assert sum((m+7)//8*8 for m in maxima.values()) > int(run['budget'])
        return run['status']
    assert run['status'] == 'complete'
    assert len(events) == len(trace) == int(run['events'])
    static = run['policy'] == 'static_reservation'
    budget = int(run['budget'])
    maxima = {t['owner']: int(t['owner_maximum']) for t in trace}
    reserved = sum((m+7)//8*8 for m in maxima.values())
    payload = sum(maxima.values())
    pending, accepted = {}, {}
    counts = dict(requests=0,successes=0,failures=0,skipped_frees=0,
                  fragmentation_failures=0,capacity_failures=0,descriptor_failures=0)
    peaks = dict(demand=0,live=0,occupied=reserved if static else 0,
                 unused_reserved=payload if static else 0,padding=reserved-payload if static else 0)
    for t,e in zip(trace,events):
        for key in ('event_index','time','owner','request_id','operation','bytes'):
            assert t[key] == e[key]
        owner, request = t['owner'],t['request_id']
        reason = e['outcome']
        if t['operation'] == 'allocate':
            assert owner not in pending
            pending[owner] = (request,int(t['bytes']))
            counts['requests'] += 1
            if reason == 'ok':
                accepted[owner] = pending[owner]
                counts['successes'] += 1
            else:
                assert not static and reason in ('capacity','fragmentation','descriptors')
                counts['failures'] += 1
                counts[dict(capacity='capacity_failures',fragmentation='fragmentation_failures',descriptors='descriptor_failures')[reason]] += 1
                need = (int(t['bytes'])+7)//8*8
                if reason == 'capacity': assert int(e['free']) < need
                if reason == 'fragmentation': assert int(e['largest_free']) < need <= int(e['free'])
        else:
            assert pending[owner][0] == request
            del pending[owner]
            if owner in accepted:
                assert reason == 'ok'; del accepted[owner]
            else:
                assert reason == 'skipped_free'; counts['skipped_frees'] += 1
        demand = sum(r[1] for r in pending.values())
        live = sum(r[1] for r in accepted.values())
        assert int(e['demand']) == demand and int(e['live']) == live
        occupied = reserved if static else sum((r[1]+7)//8*8 for r in accepted.values())
        assert int(e['occupied']) == occupied <= budget
        assert int(e['free']) == budget-occupied
        assert int(e['padding']) == (reserved-payload if static else occupied-live)
        if static:
            assert int(e['unused_reserved']) == payload-live
            assert e['external_fragmentation'] == e['largest_free'] == 'NA'
        else:
            assert 0 <= int(e['largest_free']) <= int(e['free'])
            if int(e['free']):
                expected = 1-int(e['largest_free'])/int(e['free'])
                assert abs(float(e['external_fragmentation'])-expected) < 1e-12
            else: assert e['external_fragmentation'] == 'NA'
        for key in peaks:
            if e[key] != 'NA': peaks[key] = max(peaks[key],int(e[key]))
    assert not pending and not accepted
    for key,value in counts.items(): assert int(run[key]) == value, (folder,key)
    for key,value in peaks.items():
        if static or key != 'unused_reserved': assert int(run['peak_'+key]) == value, (folder,key)
    assert counts['requests'] == counts['successes']+counts['failures']
    assert counts['skipped_frees'] == counts['failures']
    if not static:
        assert int(events[-1]['free']) == int(events[-1]['largest_free']) == budget
    return run['status']


def find_run_files(directory):
    files = sorted(directory.rglob('runs.csv'))
    memory_files = sorted(path for path in directory.rglob('memory_runs.csv')
                          if not (path.parent / 'analysis.json').is_file())
    return files, memory_files


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    files, memory_files = find_run_files(args.directory)
    assert files or memory_files, 'no runs found'
    counts = {}
    for path in files:
        status = check_schedule(path.parent)
        counts[status] = counts.get(status, 0)+1
    print('Checked schedule CSVs:', counts)
    memory_counts = {}
    for path in memory_files:
        status = check_memory(path.parent)
        memory_counts[status] = memory_counts.get(status,0)+1
    print('Checked memory CSVs:',memory_counts)


if __name__ == '__main__':
    main()
