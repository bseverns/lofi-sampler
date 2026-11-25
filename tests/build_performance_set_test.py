#!/usr/bin/env python3
import hashlib
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILDER = ROOT / "tools" / "build_performance_set.py"
TEMPLATE = ROOT / "examples" / "performance-set.template.json"
HEADER = ROOT / "firmware/platformio/lib/Adafruit_LittleFS/src/generated/BundledDemoSlices.h"


class PerformanceSetBuilderTest(unittest.TestCase):
    def test_invalid_manifest_preserves_bundled_header(self):
        before = hashlib.sha256(HEADER.read_bytes()).hexdigest()
        result = subprocess.run([sys.executable, str(BUILDER), str(TEMPLATE)],
                                cwd=ROOT, text=True, capture_output=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("missing row source", result.stderr)
        self.assertEqual(before, hashlib.sha256(HEADER.read_bytes()).hexdigest())


if __name__ == "__main__":
    unittest.main()
