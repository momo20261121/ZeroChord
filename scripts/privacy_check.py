#!/usr/bin/env python3
"""Scan everything a release would publish for personal information.

Checks the tracked and not-ignored files, commit metadata since the last release
tag, and optionally a built .deb. Private values come from the ignored .env, the
Git configuration and the local account; they are reported by label, never echoed.
Exit status 1 means at least one FAIL. WARN items need a human decision.
"""
import argparse
import getpass
import io
import os
from pathlib import Path
import re
import socket
import subprocess
import tarfile

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--since', help='ref before the release (default: newest v* tag)')
parser.add_argument('--deb', type=Path, help='also scan this Debian package')
args = parser.parse_args()


def git(*cmd):
    done = subprocess.run(['git', *cmd], cwd=root, capture_output=True, text=True)
    return done.stdout.strip() if done.returncode == 0 else ''


def read_env():
    values = {}
    path = root/'.env'
    if path.exists():
        for line in path.read_text().splitlines():
            match = re.match(r'\s*(?:export\s+)?([A-Z_]+)=(.*)', line)
            if match:
                value = match.group(2).strip()
                if len(value) > 1 and value[0] == value[-1] and value[0] in '\'"':
                    value = value[1:-1]
                values[match.group(1)] = value
    for key, value in os.environ.items():
        if key.startswith(('ZEROCHORD_', 'GIT_')):
            values[key] = value
    return values


env = read_env()
private = {}  # label -> term; a hit is a FAIL
public = {}   # label -> term; intended public identity, a hit is a WARN


def add(table, label, term):
    term = (term or '').strip()
    if len(term) >= 4 and term.lower() not in ('cardputer', 'zerochord') and 'zerochord' not in term.lower():
        table[label] = term


for key in ['ZEROCHORD_SSH_HOST', 'ZEROCHORD_SSH_CONFIG', 'GIT_AUTHOR_EMAIL', 'GIT_COMMITTER_EMAIL']:
    add(private, key, env.get(key))
if env.get('ZEROCHORD_REMOTE_DIR', 'ZeroChord') != 'ZeroChord':
    add(private, 'ZEROCHORD_REMOTE_DIR', env.get('ZEROCHORD_REMOTE_DIR'))
add(private, 'git user.email', git('config', 'user.email'))
add(private, 'git user.name', git('config', 'user.name'))
add(private, 'local account', getpass.getuser())
add(private, 'home directory', str(Path.home()))
add(private, 'project path', str(root))
add(private, 'hostname', socket.gethostname().split('.')[0])
for number, extra in enumerate(filter(None, env.get('ZEROCHORD_PRIVATE_TERMS', '').split(','))):
    add(private, f'ZEROCHORD_PRIVATE_TERMS[{number}]', extra)
add(public, 'ZEROCHORD_MAINTAINER', env.get('ZEROCHORD_MAINTAINER'))
add(public, 'ZEROCHORD_HOMEPAGE', env.get('ZEROCHORD_HOMEPAGE'))
account = re.search(r'[:/]([^/:]+)/[^/]+?(?:\.git)?$', git('remote', 'get-url', 'origin'))
if account:
    add(public, 'origin account name', account.group(1))

GENERIC = [
    ('FAIL', 'private key', re.compile(rb'-----BEGIN [A-Z ]*PRIVATE KEY-----')),
    ('FAIL', 'GitHub token', re.compile(rb'gh[pousr]_[A-Za-z0-9]{30,}')),
    ('FAIL', 'AWS key', re.compile(rb'AKIA[0-9A-Z]{16}')),
    ('WARN', 'user directory path', re.compile(rb'/(?:Users|home)/[A-Za-z0-9._-]+')),
    ('WARN', 'private IPv4 address',
     re.compile(rb'\b(?:10|192\.168|172\.(?:1[6-9]|2\d|3[01]))\.\d{1,3}\.\d{1,3}(?:\.\d{1,3})?\b')),
    ('WARN', 'email address', re.compile(rb'[\w.+-]+@[\w-]+(?:\.[\w-]+)+')),
]
HARMLESS_EMAIL = re.compile(rb'@(?:example\.(?:invalid|com|org)|users\.noreply\.github\.com)$|noreply', re.I)
findings = []


def report(level, where, what):
    findings.append((level, where, what))


def line_of(data, offset):
    return data.count(b'\n', 0, offset) + 1


def scan(where, data, third_party=False, public_ok=False):
    low = data.lower()
    text = b'\0' not in data[:8192]
    for table, level in [(private, 'FAIL')] + ([] if public_ok else [(public, 'WARN')]):
        for label, term in table.items():
            at = low.find(term.lower().encode())
            if at >= 0:
                report(level, f'{where}:{line_of(data, at)}' if text else where, f'contains {label}')
    if third_party:
        return
    for level, label, pattern in GENERIC:
        for match in pattern.finditer(data):
            if label == 'email address' and (HARMLESS_EMAIL.search(match.group()) or not text):
                continue
            report(level, f'{where}:{line_of(data, match.start())}' if text else where, label)
            break


# 1. Files that a release commit would publish.
files = git('ls-files', '--cached', '--others', '--exclude-standard').splitlines()
for name in sorted(set(files)):
    path = root/name
    if path.is_file():
        scan(name, path.read_bytes(), third_party=name.startswith('third_party/') or name in ('LICENSE', 'THIRD_PARTY_NOTICES.md'))
    scan_name = name.encode()
    for label, term in private.items():
        if term.lower().encode() in scan_name.lower():
            report('FAIL', name, f'file name contains {label}')

# 2. Commit metadata since the previous release.
since = args.since or git('describe', '--tags', '--abbrev=0', '--match', 'v*')
history = git('log', '--format=%H%x1f%an%x1f%ae%x1f%cn%x1f%ce%x1f%B%x1e', f'{since}..HEAD' if since else 'HEAD')
for entry in filter(None, (e.strip() for e in history.split('\x1e'))):
    sha, *fields = entry.split('\x1f')
    scan(f'commit {sha[:9]}', '\n'.join(fields).encode())

# 3. The built package: metadata, file names and every payload byte.
if args.deb:
    blob = args.deb.read_bytes()
    assert blob[:8] == b'!<arch>\n', 'not a Debian archive'
    at, members = 8, {}
    while at + 60 <= len(blob):
        header = blob[at:at+60]
        size = int(header[48:58])
        members[header[:16].decode().strip().rstrip('/')] = blob[at+60:at+60+size]
        at += 60 + size + size % 2
    version = (root/'VERSION.txt').read_text().strip()
    for member, payload in members.items():
        if not member.endswith('.tar.xz'):
            continue
        with tarfile.open(fileobj=io.BytesIO(payload), mode='r:xz') as tar:
            for info in tar:
                for label, term in private.items():
                    if term.lower() in info.name.lower():
                        report('FAIL', f'deb:{info.name}', f'file name contains {label}')
                if info.isfile():
                    data = tar.extractfile(info).read()
                    scan(f'deb:{info.name}', data, third_party='/third_party/' in info.name or info.name.endswith(('LICENSE', 'copyright', 'THIRD_PARTY_NOTICES.md')),
                         public_ok=member == 'control.tar.xz')
                    if member == 'control.tar.xz' and info.name.endswith('control'):
                        control = data.decode()
                        if f'Version: {version}\n' not in control:
                            report('FAIL', 'deb:control', f'version differs from VERSION.txt ({version})')
                        wanted = env.get('ZEROCHORD_MAINTAINER')
                        if wanted and f'Maintainer: {wanted}\n' not in control:
                            report('FAIL', 'deb:control', 'Maintainer differs from ZEROCHORD_MAINTAINER')

fails = [f for f in findings if f[0] == 'FAIL']
for level, where, what in sorted(set(findings)):
    print(f'{level} {where}: {what}')
print(f'privacy check: {len(fails)} FAIL, {len(findings) - len(fails)} WARN, '
      f'{len(set(files))} files, {len(private)}+{len(public)} private/public terms'
      + (f', package {args.deb.name}' if args.deb else ''))
raise SystemExit(1 if fails else 0)
