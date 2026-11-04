#!/usr/bin/env python3
"""Durable local revocation outbox; never stores administrator credentials.

This is the local half of the reset protocol. Until device-side authenticated
export and acknowledgment exist, a pending entry must be supplied separately.
"""
import argparse
import fcntl
import json
import os
from pathlib import Path
import re
import ssl
import tempfile
import urllib.request

ID = re.compile(r"[A-Za-z0-9_-]{1,100}\Z")
HASH = re.compile(r"[a-f0-9]{64}\Z")
ORIGIN = re.compile(r"https://[A-Za-z0-9.-]+(?::[0-9]{1,5})?\Z")
FIELDS = {"version", "origin", "deviceId", "credentialId", "eventId", "state"}


def validate(entry):
    if not isinstance(entry, dict) or set(entry) != FIELDS or type(entry['version']) is not int or entry['version'] != 1:
        raise ValueError('invalid outbox record')
    if (not isinstance(entry['origin'], str) or not ORIGIN.fullmatch(entry['origin']) or
        not isinstance(entry['deviceId'], str) or not ID.fullmatch(entry['deviceId']) or
        not isinstance(entry['eventId'], str) or not ID.fullmatch(entry['eventId']) or
        not isinstance(entry['credentialId'], str) or not HASH.fullmatch(entry['credentialId']) or
        entry['state'] not in ('pending', 'confirmed')):
        raise ValueError('invalid outbox record')
    return entry


def _strict_json(data):
    def pairs(items):
        obj = {}
        for key, value in items:
            if key in obj:
                raise ValueError('duplicate JSON key')
            obj[key] = value
        return obj
    return json.loads(data, object_pairs_hook=pairs)


def _sync_dir(directory):
    fd = os.open(directory, os.O_RDONLY)
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


def _atomic_write(path, entry):
    data = (json.dumps(validate(entry), separators=(',', ':'), sort_keys=True) + '\n').encode()
    fd, name = tempfile.mkstemp(prefix='.revocation-', dir=path.parent)
    try:
        os.fchmod(fd, 0o600)
        with os.fdopen(fd, 'wb') as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(name, path)
        _sync_dir(path.parent)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def create(path, origin, device_id, credential_id, event_id):
    path = Path(path)
    entry = validate(dict(version=1, origin=origin, deviceId=device_id,
                          credentialId=credential_id, eventId=event_id, state='pending'))
    # The lock file serializes create/retry in this directory; never overwrite
    # an existing operation, including a corrupted or incomplete one.
    with _locked(path):
        if path.exists() or path.is_symlink():
            raise FileExistsError('outbox already exists')
        _atomic_write(path, entry)
    return entry


class _locked:
    def __init__(self, path):
        self.path = path.parent / ('.' + path.name + '.lock')
        self.fd = None
    def __enter__(self):
        self.fd = os.open(self.path, os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW, 0o600)
        fcntl.flock(self.fd, fcntl.LOCK_EX)
    def __exit__(self, *_):
        fcntl.flock(self.fd, fcntl.LOCK_UN)
        os.close(self.fd)


def retry(path, admin_token, ca_pem=None, opener=None):
    path = Path(path)
    if not isinstance(admin_token, str) or len(admin_token) < 32 or any(ord(c) <= 32 or ord(c) >= 127 for c in admin_token):
        raise ValueError('invalid administrator credential')
    with _locked(path):
        if path.is_symlink():
            raise ValueError('outbox must not be a symlink')
        raw = path.read_bytes()
        if len(raw) > 4096:
            raise ValueError('outbox record too large')
        entry = validate(_strict_json(raw))
        if entry['state'] == 'confirmed':
            return True
        if opener is None:
            context = ssl.create_default_context(cadata=ca_pem) if ca_pem else ssl.create_default_context()
            if ca_pem:
                context.verify_flags |= ssl.VERIFY_X509_PARTIAL_CHAIN
            opener = urllib.request.build_opener(urllib.request.ProxyHandler({}),
                                                  urllib.request.HTTPSHandler(context=context),
                                                  _NoRedirect())
        body = json.dumps({'credentialId': entry['credentialId'], 'eventId': entry['eventId']}).encode()
        request = urllib.request.Request(entry['origin'] + '/meeting-service/devices/' + entry['deviceId'] + '/revoke',
                                         data=body, method='POST',
                                         headers={'Authorization': 'Bearer ' + admin_token,
                                                  'Content-Type': 'application/json'})
        with opener.open(request, timeout=10) as response:
            if response.status != 200:
                raise ValueError('revocation response not confirmed')
            data = response.read(4097)
        if len(data) > 4096:
            raise ValueError('revocation response too large')
        receipt = _strict_json(data)
        if not isinstance(receipt, dict) or receipt.get('revoked') is not True or receipt.get('eventId') != entry['eventId']:
            raise ValueError('revocation response not confirmed')
        entry['state'] = 'confirmed'
        _atomic_write(path, entry)
        return True


class _NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='action', required=True)
    init = sub.add_parser('create')
    init.add_argument('path', type=Path)
    init.add_argument('--origin', required=True)
    init.add_argument('--device-id', required=True)
    init.add_argument('--credential-id', required=True)
    init.add_argument('--event-id', required=True)
    send = sub.add_parser('retry')
    send.add_argument('path', type=Path)
    send.add_argument('--ca', type=Path)
    args = parser.parse_args()
    if args.action == 'create':
        create(args.path, args.origin, args.device_id, args.credential_id, args.event_id)
        print('Revocation pending')
    else:
        token = os.environ.get('MEETING_ADMIN_TOKEN', '')
        retry(args.path, token, args.ca.read_text() if args.ca else None)
        print('Revocation confirmed; device acknowledgment still required')


if __name__ == '__main__':
    main()
