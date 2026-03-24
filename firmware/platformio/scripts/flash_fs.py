Import("env")
from pathlib import Path
import subprocess

ENV_NAME = env.subst("$PIOENV") or "adafruit_trellis_m4"
BUILD_DIR = Path(env.subst("$BUILD_DIR"))
PROJECT_DIR = Path(env.subst("$PROJECT_DIR"))
LIBDEPS_DIR = PROJECT_DIR / ".pio" / "libdeps" / ENV_NAME


def _as_bool(value):
    return str(value).strip().lower() in ("1", "true", "yes", "on")


AUTO_FS_UPLOAD = _as_bool(env.GetProjectOption("custom_auto_fs_upload", "false"))


def _patch_file(path, replacements):
    if not path.exists():
        return

    text = path.read_text()
    updated = text
    for old, new in replacements:
        updated = updated.replace(old, new)

    if updated != text:
        path.write_text(updated)


def _patch_dependency_warnings():
    neopixel_dma = LIBDEPS_DIR / "Adafruit DMA neopixel library" / "Adafruit_NeoPixel_ZeroDMA.cpp"
    _patch_file(
        neopixel_dma,
        [
            (
                "    : Adafruit_NeoPixel(n, p, t), brightness(256), dmaBuf(NULL), spi(NULL) {}\n",
                "    : Adafruit_NeoPixel(n, p, t), spi(NULL), dmaBuf(NULL), brightness(256) {}\n",
            ),
            (
                "    : Adafruit_NeoPixel(), brightness(256), dmaBuf(NULL), spi(NULL) {}\n",
                "    : Adafruit_NeoPixel(), spi(NULL), dmaBuf(NULL), brightness(256) {}\n",
            ),
            ("      int i;\n\n", ""),
        ],
    )

    trellis_m4 = LIBDEPS_DIR / "Adafruit NeoTrellis M4 Library" / "Adafruit_NeoTrellisM4.cpp"
    _patch_file(
        trellis_m4,
        [
            (
                "    midiEventPacket_t noteOn = {0x09, 0x90 | _midi_channel_usb, pitch,\n",
                "    midiEventPacket_t noteOn = {0x09, static_cast<uint8_t>(0x90 | _midi_channel_usb), pitch,\n",
            ),
            (
                "    midiEventPacket_t noteOff = {0x08, 0x80 | _midi_channel_usb, pitch,\n",
                "    midiEventPacket_t noteOff = {0x08, static_cast<uint8_t>(0x80 | _midi_channel_usb), pitch,\n",
            ),
            (
                "    midiEventPacket_t pitchBend = {0x0E, 0xE0 | _midi_channel_usb, lowValue,\n",
                "    midiEventPacket_t pitchBend = {0x0E, static_cast<uint8_t>(0xE0 | _midi_channel_usb), lowValue,\n",
            ),
            (
                "    midiEventPacket_t event = {0x0B, 0xB0 | _midi_channel_usb, control, value};\n",
                "    midiEventPacket_t event = {0x0B, static_cast<uint8_t>(0xB0 | _midi_channel_usb), control, value};\n",
            ),
            (
                "  midiEventPacket_t pc = {0x0C, 0xC0 | channel, program, 0};\n",
                "  midiEventPacket_t pc = {0x0C, static_cast<uint8_t>(0xC0 | channel), program, 0};\n",
            ),
        ],
    )


_patch_dependency_warnings()


def _run_target(target, description):
    cmd = [
        env.subst("$PYTHONEXE"),
        "-m",
        "platformio",
        "run",
        "-e",
        ENV_NAME,
        "-t",
        target,
    ]
    print(description)
    result = subprocess.run(cmd, cwd=PROJECT_DIR)
    if result.returncode != 0:
        print(f"[littlefs] Skipping optional target '{target}' (exit code {result.returncode}).")
    return None


def _buildfs(source, target, env):
    return _run_target("buildfs", "Building LittleFS image")


def _uploadfs(source, target, env):
    bin_path = BUILD_DIR / "littlefs.bin"
    if not bin_path.exists():
        print("[littlefs] Skipping upload; littlefs.bin not found (run tools/build_demo_fs.py?)")
        return None
    return _run_target("uploadfs", "Flashing LittleFS demo set")


if AUTO_FS_UPLOAD:
    env.AddPreAction("upload", _buildfs)
    env.AddPostAction("upload", _uploadfs)
else:
    print("[littlefs] Auto FS upload disabled. Set custom_auto_fs_upload = true to enable.")
