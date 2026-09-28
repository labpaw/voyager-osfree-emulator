#!/usr/bin/env python3
import csv
import subprocess
import sys
import tempfile
from pathlib import Path

exe = Path(sys.argv[1]).resolve()
root = Path(sys.argv[2]).resolve()
sys.path.insert(0,str(root/'scripts'))
from check_csv import check_schedule, check_memory
from workloads import TASK_HEADER, write_csv


def invoke(command, code):
    p = subprocess.run([str(exe),*map(str,command)],capture_output=True,text=True)
    assert p.returncode == code, (command,p.returncode,p.stdout,p.stderr)


with tempfile.TemporaryDirectory(prefix='irp tests ') as tmp:
    tmp = Path(tmp)
    def schedule(path,out,code=0,extra=()):
        invoke(['schedule','--tasks',path,'--horizon','10','--budget','4096','--out',out,*extra],code)
    good = tmp/'tasks with spaces.csv'
    write_csv(good,TASK_HEADER,[[1,5,5,1,0,8]])
    schedule(good,tmp/'valid output')
    check_schedule(tmp/'valid output')
    schedule(good,tmp/'valid output',2)  # No overwriting raw runs.
    schedule(good,tmp/'unknown',2,['--scheduler','preemptive_rm'])
    schedule(good,tmp/'negative',2,['--jobs','-1'])
    for index,bad in enumerate((['1','0','5','1','0','8'],
                                ['1','5','5','0','0','8'],
                                ['1','5','5','1','5','8'])):
        path = tmp/('bad%d.csv'%index)
        write_csv(path,TASK_HEADER,[bad])
        schedule(path,tmp/('badout%d'%index),4)
    for index,field in enumerate(('-1','18446744073709551616','1junk','')):
        path = tmp/('parse%d.csv'%index)
        write_csv(path,TASK_HEADER,[[1,field,5,1,0,8]])
        schedule(path,tmp/('parseout%d'%index),2)
    duplicate = tmp/'duplicates.csv'
    write_csv(duplicate,TASK_HEADER,[[1,5,5,1,0,8]]*2)
    schedule(duplicate,tmp/'duplicateout',4)
    for budget,code in ((512,0),(1024,0)):
        folder = tmp/('mem%d'%budget)
        invoke(['memory','--trace',root/'configs/traces/fragmentation_fixture.csv',
                '--policy','bounded_first_fit','--budget',budget,'--out',folder],code)
        check_memory(folder)
    invoke(['memory','--trace',root/'configs/traces/constant.csv','--policy',
            'static_reservation','--budget','512','--out',tmp/'rejected'],3)
    check_memory(tmp/'rejected')
    # Malformed trace lifetimes are rejected before replay/output.
    trace = tmp/'badtrace.csv'
    trace.write_text('event_index,time,owner,request_id,operation,bytes,owner_maximum\n0,0,0,1,free,0,16\n')
    invoke(['memory','--trace',trace,'--policy','bounded_first_fit','--budget','16','--out',tmp/'badtrace'],2)
    assert not (tmp/'badtrace').exists()
print('CLI input, output and memory checks passed')
