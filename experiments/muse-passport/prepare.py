#!/usr/bin/env python3
"""Fetch the pinned upstream SDK and apply this experimental board adapter."""
import argparse
from pathlib import Path
import shutil
import subprocess

PIN = '693cde9a884ad1edc87251b9f8944815f8de4809'
p = argparse.ArgumentParser()
p.add_argument('--source', default='https://github.com/facebookincubator/muse-gadget-sdk', help='Upstream URL or local clone for offline preparation')
a = p.parse_args()
root = Path(__file__).resolve().parent
sdk = root / '.work/muse-gadget-sdk'
if sdk.exists():
    raise SystemExit(f'{sdk} already exists; preserve it or choose a fresh copy of this experiment.')
sdk.parent.mkdir(parents=True, exist_ok=True)
subprocess.run(['git', 'clone', '--no-checkout', a.source, str(sdk)], check=True)
subprocess.run(['git', '-C', str(sdk), 'checkout', '--detach', PIN], check=True)
subprocess.run(['git', '-C', str(sdk), 'apply', '--check', str(root / 'muse-sdk.patch')], check=True)
subprocess.run(['git', '-C', str(sdk), 'apply', str(root / 'muse-sdk.patch')], check=True)
shutil.copytree(root / 'overlay', sdk, dirs_exist_ok=True)
print(f'Prepared {sdk}/esp32. Read upstream AGENTS.md before building.')
