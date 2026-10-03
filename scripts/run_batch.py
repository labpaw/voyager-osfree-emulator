#!/usr/bin/env python3
import argparse
import hashlib
import json
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from check_csv import check_schedule, check_memory


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def source_hash():
    h = hashlib.sha256()
    paths = [Path('CMakeLists.txt')]
    for base in ('include', 'src', 'scripts', 'tests'):
        paths += sorted(p for p in Path(base).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
    for path in sorted(paths):
        h.update(str(path).encode()); h.update(path.read_bytes())
    return h.hexdigest()


def run_one(exe, item, folder):
    command = [str(exe), 'schedule', '--tasks', item['input'], '--horizon', str(item.get('horizon',0)), '--budget', str(item['budget']), '--out', str(folder), '--run-id', item['id']]
    if item['kind'] == 'memory':
        command = [str(exe), 'memory', '--trace', item['input'], '--policy', item['policy'],
                   '--budget', str(item['budget']), '--out', str(folder), '--run-id', item['id']]
    done = subprocess.run(command, capture_output=True, text=True)
    if done.returncode not in (0, 3):
        raise RuntimeError('%s: %s %s' % (item['id'], done.stdout, done.stderr))
    status = check_memory(folder) if item['kind'] == 'memory' else check_schedule(folder)
    assert (done.returncode == 3) == (status == 'memory_rejected')
    return command, status


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--profile', choices=['smoke','full'], default='smoke')
    parser.add_argument('--exe', type=Path, default=Path('build/emulator'))
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    plan = json.loads(Path('configs/experiments.json').read_text())
    if Path('configs/frozen.json').exists():
        frozen = json.loads(Path('configs/frozen.json').read_text())
        assert frozen['plan_sha256'] == digest('configs/experiments.json'), 'plan changed since freeze'
        assert frozen['tasksets_sha256'] == digest('configs/tasksets.csv')
        for path,expected in frozen['inputs'].items():
            assert digest(path) == expected, ('frozen input changed',path)
    selected = [r for r in plan['runs'] if args.profile == 'full' or r['smoke']]
    args.out.mkdir(parents=True, exist_ok=False)
    revision = subprocess.check_output(['git','rev-parse','HEAD'], text=True).strip()
    manifest = dict(schema_version=1, profile=args.profile, generator=plan['generator'],
                    source_revision=revision, source_sha256=source_hash(),
                    dirty=bool(subprocess.check_output(['git','status','--porcelain'],text=True).strip()),
                    platform=platform.platform(), python=sys.version,
                    build=json.loads(subprocess.check_output([str(args.exe.resolve()),'info'],text=True)),
                    binary_sha256=digest(args.exe),
                    plan_sha256=digest('configs/experiments.json'), cwd=str(Path.cwd()),
                    command=sys.argv, runs=[])
    shutil.copyfile('configs/tasksets.csv',args.out/'tasksets.csv')
    target = args.out / 'manifest.json'
    for item in selected:
        manifest['runs'].append(dict(item, input_sha256=digest(item['input']), status='pending'))
    target.write_text(json.dumps(manifest,indent=2)+'\n')
    for i, entry in enumerate(manifest['runs']):
        folder = args.out / entry['group'] / entry['id']
        try:
            assert digest(entry['input']) == entry['input_sha256']
            command, status = run_one(args.exe.resolve(), entry, folder)
            assert digest(entry['input']) == entry['input_sha256']
            entry.update(status=status, command=command, output=str(folder.relative_to(args.out)),
                         output_sha256={p.name:digest(p) for p in sorted(folder.iterdir())})
        except Exception as error:
            entry.update(status='unexpected_failure', reason=str(error))
            target.write_text(json.dumps(manifest,indent=2)+'\n')
            raise
        target.write_text(json.dumps(manifest,indent=2)+'\n')
        print('%d/%d %s: %s' % (i+1,len(selected),entry['id'],entry['status']), flush=True)
    print('Batch complete:', args.out)


if __name__ == '__main__':
    main()
