import importlib.util
import pathlib
import unittest

spec = importlib.util.spec_from_file_location("meeting_config_frames", pathlib.Path(__file__).parents[2] / "tools" / "meeting_config_frames.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class FramesTest(unittest.TestCase):
    def test_bounded_chunks_and_offsets(self):
        payload = bytes(range(256)) * 20
        encoded = list(module.frames(payload))
        self.assertEqual(encoded[0], b"\x01\x00\x14")
        self.assertEqual(encoded[-1], b"\x03")
        result = bytearray()
        for frame in encoded[1:-1]:
            self.assertLessEqual(len(frame) + 16, 480)
            self.assertEqual(frame[0], 2)
            self.assertEqual(int.from_bytes(frame[1:3], "little"), len(result))
            result.extend(frame[3:])
        self.assertEqual(bytes(result), payload)

    def test_reject_invalid_lengths(self):
        for payload in (b"", b"x" * 6145):
            with self.assertRaises(ValueError):
                list(module.frames(payload))
        with self.assertRaises(ValueError):
            list(module.frames(b"x", 401))


if __name__ == "__main__":
    unittest.main()
