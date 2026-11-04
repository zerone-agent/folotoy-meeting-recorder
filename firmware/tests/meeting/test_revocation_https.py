"""Optional integration: real service, TLS, persisted revoke and lost response."""
import hashlib
import json
import os
from pathlib import Path
import select
import socket
import ssl
import subprocess
import sys
import tempfile
import urllib.request

repo = Path(__file__).resolve().parents[3]
script = repo / 'firmware/tools/meeting_revocation_outbox.py'
server = repo / 'firmware/tests/upload_https/server.mjs'


def run():
    with tempfile.TemporaryDirectory(prefix='meeting-revocation-https-') as root:
        root = Path(root)
        cert, key, wrong_cert, wrong_key = [root / name for name in ('cert.pem', 'key.pem', 'wrong.pem', 'wrong-key.pem')]
        for c, k in ((cert, key), (wrong_cert, wrong_key)):
            subprocess.run(['openssl', 'req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-keyout', str(k),
                            '-out', str(c), '-days', '1', '-subj', '/CN=localhost', '-addext',
                            'subjectAltName=DNS:localhost'], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        with socket.socket() as sock:
            sock.bind(('127.0.0.1', 0)); port = sock.getsockname()[1]
        env = dict(os.environ, TEST_REPO=str(repo), TEST_PORT=str(port), TEST_DB=str(root / 'db.sqlite'),
                   TEST_CERT=str(cert), TEST_KEY=str(key), TEST_DROP_REVOKE_ONCE='1')
        def start():
            process = subprocess.Popen(['node', str(server)], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            ready, _, _ = select.select([process.stdout], [], [], 15)
            if not ready or 'ready' not in process.stdout.readline():
                process.kill(); process.wait(timeout=5)
                raise RuntimeError('test server did not start')
            return process
        proc = start()
        try:
            origin, path = f'https://localhost:{port}', root / 'outbox.json'
            # Exercise registration -> version 2 configuration against the real service.
            context = ssl.create_default_context(cafile=str(cert))
            registration_request = urllib.request.Request(origin + '/meeting-service/devices',
                data=json.dumps({'id': 'new_device'}).encode(), method='POST',
                headers={'Authorization': 'Bearer ' + 'a' * 40, 'Content-Type': 'application/json'})
            with urllib.request.urlopen(registration_request, context=context, timeout=10) as response:
                registration = json.load(response)
            pem = cert.read_text()
            base = {'version': 1, 'ssid': 'test', 'password': 'password', 'origin': origin,
                    'deviceToken': registration['token'], 'caPem': pem,
                    'caSha256': hashlib.sha256(ssl.PEM_cert_to_DER_cert(pem)).hexdigest(),
                    'utc': 1800000000, 'timezoneMinutes': 480}
            base_file, registration_file, config_file = root / 'base.json', root / 'registration.json', root / 'config.json'
            base_file.write_text(json.dumps(base)); registration_file.write_text(json.dumps(registration))
            os.chmod(base_file, 0o600); os.chmod(registration_file, 0o600)
            subprocess.run([sys.executable, str(repo / 'firmware/tools/meeting_prepare_identity.py'), str(base_file),
                            str(registration_file), str(config_file)], check=True, capture_output=True)
            config = json.loads(config_file.read_text())
            assert config['version'] == 2 and config['deviceId'] == registration['id']
            assert config['credentialId'] == registration['credentialId']
            cred = hashlib.sha256(b'd' * 40).hexdigest()
            subprocess.run([sys.executable, str(script), 'create', str(path), '--origin', origin,
                            '--device-id', 'device', '--credential-id', cred, '--event-id', 'event_1'], check=True, capture_output=True)
            def invoke(token, ca):
                return subprocess.run([sys.executable, str(script), 'retry', str(path), '--ca', str(ca)],
                                      env=dict(os.environ, MEETING_ADMIN_TOKEN=token), capture_output=True, timeout=20)
            assert invoke('a' * 40, wrong_cert).returncode != 0
            assert json.loads(path.read_text())['state'] == 'pending'
            assert invoke('x' * 40, cert).returncode != 0
            assert json.loads(path.read_text())['state'] == 'pending'
            # Real server commits the revoke, but the proxy discards its receipt.
            assert invoke('a' * 40, cert).returncode != 0
            assert json.loads(path.read_text())['state'] == 'pending'
            ctx = ssl.create_default_context(cafile=str(cert))
            req = urllib.request.Request(origin + '/meeting-service/devices/device', headers={'Authorization': 'Bearer ' + 'a' * 40})
            with urllib.request.urlopen(req, context=ctx, timeout=10) as response:
                assert json.load(response)['revoked'] is True
            proc.terminate(); proc.wait(timeout=5)
            env['TEST_DROP_REVOKE_ONCE'] = '0'
            proc = start()
            with urllib.request.urlopen(req, context=ctx, timeout=10) as response:
                assert json.load(response)['revoked'] is True
            assert invoke('a' * 40, cert).returncode == 0
            assert json.loads(path.read_text())['state'] == 'confirmed'
            assert 'a' * 40 not in path.read_text()
            assert invoke('a' * 40, cert).returncode == 0
            print('PASS: registration and configuration, wrong CA/admin, lost response, restart and idempotent revoke')
        finally:
            proc.terminate()
            try: proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill(); proc.wait(timeout=5)


if __name__ == '__main__': run()
