#!/usr/bin/env python3
"""Narrow dependency audit; complements rather than replaces the new guard."""
import argparse
import re
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--library',type=Path,default=Path('build/libemulator_core.a'))
args = parser.parse_args()
for base in ('src/core','include/emulator'):
    for path in Path(base).rglob('*'):
        if not path.is_file(): continue
        text = path.read_text()
        assert not re.search(r'\b(malloc|calloc|realloc|free|aligned_alloc|sleep)\s*\(|\b(new|delete)\s+|std::(thread|system_clock|fstream|iostream|vector|string|function)\b',text), path
symbols = subprocess.check_output(['nm','-u',str(args.library)],text=True)
assert not re.search(r'\b_?(malloc|calloc|realloc|free|aligned_alloc|posix_memalign|_Zn\w*)\b',symbols), symbols
print('Core source and undefined-symbol allocation audit passed')
print(symbols)
