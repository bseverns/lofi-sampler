# Rehearsal Candidate Acceptance Ledger

Updated against `39de71d`. “Software-tested” never means hardware-verified.

| Criterion | Status | Evidence / next proof |
| --- | --- | --- |
| Custom header uses requested staging directory | **Software-tested** | `tools/build_bundled_demo_header.py --data-dir`; temporary-stage command generated 32 entries. |
| Preparation is transactional | **Outstanding** | `build_performance_set.py` still deletes its destination before publication. |
| Existing assets survive failed generation | **Partial** | Inputs validate before deletion, but header/build failures are not rolled back. |
| Private prepared assets cannot be committed | **Partial** | `.gitignore` ignores `prepared-sets/` and `releases/`; builder’s default output is still tracked firmware staging. |
| Initial pattern, set ID, version are firmware-readable | **Outstanding** | Manifest stores them, firmware does not consume them. |
| Length, nonempty, PCM, flash-budget validation | **Partial** | PCM/nonempty checks exist for explicit pads; no actual image/flash budget gate. |
| Builder regressions in CI | **Outstanding** | No isolated builder test exists yet. |
| Physical dispatch sequences | **Partial** | `tests/pad_input_test.cpp` covers modifier lifecycle, not the extracted production dispatch path. |
| Runtime diagnostics / behavioral queue test | **Outstanding** | Queue test remains structural; counters are absent. |
| Test-set UF2 + traceable receipt | **Outstanding** | No receipt binds firmware/UF2 hashes or resource usage. |
| Physical rehearsal matrix | **Physically unverified** | Follow `docs/performance-profile.md`; no board run claimed. |
