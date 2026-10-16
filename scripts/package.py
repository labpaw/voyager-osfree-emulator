#!/usr/bin/env python3
"""Bundle source and one full evidence batch, with a file hash inventory."""
import argparse
import hashlib
import json
import subprocess
import zipfile
from pathlib import Path, PurePosixPath

ROOT = 'voyager-osfree-emulator/'


def check(path):
    with zipfile.ZipFile(path) as archive:
        manifest = json.loads(archive.read(ROOT+'ARTIFACT_MANIFEST.json'))
        expected = {ROOT+p for p in manifest['files']} | {ROOT+'ARTIFACT_MANIFEST.json'}
        names = archive.namelist()
        assert len(names) == len(set(names)) and set(names) == expected
        for name in names:
            assert name.startswith(ROOT) and '..' not in PurePosixPath(name).parts
            assert not any(p in ('.git','.venv','__pycache__') for p in PurePosixPath(name).parts)
        for name,info in manifest['files'].items():
            content = archive.read(ROOT+name)
            assert len(content) == info['bytes']
            assert hashlib.sha256(content).hexdigest() == info['sha256'], name
        print('Archive verified:',len(manifest['files']),'files,',sum(i['bytes'] for i in manifest['files'].values()),'uncompressed bytes')


def package(path):
    if path.exists(): raise FileExistsError(path)
    files = [Path(p) for p in ('README.md','CMakeLists.txt','IRP_Emulator_Codex_Build_Plan.md','.gitignore')]
    for base in ('include','src','tests','scripts','configs','docs','results/full'):
        files.extend(p for p in Path(base).rglob('*') if p.is_file() and '__pycache__' not in p.parts and not p.name.endswith('.pyc'))
    files = sorted(files)
    revision = subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()
    manifest = dict(schema_version=1,source_revision=revision,
                    scope='source, inputs, verification notes, full raw and derived evidence; no builds, caches or duplicate smoke batches',
                    archive_timestamps='normalised to ZIP epoch; not measurement dates',files={})
    path.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(path,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
        for file in files:
            content = file.read_bytes()
            manifest['files'][file.as_posix()] = dict(bytes=len(content),sha256=hashlib.sha256(content).hexdigest())
            entry = zipfile.ZipInfo(ROOT+file.as_posix())
            entry.compress_type = zipfile.ZIP_DEFLATED
            entry.external_attr = 0o100644 << 16
            archive.writestr(entry,content)
        entry = zipfile.ZipInfo(ROOT+'ARTIFACT_MANIFEST.json')
        entry.compress_type = zipfile.ZIP_DEFLATED
        archive.writestr(entry,json.dumps(manifest,indent=2,sort_keys=True)+'\n')
    check(path)
    print('Saved',path)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    choice = parser.add_mutually_exclusive_group(required=True)
    choice.add_argument('--out',type=Path)
    choice.add_argument('--check',type=Path)
    args = parser.parse_args()
    if args.check: check(args.check)
    else: package(args.out)
