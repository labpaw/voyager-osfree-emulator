#!/usr/bin/env python3
from workloads import RNG, write_csv

HEADER = ('event_index','time','owner','request_id','operation','bytes','owner_maximum')


def save_trace(path, operations, maxima):
    operations.sort(key=lambda e: e[0])  # Stable insertion order resolves equal times.
    rows = [[i,time,owner,request,op,bytes_,maxima[owner]]
            for i,(time,owner,request,op,bytes_) in enumerate(operations)]
    write_csv(path,HEADER,rows)


def traces():
    definitions = []
    operations = []
    for cycle in range(100):
        for owner in range(5):
            request = cycle*5+owner
            operations.extend([(cycle*10,owner,request,'allocate',256),
                               (cycle*10+5,owner,request,'free',0)])
    path = 'configs/traces/constant.csv'
    save_trace(path,operations,[256]*5)
    definitions.append(('constant',0,path))
    for seed in range(1,11):
        rng = RNG(seed*104729)
        for family in ('staggered','fragmentation'):
            operations = []
            for cycle in range(100):
                if family == 'staggered':
                    for owner in range(5):
                        time = cycle*40 + owner*7
                        request = cycle*5+owner
                        size = (1+rng.next()%8)*64
                        operations.extend([(time,owner,request,'allocate',size),
                                           (time+5,owner,request,'free',0)])
                else:
                    base = cycle*20
                    for owner in range(4):
                        operations.append((base+owner,owner,cycle*5+owner,'allocate',(1+rng.next()%4)*64))
                    for dt,owner in ((4,0),(5,2)):
                        operations.append((base+dt,owner,cycle*5+owner,'free',0))
                    operations.append((base+6,4,cycle*5+4,'allocate',(4+rng.next()%5)*64))
                    for dt,owner in ((7,1),(8,3),(9,4)):
                        operations.append((base+dt,owner,cycle*5+owner,'free',0))
            path = 'configs/traces/%s_s%d.csv' % (family,seed)
            save_trace(path,operations,[512]*5)
            definitions.append((family,seed,path))
    # Four 128-byte blocks fill 512. Free alternating blocks, then request 192.
    operations = [(i,i,i,'allocate',128) for i in range(4)]
    operations += [(4,0,0,'free',0),(5,2,2,'free',0),(6,4,4,'allocate',192),
                   (7,1,1,'free',0),(8,3,3,'free',0),(9,4,4,'free',0)]
    path = 'configs/traces/fragmentation_fixture.csv'
    save_trace(path,operations,[128,128,128,128,192])
    definitions.append(('fragmentation_fixture',0,path))
    return definitions
