#!/usr/bin/env python3
import json
from pathlib import Path
from workloads import family, write_tasks, write_csv


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


def main():
    plan = e1()
    Path('configs/experiments.json').write_text(json.dumps({'schema_version':1, 'generator':'xorshift32-v1', 'runs':plan}, indent=2)+'\n')
    print('Prepared', len(plan), 'experiments')


if __name__ == '__main__':
    main()
