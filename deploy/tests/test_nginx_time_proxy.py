#!/usr/bin/env python3
"""Exercise the example with the prebuilt official Nginx image; no image build."""
import contextlib
import http.client
import http.server
import pathlib
import subprocess
import tempfile
import threading
import time
import unittest
import uuid

CONFIG = pathlib.Path(__file__).resolve().parents[1] / 'nginx-time-proof.conf'
IMAGE = 'nginx:1.27-alpine'
seen = []
class Backend(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        seen.append((self.path, self.headers.get('Authorization'), self.headers.get('Cookie'), self.headers.get('X-Api-Key')))
        body = b'{"proof":"fixture"}'
        self.send_response(200); self.send_header('Content-Length', str(len(body))); self.end_headers(); self.wfile.write(body)
    def log_message(self, *args): pass

class Proxy(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix='meeting-nginx-')
        cls.backend = http.server.ThreadingHTTPServer(('0.0.0.0', 0), Backend)
        cls.worker = threading.Thread(target=cls.backend.serve_forever, daemon=True); cls.worker.start()
        conf = pathlib.Path(cls.tmp.name) / 'default.conf'
        conf.write_text(CONFIG.read_text().replace('127.0.0.1:3443', 'host.docker.internal:%d' % cls.backend.server_port))
        cls.name = 'meeting-proxy-test-' + uuid.uuid4().hex[:12]
        result = subprocess.run(['docker','run','--detach','--rm','--name',cls.name,'--publish','127.0.0.1::80','--volume',str(conf)+':/etc/nginx/conf.d/default.conf:ro',IMAGE],capture_output=True,text=True)
        if result.returncode:
            cls.backend.shutdown(); cls.tmp.cleanup(); raise RuntimeError(result.stderr)
        try:
            output = subprocess.check_output(['docker','port',cls.name,'80/tcp'],text=True).strip()
            cls.port = int(output.rsplit(':',1)[1])
            for _ in range(100):
                try:
                    conn=http.client.HTTPConnection('127.0.0.1',cls.port,timeout=1);conn.request('GET','/');conn.getresponse().read();conn.close();break
                except OSError: time.sleep(.1)
            else: raise RuntimeError('Nginx did not start')
        except BaseException:
            cls.tearDownClass(); raise
    @classmethod
    def tearDownClass(cls):
        subprocess.run(['docker','stop','--time','1',cls.name],capture_output=True)
        cls.backend.shutdown();cls.backend.server_close();cls.worker.join();cls.tmp.cleanup()
    def request(self,method,path,headers=None):
        c=http.client.HTTPConnection('127.0.0.1',self.port,timeout=3)
        try: c.request(method,path,headers=headers or {});r=c.getresponse();r.read();return r.status
        finally: c.close()
    def test_public_time_only_and_no_credentials_forwarded(self):
        seen.clear(); path='/meeting-service/time-proof/'+'01'*32
        self.assertEqual(self.request('GET',path,{'Authorization':'Bearer test-token','Cookie':'session=test','X-Api-Key':'test-key'}),200)
        self.assertEqual(seen,[(path,None,None,None)])
        for path in ['/meeting-service/admin/settings','/meeting-service/meetings/a','/v1/models','/meeting-service/time-proof/'+'0'*63,'/meeting-service/time-proof/'+'AA'*32]:
            self.assertEqual(self.request('GET',path),404)
        self.assertEqual(self.request('GET','/meeting-service/time-proof/'+'01'*32+'?x=1'),400)
        self.assertEqual(self.request('POST','/meeting-service/time-proof/'+'01'*32),405)
        self.assertEqual(len(seen),1)
if __name__ == '__main__': unittest.main()
