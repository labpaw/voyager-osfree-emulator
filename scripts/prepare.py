#!/usr/bin/env python3
import json
from pathlib import Path
from workloads import family, write_tasks, write_csv
from memory_traces import traces


def e1():
    plan, tasksets = [], []
    for n in (3, 5, 10):
        for seed in range(1, 11):
            sets, attempt = family(n, seed)
            for load, tasks in sets.items():
                name = 'e1_n%d_s%d_u%d' % (n, seed, load)
                path = 'configs/generated/%s.csv' % name
                write_tasks(path, tasks)
                actual = sum(t[3]/t[1] for t in tasks)
                priorities = {t[0]: rank for rank, t in enumerate(sorted(tasks, key=lambda t: (t[1],t[0])))}
                for t in tasks:
                    tasksets.append([name, seed, n, attempt, load/100, actual, t[0], priorities[t[0]], *t[1:]])
                plan.append(dict(id=name, group='e1', kind='schedule', input=path, horizon=100000,
                                 budget=4096, seed=seed, nominal=load/100,
                                 smoke=(seed == 1 and load in (20,80,110))))
    write_csv('configs/tasksets.csv', ['taskset_id','seed','task_count','family_attempt','nominal_utilisation','actual_utilisation','task_id','priority','period','deadline','cost','offset','reserved_bytes'], tasksets)
    return plan


def blocking_and_sensitivity():
    plan = []
    for cost, period in ((2,40),(4,80),(8,160),(12,240)):
        for offset in (0,1):
            name = 'e2_c%d_o%d' % (cost, offset)
            path = 'configs/generated/%s.csv' % name
            write_tasks(path, [[1,5,5,1,offset,256],[2,period,period,cost,0,256]])
            plan.append(dict(id=name,group='e2',kind='schedule',input=path,horizon=10000,
                             budget=4096,contiguous_cost=cost,phase=offset,smoke=True))
    sets, _ = family(5,1)
    for load in (20,80,110):
        for staggered in (False,True):
            name = 'sensitivity_u%d_%s' % (load, 'staggered' if staggered else 'zero')
            tasks = [list(t) for t in sets[load]]
            if staggered:
                for i,t in enumerate(tasks):
                    t[4] = i*t[1]//len(tasks)
            path = 'configs/generated/%s.csv' % name
            write_tasks(path,tasks)
            plan.append(dict(id=name,group='sensitivity',kind='schedule',input=path,
                             horizon=200000,budget=4096,nominal=load/100,seed=1,smoke=True))
    for fixture in ('single','blocking','ties'):
        plan.append(dict(id='e0_'+fixture,group='e0',kind='schedule',input='configs/fixtures/'+fixture+'.csv',
                         horizon=12 if fixture == 'single' else 10 if fixture == 'blocking' else 1,
                         budget=4096,smoke=True))
    return plan


def memory_plan():
    plan = []
    for family_name, seed, path in traces():
        for budget in (512,1024,1536,2048,2560,4096):
            for policy in ('static_reservation','bounded_first_fit'):
                name = 'e3_%s_s%d_b%d_%s' % (family_name,seed,budget,policy)
                plan.append(dict(id=name,group='e3',kind='memory',input=path,policy=policy,
                                 budget=budget,seed=seed,family=family_name,
                                 smoke=(seed in (0,1) and budget in (512,1024,2560,4096))))
    return plan


def main():
    plan = e1() + blocking_and_sensitivity() + memory_plan()
    Path('configs/experiments.json').write_text(json.dumps({'schema_version':1, 'generator':'xorshift32-v1', 'runs':plan}, indent=2)+'\n')
    print('Prepared', len(plan), 'experiments')


if __name__ == '__main__':
    main()
