Import("env")
from pathlib import Path

ENV_NAME = env.subst("$PIOENV") or "adafruit_trellis_m4"
BUILD_DIR = Path(env.subst("$BUILD_DIR"))


def _verbose(cmd, message):
    return env.VerboseAction(cmd, message)


def _buildfs(source, target, env):
    cmd = f"pio -e {ENV_NAME} -t buildfs"
    return env.Execute(_verbose(cmd, "Building LittleFS image"))


def _uploadfs(source, target, env):
    bin_path = BUILD_DIR / "littlefs.bin"
    if not bin_path.exists():
        print("[littlefs] Skipping upload; littlefs.bin not found (run tools/build_demo_fs.py?)")
        return None
    cmd = f"pio -e {ENV_NAME} -t uploadfs"
    return env.Execute(_verbose(cmd, "Flashing LittleFS demo set"))


env.AddPreAction("upload", _buildfs)
env.AddPostAction("upload", _uploadfs)
