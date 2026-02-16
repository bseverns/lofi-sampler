Import("env")
from pathlib import Path
import subprocess

ENV_NAME = env.subst("$PIOENV") or "adafruit_trellis_m4"
BUILD_DIR = Path(env.subst("$BUILD_DIR"))
PROJECT_DIR = Path(env.subst("$PROJECT_DIR"))


def _as_bool(value):
    return str(value).strip().lower() in ("1", "true", "yes", "on")


AUTO_FS_UPLOAD = _as_bool(env.GetProjectOption("custom_auto_fs_upload", "false"))


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
