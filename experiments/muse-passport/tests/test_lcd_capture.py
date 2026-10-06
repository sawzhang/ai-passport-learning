import importlib.util
from pathlib import Path
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('capture_lcd', ROOT / 'capture_lcd.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class Capture(unittest.TestCase):
    def test_bands_reassemble_out_of_order_without_accepting_missing_pixels(self):
        f = module.Frame()
        f.feed('PXS BEGIN 2 1')
        f.feed('PXS 0 1 1 4Ac=')  # green RGB565
        with self.assertRaises(ValueError): f.feed('PXS END')
        self.assertFalse(f.feed('unrelated device log'))
        f.feed('PXS 0 0 1 APg=')  # red RGB565
        self.assertTrue(f.feed('PXS END'))
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'screen.png'; f.save(p)
            self.assertTrue(p.read_bytes().startswith(b'\x89PNG\r\n\x1a\n'))
            self.assertEqual(p.stat().st_mode & 0o777, 0o600)
    def test_bounds_and_payload_validation(self):
        for line in ['PXS 0 -1 1 APg=', 'PXS 1 0 1 APg=', 'PXS 0 1 1 APg=', 'PXS 0 0 2 APg=', 'PXS 0 0 1 AA==', 'PXS ABORT']:
            f = module.Frame(); f.feed('PXS BEGIN 1 1')
            with self.assertRaises(ValueError): f.feed(line)

if __name__ == '__main__': unittest.main()
