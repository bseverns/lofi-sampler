#!/usr/bin/env python3
"""Generate the bundled demo-slice header used by the current firmware.

The current known-good playback path ships a read-only demo slice pack compiled
into the firmware. This script regenerates that header from host-side staged RAW
files in `firmware/platformio/data/`.
"""
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = REPO_ROOT / "firmware" / "platformio" / "data"
OUT_PATH = REPO_ROOT / "firmware" / "platformio" / "lib" / "Adafruit_LittleFS" / "src" / "generated" / "BundledDemoSlices.h"


def main() -> int:
    files = sorted(p for p in DATA_DIR.rglob("*.raw") if p.name != "source.raw")
    if not files:
        raise SystemExit(f"No slice raws found under {DATA_DIR}")

    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with OUT_PATH.open("w", encoding="utf-8") as f:
        f.write("#pragma once\n")
        f.write("#include <cstddef>\n")
        f.write("#include <cstdint>\n\n")
        f.write("namespace Adafruit_LittleFS_Namespace {\n")
        f.write("struct BundledReadOnlyFile {\n")
        f.write("  const char* path;\n")
        f.write("  const uint8_t* data;\n")
        f.write("  size_t size;\n")
        f.write("};\n\n")

        for idx, path in enumerate(files):
            symbol = f"bundled_demo_slice_{idx}"
            data = path.read_bytes()
            f.write(f"static const uint8_t {symbol}[] = {{\n")
            for offset in range(0, len(data), 12):
                chunk = data[offset:offset + 12]
                line = ", ".join(f"0x{byte:02X}" for byte in chunk)
                trailer = "," if offset + 12 < len(data) else ""
                f.write(f"  {line}{trailer}\n")
            f.write("};\n\n")

        f.write("static const BundledReadOnlyFile kBundledDemoSlices[] = {\n")
        for idx, path in enumerate(files):
            symbol = f"bundled_demo_slice_{idx}"
            rel_path = "/" + path.relative_to(DATA_DIR).as_posix()
            f.write(f'  {{"{rel_path}", {symbol}, sizeof({symbol})}},\n')
        f.write("};\n\n")
        f.write("static constexpr size_t kBundledDemoSliceCount = sizeof(kBundledDemoSlices) / sizeof(kBundledDemoSlices[0]);\n")
        f.write("}  // namespace Adafruit_LittleFS_Namespace\n")

    print(f"Wrote {OUT_PATH}")
    print(f"Bundled {len(files)} slice files from {DATA_DIR}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
