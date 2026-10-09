#!/usr/bin/env python3
"""Descriptive summaries only: seeds are distinct inputs, reruns aren't samples."""
import argparse
import csv
import json
import statistics
from collections import Counter, defaultdict
from pathlib import Path
from check_csv import rows, check_schedule, check_memory
from run_batch import digest
from workloads import write_csv

NORMAL = {'complete','memory_rejected'}


def statistics_columns(records, fields):
    values = []
    for field in fields:
        available = [float(r[field]) for r in records if r.get(field,'NA') != 'NA']
        values.extend([statistics.median(available),min(available),max(available)] if available else ['NA']*3)
    return values


def count_outcomes(entries):
    counts = Counter(e['status'] for e in entries)
    return [counts['complete'], counts['memory_rejected'], sum(v for k,v in counts.items() if k not in NORMAL)]


def collect(batch):
    manifest = json.loads((batch/'manifest.json').read_text())
    assert len({e['id'] for e in manifest['runs']}) == len(manifest['runs'])
    collected = []
    paired = defaultdict(set)
    for entry in manifest['runs']:
        entry = dict(entry)
        entry['record'] = None
        if entry['status'] not in NORMAL:
            collected.append(entry)
            continue
        folder = batch/entry['output']
        for name,expected in entry['output_sha256'].items(): assert digest(folder/name) == expected, (entry['id'],name)
        status = check_memory(folder) if entry['kind'] == 'memory' else check_schedule(folder)
        assert status == entry['status']
        filename = 'memory_runs.csv' if entry['kind'] == 'memory' else 'runs.csv'
        record, = rows(folder/filename)
        assert record['run_id'] == entry['id']
        if entry['kind'] == 'schedule': entry['task_count'] = len(rows(folder/'tasks.csv'))
        else:
            assert digest(folder/'memory_trace.csv') == entry['input_sha256']
            paired[entry['input']].add(entry['input_sha256'])
        entry['record'] = record
        collected.append(entry)
    assert all(len(hashes) == 1 for hashes in paired.values())
    return manifest,collected


def flat(entries,path,kind):
    selected = [e for e in entries if e['kind'] == kind and e['record'] is not None]
    if not selected: return
    extra = ['group','family','seed','nominal','task_count','phase','contiguous_cost','input_sha256']
    fields = list(selected[0]['record'])
    write_csv(path,extra+fields,[[e.get(k,'NA') for k in extra]+[e['record'].get(k,'NA') for k in fields] for e in selected])


def analyze(batch,out):
    manifest, entries = collect(batch)
    out.mkdir(parents=True,exist_ok=False)
    statuses = Counter((e['group'],e['kind'],e['status']) for e in entries)
    write_csv(out/'status_counts.csv',['group','kind','status','runs'],[list(k)+[v] for k,v in sorted(statuses.items())])
    flat(entries,out/'schedule_runs.csv','schedule')
    flat(entries,out/'memory_runs.csv','memory')
    schedule = defaultdict(list)
    memory = defaultdict(list)
    for e in entries:
        if e['kind'] == 'schedule':
            key = (e['group'],e.get('task_count','NA'),e.get('nominal','NA'),e.get('phase','NA'),e.get('contiguous_cost','NA'),e['budget'])
            schedule[key].append(e)
        else:
            key = (e['family'],e['policy'],e['budget'])
            memory[key].append(e)
    fields = ['actual_utilisation','miss_rate','max_response','cpu_utilisation','backlog']
    header = ['group','task_count','nominal_utilisation','phase','contiguous_cost','budget','runs','distinct_inputs','completed','rejected','incomplete']
    header += [f+'_'+suffix for f in fields for suffix in ('median','min','max')]
    summary = []
    for key,group in sorted(schedule.items(),key=lambda kv:str(kv[0])):
        complete = [e['record'] for e in group if e['status'] == 'complete']
        summary.append(list(key)+[len(group),len({e['input_sha256'] for e in group})]+count_outcomes(group)+statistics_columns(complete,fields))
    write_csv(out/'summary.csv',header,summary)
    fields = ['request_failure_rate','peak_demand','peak_live','peak_occupied','peak_unused_reserved','fragmentation_failures']
    header = ['family','policy','budget','runs','distinct_inputs','completed','rejected','incomplete','admission_fraction']
    header += [f+'_'+suffix for f in fields for suffix in ('median','min','max')]
    summary = []
    for key,group in sorted(memory.items()):
        completed,rejected,incomplete = count_outcomes(group)
        data = []
        for e in group:
            if e['status'] != 'complete': continue
            r = dict(e['record'])
            r['request_failure_rate'] = int(r['failures'])/int(r['requests']) if int(r['requests']) else 'NA'
            data.append(r)
        summary.append(list(key)+[len(group),len({e['input_sha256'] for e in group}),completed,rejected,incomplete,
                                 completed/len(group) if not incomplete else 'NA']+statistics_columns(data,fields))
    write_csv(out/'memory_summary.csv',header,summary)
    high_priority = []
    for e in entries:
        if e['group'] != 'e2' or e['status'] != 'complete': continue
        task, = [r for r in rows(batch/e['output']/'task_metrics.csv') if r['task_id'] == '1']
        high_priority.append([e['id'],e['contiguous_cost'],e['phase'],e['record']['actual_utilisation'],task['max_response'],task['max_waiting'],task['missed'],task['released']])
    write_csv(out/'blocking_summary.csv',['run_id','contiguous_cost','phase','actual_utilisation','high_max_response','high_max_waiting','high_missed','high_released'],high_priority)
    provenance = dict(schema_version=1,batch=str(batch),manifest_sha256=digest(batch/'manifest.json'),
                      analysis_sha256=digest(__file__),planned=len(entries),
                      completed=sum(e['status']=='complete' for e in entries),
                      rejected=sum(e['status']=='memory_rejected' for e in entries),
                      incomplete=sum(e['status'] not in NORMAL for e in entries),
                      input_revision=manifest['source_revision'])
    (out/'analysis.json').write_text(json.dumps(provenance,indent=2)+'\n')
    print('Analysis checked and saved:',provenance)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('batch',type=Path)
    parser.add_argument('--out',required=True,type=Path)
    args = parser.parse_args()
    analyze(args.batch,args.out)


if __name__ == '__main__': main()
