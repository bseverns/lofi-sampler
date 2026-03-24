#!/usr/bin/env python3

import math
import struct
import sys
from pathlib import Path

MAGIC_START0 = 0x0A324655
MAGIC_START1 = 0x9E5D5157
MAGIC_END = 0x0AB16F30
FLAG_FAMILY_ID_PRESENT = 0x00002000
UF2_PAYLOAD_SIZE = 256
UF2_BLOCK_SIZE = 512
SAMD51_FAMILY_ID = 0x55114460


def main() -> int:
    if len(sys.argv) >= 2 and sys.argv[1] == "--extract":
        return extract_mode(sys.argv[2:])

    if len(sys.argv) != 4:
        print("usage: bin_to_uf2.py <input.bin> <start_addr> <output.uf2>", file=sys.stderr)
        print("   or: bin_to_uf2.py --extract <input.uf2> <output.bin>", file=sys.stderr)
        return 2

    input_path = Path(sys.argv[1])
    start_addr = int(sys.argv[2], 0)
    output_path = Path(sys.argv[3])

    payload = input_path.read_bytes()
    num_blocks = max(1, math.ceil(len(payload) / UF2_PAYLOAD_SIZE))
    blocks = bytearray()

    for block_no in range(num_blocks):
        chunk = payload[block_no * UF2_PAYLOAD_SIZE : (block_no + 1) * UF2_PAYLOAD_SIZE]
        chunk = chunk.ljust(UF2_PAYLOAD_SIZE, b"\x00")
        target_addr = start_addr + block_no * UF2_PAYLOAD_SIZE

        header = struct.pack(
            "<IIIIIIII",
            MAGIC_START0,
            MAGIC_START1,
            FLAG_FAMILY_ID_PRESENT,
            target_addr,
            UF2_PAYLOAD_SIZE,
            block_no,
            num_blocks,
            SAMD51_FAMILY_ID,
        )
        block = header + chunk + bytes(UF2_BLOCK_SIZE - len(header) - len(chunk) - 4) + struct.pack("<I", MAGIC_END)
        blocks.extend(block)

    output_path.write_bytes(blocks)
    print(f"Wrote {output_path} ({num_blocks} UF2 blocks)")
    return 0


def extract_mode(args) -> int:
    if len(args) != 2:
        print("usage: bin_to_uf2.py --extract <input.uf2> <output.bin>", file=sys.stderr)
        return 2

    input_path = Path(args[0])
    output_path = Path(args[1])
    data = input_path.read_bytes()
    if len(data) % UF2_BLOCK_SIZE != 0:
        print("input is not a whole number of UF2 blocks", file=sys.stderr)
        return 1

    chunks = []
    for offset in range(0, len(data), UF2_BLOCK_SIZE):
        block = data[offset : offset + UF2_BLOCK_SIZE]
        header = struct.unpack("<IIIIIIII", block[:32])
        if header[0] != MAGIC_START0 or header[1] != MAGIC_START1 or struct.unpack("<I", block[-4:])[0] != MAGIC_END:
            print("invalid UF2 block magic", file=sys.stderr)
            return 1
        target_addr = header[3]
        payload_size = header[4]
        chunks.append((target_addr, block[32 : 32 + payload_size]))

    chunks.sort(key=lambda item: item[0])
    out = bytearray()
    expected_addr = chunks[0][0]
    for addr, payload in chunks:
        if addr != expected_addr:
            print("non-contiguous UF2 payload, refusing extract", file=sys.stderr)
            return 1
        out.extend(payload)
        expected_addr += len(payload)

    output_path.write_bytes(out)
    print(f"Extracted {output_path} ({len(out)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
