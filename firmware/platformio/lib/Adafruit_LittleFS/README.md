# Adafruit LittleFS (vendored, offline-safe)

We want this firmware to build even when the internet is a brick wall. The usual PlatformIO
`lib_deps` entry would try to pull `Adafruit_LittleFS` from the registry, so this directory
houses a local, API-compatible shim instead. Drop in the real upstream code whenever you
regain connectivity; until then this keeps the compilation train rolling.

## What lives here

- `src/Adafruit_LittleFS.*` — a lightweight, in-memory stand-in that mirrors the upstream API
  surface. It leans on C++ containers to keep file contents alive while the MCU is powered.
- `library.properties` — minimal metadata so PlatformIO treats this as a bona fide library.

## Caveats (because punk rock also means honest)

- This is **not** a flash-backed LittleFS implementation. It is intentionally simple so we can
  ship code and unblock local builds without external downloads.
- On real hardware you probably want the official library for persistence and wear leveling.

## Upgrading to the real deal

1. Remove this folder.
2. Clone or unzip the actual [`Adafruit_LittleFS`](https://github.com/adafruit/Adafruit_LittleFS)
   release here: `firmware/platformio/lib/Adafruit_LittleFS/`.
3. Keep the folder name the same so the include paths continue to resolve.

That’s it; PlatformIO will pick the local copy first, so builds remain deterministic and offline-friendly.
