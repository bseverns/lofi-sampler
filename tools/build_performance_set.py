#!/usr/bin/env python3
"""Build one immutable 4x8 prepared-sample set for the performance firmware."""
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from build_demo_fs import EXPECTED_RATE, EXPECTED_WIDTH, ROWS, SLICES, _read_wav, _sha256, _write_row

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "firmware/platformio/data"
HEADER = ROOT / "tools/build_bundled_demo_header.py"
PIO = ROOT / "firmware/platformio"


def fail(message):
    raise SystemExit(f"performance set error: {message}")


def copy_pad(source: Path, target: Path):
    _, pcm = _read_wav(source)
    if len(pcm) == 0:
        fail(f"empty pad source: {source}")
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(pcm)
    return {"source": str(source), "source_sha256": _sha256(source),
            "raw_sha256": _sha256(target), "samples": len(pcm) // EXPECTED_WIDTH}


def validate_pattern(pattern):
    if pattern is None:
        return [[{"gate": False, "velocity": 108, "probability": 100} for _ in range(8)] for _ in range(4)]
    if not isinstance(pattern, list) or len(pattern) != 4 or any(not isinstance(row, list) or len(row) != 8 for row in pattern):
        fail("initial_pattern must be a 4x8 array")
    for row in pattern:
        for step in row:
            if not isinstance(step, dict) or not isinstance(step.get("gate"), bool):
                fail("each initial_pattern step needs boolean gate")
            for key, minimum, maximum in (("velocity", 0, 127), ("probability", 0, 100)):
                if not isinstance(step.get(key), int) or not minimum <= step[key] <= maximum:
                    fail(f"each initial_pattern step needs {key} in {minimum}..{maximum}")
    return pattern


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path, help="prepared-set JSON manifest")
    parser.add_argument("--stage-dir", type=Path,
                        help="private output directory (default: prepared-sets/<id>/<version>)")
    parser.add_argument("--header", type=Path,
                        default=ROOT / "firmware/platformio/lib/Adafruit_LittleFS/src/generated/BundledDemoSlices.h")
    parser.add_argument("--receipt", type=Path, help="write resolved receipt JSON here")
    parser.add_argument("--uf2", type=Path, help="build performance firmware and write UF2 here")
    args = parser.parse_args()

    try:
        spec = json.loads(args.manifest.read_text())
    except (OSError, json.JSONDecodeError) as exc:
        fail(f"cannot read manifest: {exc}")
    if not isinstance(spec.get("id"), str) or not spec["id"]:
        fail("manifest needs non-empty id")
    if not isinstance(spec.get("version"), str) or not spec["version"]:
        fail("manifest needs non-empty version")
    pattern = validate_pattern(spec.get("initial_pattern"))
    rows, pads = spec.get("rows"), spec.get("pads")
    if bool(rows) == bool(pads):
        fail("provide exactly one of rows or pads")
    # Validate every input before touching the currently known-good staged set.
    if rows:
        if not isinstance(rows, dict) or set(rows) != set(ROWS):
            fail("rows must map exactly A, B, C, D to WAV paths")
        for row in ROWS:
            source = Path(rows[row]).expanduser()
            if not source.is_file():
                fail(f"missing row source: {source}")
            _read_wav(source)
    else:
        expected = {f"{row}{step}" for row in ROWS for step in range(1, SLICES + 1)}
        if not isinstance(pads, dict) or set(pads) != expected:
            fail("pads must map exactly A1..D8 to WAV paths")
        for pad in expected:
            source = Path(pads[pad]).expanduser()
            if not source.is_file():
                fail(f"missing pad source: {source}")
            _read_wav(source)

    stage = (args.stage_dir or (ROOT / "prepared-sets" / spec["id"] / spec["version"])).resolve()
    header = args.header.resolve()
    stage.parent.mkdir(parents=True, exist_ok=True)
    work_root = Path(tempfile.mkdtemp(prefix=".performance-set-", dir=stage.parent))
    work_stage = work_root / "stage"
    work_header = work_root / "BundledDemoSlices.h"
    metadata = header.with_name("BundledPerformanceSet.h")
    work_metadata = work_root / "BundledPerformanceSet.h"
    assets = {}
    try:
        work_stage.mkdir(parents=True)
        if rows:
            for row in ROWS:
                source = Path(rows[row]).expanduser()
                _, pcm = _read_wav(source)
                hashes, samples = _write_row(row, pcm, work_stage)
                assets[row] = {"source": str(source), "source_sha256": _sha256(source),
                               "samples": samples, "slice_hashes": hashes}
        else:
            for pad in sorted(expected):
                target = work_stage / pad[0] / f"{pad}.raw"
                assets[pad] = copy_pad(Path(pads[pad]).expanduser(), target)

        resolved = {"schema": "lofi-performance-set@1", "id": spec["id"], "version": spec["version"],
                    "initial_pattern": pattern, "assets": assets}
        (work_stage / "performance-set.json").write_text(json.dumps(resolved, indent=2, sort_keys=True) + "\n")
        subprocess.run([sys.executable, str(HEADER), "--data-dir", str(work_stage), "--output", str(work_header)], cwd=ROOT, check=True)
        rows_cpp = ",\n  ".join("{" + ", ".join("{" + ("true" if step["gate"] else "false") + f", {step['velocity']}, {step['probability']}" + "}" for step in row) + "}" for row in pattern)
        work_metadata.write_text("#pragma once\n#include <stdint.h>\nnamespace Ntm4PerformanceSet {\nstruct Step { bool gate; uint8_t velocity; uint8_t probability; };\n"
                                 f"static constexpr char kSetId[] = \"{spec['id']}\";\nstatic constexpr char kSetVersion[] = \"{spec['version']}\";\n"
                                 f"static constexpr Step kInitialPattern[4][8] = {{\n  {rows_cpp}\n}};\n}}\n")

        old_stage = stage.with_name(stage.name + ".previous")
        old_header = header.with_name(header.name + ".previous")
        old_metadata = metadata.with_name(metadata.name + ".previous")
        if old_metadata.exists(): old_metadata.unlink()
        if metadata.exists(): os.replace(metadata, old_metadata)
        if old_stage.exists(): shutil.rmtree(old_stage)
        if old_header.exists(): old_header.unlink()
        if stage.exists(): os.replace(stage, old_stage)
        if header.exists(): os.replace(header, old_header)
        try:
            os.replace(work_stage, stage)
            os.replace(work_header, header)
            os.replace(work_metadata, metadata)
        except Exception:
            if stage.exists(): shutil.rmtree(stage)
            if header.exists(): header.unlink()
            if old_stage.exists(): os.replace(old_stage, stage)
            if old_header.exists(): os.replace(old_header, header)
            if old_metadata.exists(): os.replace(old_metadata, metadata)
            raise
        receipt = args.receipt or stage / "build-receipt.json"
        receipt.parent.mkdir(parents=True, exist_ok=True)
        receipt_data = {**resolved, "header_sha256": _sha256(header), "metadata_sha256": _sha256(metadata)}
        if args.uf2:
            build = subprocess.run(["pio", "run", "-e", "adafruit_trellis_m4_performance"], cwd=PIO, text=True, capture_output=True)
            if build.returncode:
                if stage.exists(): shutil.rmtree(stage)
                if header.exists(): header.unlink()
                if metadata.exists(): metadata.unlink()
                if old_stage.exists(): os.replace(old_stage, stage)
                if old_header.exists(): os.replace(old_header, header)
                if old_metadata.exists(): os.replace(old_metadata, metadata)
                raise SystemExit(build.stdout + build.stderr)
            firmware = PIO / ".pio/build/adafruit_trellis_m4_performance/firmware.bin"
            args.uf2.parent.mkdir(parents=True, exist_ok=True)
            subprocess.run([sys.executable, str(PIO / "scripts/bin_to_uf2.py"), str(firmware), "0x4000", str(args.uf2)], check=True)
            receipt_data.update({"target_environment": "adafruit_trellis_m4_performance",
                                 "firmware_sha256": _sha256(firmware), "uf2_sha256": _sha256(args.uf2),
                                 "firmware_bytes": firmware.stat().st_size, "uf2_bytes": args.uf2.stat().st_size,
                                 "platformio_output": build.stdout + build.stderr})
        receipt.write_text(json.dumps(receipt_data, indent=2, sort_keys=True) + "\n")
        if old_stage.exists(): shutil.rmtree(old_stage)
        if old_header.exists(): old_header.unlink()
        if old_metadata.exists(): old_metadata.unlink()
    finally:
        shutil.rmtree(work_root, ignore_errors=True)
    print(f"Prepared {spec['id']} {spec['version']} with {len(assets)} source assignments")
    print(f"Receipt: {receipt}")

if __name__ == "__main__":
    main()
