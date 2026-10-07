#!/usr/bin/env python3
"""Install/manage a per-user launchd service for the Muse LAN proxy."""
import argparse
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys

LABEL = 'com.musepassport.proxy'


def service_plist(python, runtime, log):
    return {'Label': LABEL,
            'ProgramArguments': [str(python), str(runtime), '--listen', '0.0.0.0', '--port', '18087', '--log-file', str(log)],
            'RunAtLoad': True, 'KeepAlive': True, 'ThrottleInterval': 10,
            'ExitTimeOut': 5, 'ProcessType': 'Background', 'Umask': 0o077,
            'WorkingDirectory': str(runtime.parent)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['install', 'status', 'uninstall'])
    args = parser.parse_args()
    if sys.platform != 'darwin':
        parser.error('launchd service requires macOS')
    home = Path.home()
    base = home / 'Library/Application Support/MusePassportProxy'
    logs = home / 'Library/Logs/MusePassportProxy'
    plist = home / 'Library/LaunchAgents' / (LABEL + '.plist')
    domain = f'gui/{os.getuid()}'
    target = domain + '/' + LABEL
    if args.action == 'status':
        raise SystemExit(subprocess.run(['launchctl', 'print', target]).returncode)
    if args.action == 'uninstall':
        subprocess.run(['launchctl', 'bootout', target], check=False)
        plist.unlink(missing_ok=True)
        print('Service removed; runtime and diagnostic logs retained at', base)
        return
    python = shutil.which('python3')
    if not python:
        parser.error('python3 not found')
    for folder in (base, logs, plist.parent):
        folder.mkdir(parents=True, exist_ok=True)
    base.chmod(0o700); logs.chmod(0o700)
    runtime = base / 'mac_proxy.py'
    temporary = runtime.with_suffix('.tmp')
    shutil.copyfile(Path(__file__).with_name('mac_proxy.py'), temporary)
    temporary.chmod(0o600)
    temporary.replace(runtime)
    data = plistlib.dumps(service_plist(python, runtime, logs / 'proxy.log'))
    temporary = plist.with_suffix('.tmp')
    temporary.write_bytes(data); temporary.chmod(0o600); temporary.replace(plist)
    existing = subprocess.run(['launchctl', 'print', target], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if existing.returncode == 0:
        subprocess.run(['launchctl', 'bootout', target], check=True)
    subprocess.run(['launchctl', 'bootstrap', domain, str(plist)], check=True)
    print('Installed', LABEL, '(login start, automatic restart, 10-second throttle)')
    print('Log:', logs / 'proxy.log')
    print('Use status to confirm a running PID and check port 18087 before use.')


if __name__ == '__main__':
    main()
