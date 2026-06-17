#!/usr/bin/env python3
"""Validate host-visible v0.1 filesystem/sample path assumptions."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROWS = ("A", "B", "C", "D")
SLICES = 8

ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = ROOT / "firmware" / "platformio" / "data"
BUNDLED_HEADER = (
    ROOT
    / "firmware"
    / "platformio"
    / "lib"
    / "Adafruit_LittleFS"
    / "src"
    / "generated"
    / "BundledDemoSlices.h"
)
CONTRACT_DOC = ROOT / "docs" / "filesystem-contract.md"


def expected_slice_paths() -> list[str]:
    return [f"/{row}/{row}{idx}.raw" for row in ROWS for idx in range(1, SLICES + 1)]


def fail(message: str) -> None:
    print(f"[filesystem-contract] FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def require_even_nonzero_raw(path: Path) -> int:
    if not path.exists():
        fail(f"missing {path.relative_to(ROOT)}")
    size = path.stat().st_size
    if size == 0:
        fail(f"{path.relative_to(ROOT)} is empty")
    if size % 2 != 0:
        fail(f"{path.relative_to(ROOT)} is not 16-bit aligned")
    return size


def validate_staged_data(expected: list[str]) -> None:
    actual_raws = sorted(p for p in DATA_DIR.rglob("*.raw") if p.is_file())
    actual_rel = sorted("/" + p.relative_to(DATA_DIR).as_posix() for p in actual_raws)
    expected_sources = [f"/{row}/source.raw" for row in ROWS]
    allowed = sorted(expected + expected_sources)
    extras = sorted(set(actual_rel) - set(allowed))
    missing = sorted(set(allowed) - set(actual_rel))
    if missing:
        fail("staged data missing: " + ", ".join(missing))
    if extras:
        fail("staged data has non-contract raw files: " + ", ".join(extras))

    for row in ROWS:
        source_size = require_even_nonzero_raw(DATA_DIR / row / "source.raw")
        slice_sizes = [
            require_even_nonzero_raw(DATA_DIR / row / f"{row}{idx}.raw")
            for idx in range(1, SLICES + 1)
        ]
        if sum(slice_sizes) != source_size:
            fail(f"/{row}/ slice bytes do not add up to source.raw")
        if max(slice_sizes) - min(slice_sizes) > 8:
            fail(f"/{row}/ slices are not evenly sized within expected remainder")


def validate_bundled_header(expected: list[str]) -> None:
    if not BUNDLED_HEADER.exists():
        fail(f"missing {BUNDLED_HEADER.relative_to(ROOT)}")
    text = BUNDLED_HEADER.read_text(encoding="utf-8")
    entries = re.findall(r'\{\s*"([^"]+\.raw)"\s*,', text)
    if sorted(entries) != sorted(expected):
        missing = sorted(set(expected) - set(entries))
        extra = sorted(set(entries) - set(expected))
        details = []
        if missing:
            details.append("missing header entries: " + ", ".join(missing))
        if extra:
            details.append("unexpected header entries: " + ", ".join(extra))
        fail("; ".join(details) or "bundled header paths do not match contract")

    arrays = re.findall(r"static const uint8_t bundled_demo_slice_(\d+)\[\]", text)
    if len(arrays) != len(expected):
        fail(f"expected {len(expected)} bundled arrays, found {len(arrays)}")
    if "source.raw" in text:
        fail("bundled header must not include source.raw")
    if "/factory/" in text or "manifest.json" in text:
        fail("bundled header must not include factory or manifest paths")


def validate_contract_doc() -> None:
    if not CONTRACT_DOC.exists():
        fail(f"missing {CONTRACT_DOC.relative_to(ROOT)}")
    text = CONTRACT_DOC.read_text(encoding="utf-8")
    required_phrases = [
        "Freshly flashed firmware must make sound without a separate filesystem upload",
        "`/manifest.json` is a legacy/internal diagnostic artifact",
        "`/factory/*` restore is experimental",
    ]
    for phrase in required_phrases:
        if phrase not in text:
            fail(f"contract doc missing phrase: {phrase}")


def main() -> int:
    expected = expected_slice_paths()
    validate_staged_data(expected)
    validate_bundled_header(expected)
    validate_contract_doc()
    print("[filesystem-contract] OK")
    print(f"[filesystem-contract] staged rows: {len(ROWS)}")
    print(f"[filesystem-contract] playable slice paths: {len(expected)}")
    print("[filesystem-contract] bundled header excludes source/manifest/factory paths")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
