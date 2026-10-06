#!/usr/bin/env python3
"""Build with an activated IDF 6.0.1 and a private token file; never print token."""
import os
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parent
sdk = root / '.work/muse-gadget-sdk/esp32'
version = subprocess.check_output(['idf.py', '--version'], text=True).strip()
if not re.search(r'ESP-IDF v6\.0\.1(?:\s|$)', version):
    raise SystemExit('Activate the clean official ESP-IDF v6.0.1 toolchain first.')
source = os.environ.get('MUSE_TOKEN_FILE')
if not source:
    raise SystemExit('Set MUSE_TOKEN_FILE to your private SDK token file; do not put a token in command arguments.')
token = Path(source).expanduser().read_text().strip()
if not re.fullmatch(r'mgst_[A-Za-z0-9_-]+', token):
    raise SystemExit('Invalid SDK token file format.')
local = root / '.local'
local.mkdir(mode=0o700, exist_ok=True)
local.chmod(0o700)
config = local / 'sdkconfig.token'
fd = os.open(config, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
with os.fdopen(fd, 'w') as f:
    f.write(f'CONFIG_GADGET_SDK_TOKEN="{token}"\n')
config.chmod(0o600)
build = 'build-muse-folotoy-ai-passport'
# This directory contains token-bearing configuration, binaries and debug data.
(sdk / build).mkdir(mode=0o700, exist_ok=True)
(sdk / build).chmod(0o700)
# Rebuilding an existing generated config must also refresh its private token.
generated = sdk / build / 'sdkconfig'
if generated.exists():
    s = generated.read_text()
    s = re.sub(r'^CONFIG_GADGET_SDK_TOKEN=.*\n?', '', s, flags=re.M)
    generated.write_text(s + f'\nCONFIG_GADGET_SDK_TOKEN="{token}"\n')
    generated.chmod(0o600)
subprocess.run(['idf.py', '-B', build, '-DIDF_TARGET=esp32c3', f'-DSDKCONFIG={build}/sdkconfig',
    '-DSDKCONFIG_DEFAULTS=sdkconfig.defaults;devices/sdkconfig.muse;devices/sdkconfig.muse-folotoy-ai-passport;' + str(config), 'build'], cwd=sdk, check=True)
generated.chmod(0o600)
print('Build complete. Firmware contains your SDK token: keep binaries private.')
