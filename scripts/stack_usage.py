#!/usr/bin/env python3
import argparse
import subprocess
import tempfile
from pathlib import Path
parser = argparse.ArgumentParser()
parser.add_argument('--compiler', default='c++')
args = parser.parse_args()
print(subprocess.check_output([args.compiler,'--version'],text=True).splitlines()[0])
print('Per-function compiler estimate at -O3; not a measured whole call-chain high-water mark.')
with tempfile.TemporaryDirectory() as tmp:
    for source in sorted(Path('src/core').glob('*.cpp')):
        obj = Path(tmp)/(source.stem+'.o')
        subprocess.run([args.compiler,'-std=c++17','-O3','-fstack-usage','-Iinclude','-c',str(source),'-o',str(obj)],check=True)
        print(obj.with_suffix('.su').read_text(),end='')
