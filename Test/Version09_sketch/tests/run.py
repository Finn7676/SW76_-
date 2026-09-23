#!/usr/bin/env python3
"""Run gameplay regressions in both builds, counter tests, and benchmark parser tests."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true', help='Enable AddressSanitizer and UBSan')
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
flags = ['-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror']
flags += ['-g', '-fsanitize=address,undefined'] if args.sanitize else ['-O2']
with tempfile.TemporaryDirectory(prefix='xxqz-v05-tests-') as output:
    for name in ['integration', 'debug', 'perception', 'derived', 'navigation_state', 'candidate']:
        for enabled in [0, 1]:
            binary = Path(output) / f'{name}-{enabled}'
            subprocess.run([*shlex.split(os.environ.get('CXX', 'clang++')), *flags,
                            f'-DXXQZ_ENABLE_STATS={enabled}', str(root / 'tests' / f'{name}.cpp'), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
    subprocess.run([sys.executable, '-B', str(root / 'tests/benchmark_test.py')], check=True)

subprocess.run([sys.executable, '-B', str(root / 'tests/observer_test.py')], check=True)
