#!/usr/bin/env python3
import json
import sys
import tempfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from analyze import analyze, count_outcomes
from check_csv import rows

assert count_outcomes([{'status':'complete'},{'status':'memory_rejected'},{'status':'job_capacity'},{'status':'pending'}]) == [1,1,2]
with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    (tmp/'manifest.json').write_text(json.dumps({'source_revision':'fixture','runs':[
        dict(id='unfinished',group='e1',kind='schedule',budget=512,status='job_capacity',input_sha256='a',nominal=1.1),
        dict(id='pending',group='e3',kind='memory',family='constant',policy='bounded_first_fit',budget=512,status='pending',input_sha256='b')]}))
    analyze(tmp,tmp/'derived')
    schedule, = rows(tmp/'derived/summary.csv')
    memory, = rows(tmp/'derived/memory_summary.csv')
    assert schedule['completed'] == '0' and schedule['incomplete'] == '1' and schedule['miss_rate_median'] == 'NA'
    assert memory['completed'] == '0' and memory['incomplete'] == '1' and memory['admission_fraction'] == 'NA'
print('Incomplete and pending runs remain excluded and counted')
