import hashlib
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).parents[2] / 'tools'))
import meeting_device_export as export


def table(flags=0, offset=export.STORAGE_OFFSET):
    row = struct.pack('<HBBII16sI', 0x50aa, 1, 0x02, export.NVS_OFFSET, export.NVS_SIZE, b'nvs', 0)
    row += struct.pack('<HBBII16sI', 0x50aa, 1, 0x83, offset, export.STORAGE_SIZE, b'storage', flags)
    checksum = b'\xeb\xeb' + b'\xff' * 14 + hashlib.md5(row).digest()
    return (row + checksum).ljust(export.TABLE_SIZE, b'\xff')


class ExportTest(unittest.TestCase):
    def test_layout_and_integrity(self):
        self.assertEqual(export.verify_table(table()), (export.STORAGE_OFFSET, export.STORAGE_SIZE))
        for raw in (table(flags=1), table(offset=0x300000), table()[:500], b'\xff' * export.TABLE_SIZE):
            with self.assertRaises(ValueError):
                export.verify_table(raw)
        corrupt = bytearray(table()); corrupt[16] ^= 1
        with self.assertRaises(ValueError):
            export.verify_table(bytes(corrupt))
        duplicate = table()[:32] + table()[:64]
        checksum = b'\xeb\xeb' + b'\xff' * 14 + hashlib.md5(duplicate).digest()
        with self.assertRaises(ValueError):
            export.verify_table((duplicate + checksum).ljust(export.TABLE_SIZE, b'\xff'))

    def test_read_only_commands_and_no_overwrite(self):
        calls = []
        def runner(command, check):
            self.assertTrue(check)
            calls.append(command)
            self.assertIn('read_flash', command)
            self.assertNotIn('write_flash', command)
            path = Path(command[-1])
            if len(calls) == 1:
                path.write_bytes(table())
            elif len(calls) == 2:
                with path.open('wb') as output:
                    output.truncate(export.NVS_SIZE)
            else:
                with path.open('wb') as output:
                    output.truncate(export.STORAGE_SIZE)
        with tempfile.TemporaryDirectory() as root:
            target = Path(root) / 'recovery'
            result = export.read_device('/dev/test', target, runner)
            self.assertEqual(result.stat().st_size, export.STORAGE_SIZE)
            self.assertEqual(target.stat().st_mode & 0o777, 0o700)
            with self.assertRaises(FileExistsError):
                export.read_device('/dev/test', target, runner)
            self.assertEqual((target / 'nvs.bin').stat().st_size, export.NVS_SIZE)
            self.assertEqual(len(calls), 3)
            self.assertTrue(all(call[call.index('--baud') + 1] == '115200' for call in calls))
            self.assertEqual([call[call.index('--after') + 1] for call in calls],
                             ['no_reset', 'no_reset', 'hard_reset'])
            self.assertEqual([call[call.index('--before') + 1] for call in calls],
                             ['default_reset', 'no_reset', 'no_reset'])

    def test_bad_table_never_reads_storage(self):
        calls = []
        def runner(command, check):
            calls.append(command)
            Path(command[-1]).write_bytes(table(flags=1))
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaises(ValueError):
                export.read_device('/dev/test', Path(root) / 'recovery', runner)
        self.assertEqual(len(calls), 1)

    def test_invalid_baud_does_not_create_directory(self):
        with tempfile.TemporaryDirectory() as root:
            target = Path(root) / 'recovery'
            with self.assertRaises(ValueError):
                export.read_device('/dev/test', target, baud=0)
            self.assertFalse(target.exists())

    def test_incomplete_nvs_never_reads_storage(self):
        calls = []
        def runner(command, check):
            calls.append(command)
            Path(command[-1]).write_bytes(table() if len(calls) == 1 else b'incomplete')
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaises(ValueError):
                export.read_device('/dev/test', Path(root) / 'recovery', runner)
        self.assertEqual(len(calls), 2)

    def test_interrupted_storage_read_preserves_partial_backup(self):
        calls = []
        def runner(command, check):
            calls.append(command)
            path = Path(command[-1])
            if len(calls) == 1:
                path.write_bytes(table())
            elif len(calls) == 2:
                with path.open('wb') as output:
                    output.truncate(export.NVS_SIZE)
            else:
                path.write_bytes(b'partial image')
                raise subprocess.CalledProcessError(1, command)
        with tempfile.TemporaryDirectory() as root:
            target = Path(root) / 'recovery'
            with self.assertRaises(subprocess.CalledProcessError):
                export.read_device('/dev/test', target, runner)
            self.assertEqual((target / 'storage.bin').read_bytes(), b'partial image')
            self.assertEqual((target / 'nvs.bin').stat().st_size, export.NVS_SIZE)
            with self.assertRaises(FileExistsError):
                export.read_device('/dev/test', target, runner)
            self.assertEqual((target / 'storage.bin').read_bytes(), b'partial image')
        self.assertEqual(len(calls), 3)


if __name__ == '__main__':
    unittest.main()
