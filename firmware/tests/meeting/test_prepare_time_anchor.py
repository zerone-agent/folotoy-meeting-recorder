import hashlib
import importlib.util
import json
from pathlib import Path
import unittest
spec = importlib.util.spec_from_file_location('prepare_time_anchor', Path(__file__).parents[2] / 'tools/prepare_time_anchor.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
class Response:
    status = 200
    def __init__(self, body): self.body = body
    def __enter__(self): return self
    def __exit__(self, *args): return False
    def read(self, limit): return self.body[:limit]
class Opener:
    def __init__(self, value): self.value = value
    def open(self, request, timeout):
        assert request.full_url == 'https://example.com/meeting-service/admin/time-signer'
        assert request.get_header('Authorization') == 'Bearer secret'
        return Response(json.dumps(self.value).encode())
class TestPrepare(unittest.TestCase):
    def test_signer_binding(self):
        key = '04' + '22' * 64
        value = {'origin': 'https://example.com', 'keyId': hashlib.sha256(bytes.fromhex(key)).hexdigest(), 'publicKey': key}
        output = module.prepare(value['origin'], 'aabbccddeeff', 'secret', Opener(value))
        self.assertIn('CONFIG_MEETING_AGENT_TIME_PROVISION=y', output)
        self.assertNotIn('secret', output)
        value['keyId'] = '00' * 32
        with self.assertRaises(ValueError): module.prepare(value['origin'], 'aabbccddeeff', 'secret', Opener(value))
    def test_bad_origin_and_token(self):
        for origin in ['http://example.com', 'https://user@example.com', 'https://example.com/path']:
            with self.assertRaises(ValueError): module.prepare(origin, 'aabbccddeeff', 'secret', None)
        with self.assertRaises(ValueError): module.prepare('https://example.com', 'aabbccddeeff', 'bad token', None)
if __name__ == '__main__': unittest.main()
