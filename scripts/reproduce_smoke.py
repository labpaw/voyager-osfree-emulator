#!/usr/bin/env python3
"""Fresh build, all smoke checks, and byte comparison against saved full evidence."""
import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path
from run_batch import digest

parser = argparse.ArgumentParser()
parser.add_argument('--cmake',default='cmake')
parser.add_argument('--out',type=Path,required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
if Path.cwd() != root: raise SystemExit('run from the repository root')
cmake = Path(shutil.which(args.cmake) or args.cmake).resolve()
out = args.out.resolve()
out.mkdir(parents=True,exist_ok=False)
build = out/'build with spaces'
commands = [
    [str(cmake),'-S',str(root),'-B',str(build),'-DCMAKE_BUILD_TYPE=Release','-DPython3_EXECUTABLE='+sys.executable],
    [str(cmake),'--build',str(build),'-j','2'],
    [str(cmake.with_name('ctest')),'--test-dir',str(build),'--output-on-failure'],
    [sys.executable,'scripts/run_batch.py','--profile','smoke','--exe',str(build/'emulator'),'--out',str(out/'evidence')],
    [sys.executable,'scripts/check_csv.py',str(out/'evidence')],
    [sys.executable,'scripts/analyze.py',str(out/'evidence'),'--out',str(out/'analysis')],
]
with (out/'commands.log').open('w') as log:
    for command in commands:
        print('Running:',command[0],command[1],flush=True)
        log.write(json.dumps(command)+'\n'); log.flush()
        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
reference = json.loads((root/'results/full/manifest.json').read_text())
reference_runs = {e['id']:e for e in reference['runs']}
fresh = json.loads((out/'evidence/manifest.json').read_text())
compared = 0
for entry in fresh['runs']:
    saved = reference_runs[entry['id']]
    assert entry['input_sha256'] == saved['input_sha256'] and entry['status'] == saved['status']
    assert set(entry['output_sha256']) == set(saved['output_sha256'])
    for name,expected in saved['output_sha256'].items():
        assert digest(out/'evidence'/entry['output']/name) == expected, (entry['id'],name)
        compared += 1
report = dict(schema_version=1,result='passed',runs=len(fresh['runs']),identical_raw_files=compared,
              reference_manifest_sha256=digest(root/'results/full/manifest.json'),
              fresh_build=fresh['build'],commands=commands)
(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('Fresh smoke reproduced:',len(fresh['runs']),'runs;',compared,'raw files match the saved full batch')
