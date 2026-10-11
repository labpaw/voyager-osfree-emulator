#!/usr/bin/env python3
import argparse
import json
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from check_csv import rows
from run_batch import digest

COLORS = {'static_reservation':'#406394','bounded_first_fit':'#b75c31'}


def save(fig,path):
    fig.savefig(path,dpi=160,bbox_inches='tight')
    plt.close(fig)


def plot(analysis,out):
    provenance = json.loads((analysis/'analysis.json').read_text())
    batch = provenance['batch']
    out.mkdir(parents=True,exist_ok=False)
    raw = rows(analysis/'schedule_runs.csv')
    summary = rows(analysis/'summary.csv')
    for field,title,ylabel in (('miss_rate','Deadline misses','Missed / admitted jobs'),
                               ('max_response','Observed maximum response','Response (ticks, log scale)')):
        fig,axes = plt.subplots(1,3,figsize=(13,4),sharey=True)
        for ax,n in zip(axes,(3,5,10)):
            data = [r for r in raw if r['group']=='e1' and int(r['task_count'])==n and r['status']=='complete']
            ax.scatter([float(r['actual_utilisation']) for r in data],[float(r[field]) for r in data],s=12,alpha=.35,color='#406394',label='Distinct task sets')
            agg = sorted([r for r in summary if r['group']=='e1' and int(r['task_count'])==n],key=lambda r:float(r['nominal_utilisation']))
            x = [float(r['actual_utilisation_median']) for r in agg]
            y = [float(r[field+'_median']) for r in agg]
            low = [float(r[field+'_min']) for r in agg]; high = [float(r[field+'_max']) for r in agg]
            ax.plot(x,y,'o-',color='#243b53',markersize=4,label='Median of 10 seeds/load')
            ax.vlines(x,low,high,color='#243b53',alpha=.5,label='Input min–max')
            bound = n*(2**(1/n)-1)
            ax.axvline(bound,ls='--',color='#b75c31',label='Liu–Layland reference')
            ax.set(title='%d tasks'%n,xlabel='Achieved Σ C/T')
            ax.grid(alpha=.2)
            if field=='max_response': ax.set_yscale('log')
        axes[0].set_ylabel(ylabel)
        axes[-1].legend(fontsize=7,loc='upper left')
        fig.suptitle('%s — %s\nPreemptive RM reference is not a cooperative pass/fail threshold'%(title,batch),fontsize=11)
        fig.tight_layout()
        save(fig,out/(field+'.png'))
    fig,axes = plt.subplots(1,2,figsize=(10,4))
    blocking = rows(analysis/'blocking_summary.csv')
    for phase,label in (('0','All offsets zero'),('1','Short task offset = 1')):
        data = sorted([r for r in blocking if r['phase']==phase],key=lambda r:int(r['contiguous_cost']))
        x = [int(r['contiguous_cost']) for r in data]
        axes[0].plot(x,[int(r['high_max_response']) for r in data],'o-',label=label)
        axes[1].plot(x,[int(r['high_missed'])/int(r['high_released']) for r in data],'o-',label=label)
    axes[0].set_ylabel('Short task observed max response (ticks)')
    axes[1].set_ylabel('Short task deadline miss fraction')
    for ax in axes:
        ax.set_xlabel('Low-priority contiguous execution (ticks)')
        ax.grid(alpha=.2); ax.legend(fontsize=8)
    fig.suptitle('Controlled blocking — %s\nTotal U = 0.25; long-task period also changes (40, 80, 160, 240)'%batch,fontsize=11)
    fig.tight_layout(); save(fig,out/'blocking.png')
    memory = rows(analysis/'memory_summary.csv')
    families = ('constant','staggered','fragmentation','fragmentation_fixture')
    fig,axes = plt.subplots(4,3,figsize=(13,13))
    for row,family in enumerate(families):
        for policy,color in COLORS.items():
            data = sorted([r for r in memory if r['family']==family and r['policy']==policy],key=lambda r:int(r['budget']))
            x = [int(r['budget']) for r in data]
            axes[row,0].plot(x,[float(r['admission_fraction']) for r in data],'o-',color=color,label=policy)
            for col,field in ((1,'request_failure_rate'),(2,'peak_occupied')):
                accepted = [r for r in data if r[field+'_median']!='NA']
                axx = [int(r['budget']) for r in accepted]
                med = [float(r[field+'_median']) for r in accepted]
                axes[row,col].plot(axx,med,'o-',color=color,label=policy)
                axes[row,col].vlines(axx,[float(r[field+'_min']) for r in accepted],[float(r[field+'_max']) for r in accepted],color=color,alpha=.45)
        for col in range(3):
            axes[row,col].set_xlabel('Active payload arena budget (bytes)')
            axes[row,col].grid(alpha=.2)
        axes[row,0].set_ylim(-.05,1.05); axes[row,1].set_ylim(-.05,1.05)
        axes[row,0].set_ylabel(family+'\nReplay admission fraction')
        axes[row,1].set_ylabel('Request failure fraction\n(admitted traces)')
        axes[row,2].set_ylabel('Peak occupied bytes\n(admitted traces)')
    axes[0,0].legend(fontsize=7)
    fig.suptitle('Memory traces — %s\nMedian and min–max of 10 distinct traces; constant/fixture each use 1 canonical trace. Rejected traces have no runtime rates.'%batch,fontsize=10)
    fig.tight_layout(); save(fig,out/'memory.png')
    (out/'plots.json').write_text(json.dumps(dict(batch=batch,analysis_sha256=digest(analysis/'analysis.json'),script_sha256=digest(__file__)),indent=2)+'\n')
    print('Saved plots:',out)


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('analysis',type=Path)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    plot(args.analysis,args.out)
