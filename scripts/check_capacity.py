#!/usr/bin/env python3
import argparse
import json
import subprocess
import tempfile
from pathlib import Path
from run_batch import digest, run_one
from check_csv import rows
from workloads import write_csv

parser = argparse.ArgumentParser()
parser.add_argument('--exe',type=Path,default=Path('build/emulator'))
args = parser.parse_args()
plan = json.loads(Path('configs/experiments.json').read_text())
selected = [r for r in plan['runs'] if (r['group'] == 'e1' and r['nominal'] == 1.1) or r['group'] == 'sensitivity']
build = json.loads(subprocess.check_output([str(args.exe.resolve()),'info'],text=True))
report = []
with tempfile.TemporaryDirectory(prefix='irp-capacity-') as tmp:
    for item in selected:
        folder = Path(tmp)/item['id']
        _, status = run_one(args.exe.resolve(),item,folder)
        r, = rows(folder/'runs.csv')
        assert status == 'complete' and int(r['peak_jobs']) <= build['jobs']
        report.append([item['id'],item['horizon'],r['peak_jobs'],r['backlog'],r['released'],r['completed'],status])
write_csv('docs/verification/capacity.csv',['run_id','horizon','peak_jobs','backlog','released','completed','status'],report)
frozen = dict(schema_version=1,plan_sha256=digest('configs/experiments.json'),
              tasksets_sha256=digest('configs/tasksets.csv'),build=build,
              inputs={r['input']:digest(r['input']) for r in plan['runs']})
Path('configs/frozen.json').write_text(json.dumps(frozen,indent=2,sort_keys=True)+'\n')
print('Checked',len(report),'runs; maximum simultaneous jobs:',max(int(r[2]) for r in report),'of',build['jobs'])
