#!/usr/bin/env python3
import csv
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

exe, root = map(lambda p: Path(p).resolve(),sys.argv[1:])
sys.path.insert(0,str(root/'scripts'))
from workloads import RNG, family
from check_csv import rows, check_schedule, check_memory

assert RNG(1).next() == 270369
for n in (3,5,10):
    for seed in range(1,11):
        first, attempt = family(n,seed)
        assert (first,attempt) == family(n,seed)
        assert len({tuple(t[1] for t in tasks) for tasks in first.values()}) == 1
        for load,tasks in first.items():
            assert abs(sum(t[3]/t[1] for t in tasks)-load/100) <= 0.02+1e-12

with tempfile.TemporaryDirectory(prefix='irp replay ') as tmp:
    tmp = Path(tmp)
    for name,detailed in (('a','1'),('b','1'),('quiet','0')):
        subprocess.run([str(exe),'schedule','--tasks',str(root/'configs/fixtures/blocking.csv'),
                        '--horizon','100','--budget','16','--out',str(tmp/name),
                        '--run-id','paired','--jobs',detailed],check=True,capture_output=True)
        check_schedule(tmp/name)
    for path in (tmp/'a').iterdir():
        assert path.read_bytes() == (tmp/'b'/path.name).read_bytes()
    a, = rows(tmp/'a/runs.csv'); b, = rows(tmp/'quiet/runs.csv')
    a.pop('detailed_jobs'); b.pop('detailed_jobs')
    assert a == b
    for name in ('tasks.csv','task_metrics.csv'):
        assert (tmp/'a'/name).read_bytes() == (tmp/'quiet'/name).read_bytes()
    hashes = set()
    for i,policy in enumerate(('bounded_first_fit','bounded_first_fit','static_reservation')):
        out = tmp/('m%d'%i)
        subprocess.run([str(exe),'memory','--trace',str(root/'configs/traces/fragmentation_fixture.csv'),
                        '--policy',policy,'--budget','1024','--out',str(out),'--run-id','paired'],check=True,capture_output=True)
        check_memory(out)
        hashes.add(hashlib.sha256((out/'memory_trace.csv').read_bytes()).hexdigest())
    assert len(hashes) == 1
    for path in (tmp/'m0').iterdir(): assert path.read_bytes() == (tmp/'m1'/path.name).read_bytes()
    subprocess.run([str(exe),'schedule','--tasks',str(root/'configs/fixtures/ties.csv'),
                    '--horizon','1','--budget','16','--out',str(tmp/'ties')],check=True,capture_output=True)
    assert [r['task_id'] for r in rows(tmp/'ties/jobs.csv')] == ['2','8']
print('Deterministic inputs, output toggles, replay and priority ties passed')
