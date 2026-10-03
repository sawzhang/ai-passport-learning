[简体中文](test-report.zh_CN.md) · **English**

# Test report

Validation date: 2026-10-03. Tests below identify their evidence type; a pass in one row does not imply the others.

| Layer | Result / evidence | Scope |
| --- | --- | --- |
| Host Tetris | PASS: sanitizers, one million random actions | Bounds, collision, pause, score, four-line clear |
| Host vocabulary | PASS: 12,600 randomized rounds and rule tests | Round transitions, score, mistakes and review |
| Firmware gate | PASS on ESP-IDF 5.5.3 | Compile/link, image/partition geometry, matching ELF and merged-image archive |
| Diagnostic ESP32 | PASS: 140 cycles / 601 seconds, zero failures | Real UI/game code and synthetic queue events, heap, warm-restart NVS |
| Human device acceptance | User reported both games OK | Physical interaction; no itemized measurements supplied |
| Cold power-cycle, ADC voltage, pixel capture, battery duration | Not independently measured | Synthetic diagnostic does not establish these properties |

The diagnostic exercised navigation, move/rotate/pause/drop, fixture-based line clear and wall collision, game-over/restart, ten-question vocabulary rounds and corrected mistake removal. Each cycle entered/exited both games. Sampled free heap stayed at 238,656 bytes. Highest-score and mistake records survived `esp_restart`. That is a warm restart, not proof of a cold power cycle.

Sanitized terminal evidence:

```text
SELFTEST boot stage=0 best=0 mistake1=0
SELFTEST boot stage=1 best=1234 mistake1=1
SELFTEST persistence failures=0; starting 600-second UI/game stress
SELFTEST cycles=140 elapsed=601s heap=238656 failures=0
SELFTEST RESULT PASS cycles=140 heap=238656 failures=0
```

## Build identity

| Image | SHA-256 | Merged size |
| --- | --- | --- |
| Diagnostic tested | `a76f0084b2d6b63781cb6857ad1fef4f99b6737c40e50cb2ccf8b7eecbe1e714` | 726,368 bytes |
| Normal games restored | `3cbd047bc0a6dbb0f28201c5d5c6272f8fc9ba4c5bfde8c62b6b4af60c451088` | 721,744 bytes |

Normal application size: 656,208 bytes. Normal matching ELF SHA-256: `cad5f6b7ba40883d4f99e6bc363a65d7a7fbaef4f5224b5dc2d56cba90e00b5d`. Hardware: ESP32-C3 revision 1.1, 8 MB Flash. Factory app starts at 0x10000; merged image is written at 0x0. No full-chip erase was used. Opening USB serial can reset this board; an observed USB reset alone is not a firmware crash.

The diagnostic alters NVS test records. Ordinary firmware was restored after testing, so diagnostic scores are not presented as the user's saved progress. Rebuilds may have different hashes because paths and build metadata affect the binary; use the archive generated with your own build.

## Reproduce

Run `./tools/test.sh` and `./tools/build.sh`. Ordinary builds disable device diagnostics. To run diagnostics intentionally, activate ESP-IDF 5.5.3, enter `firmware`, set `PASSPORT_DEVICE_SELF_TEST=1` for `./tools/validate.sh --firmware`, then use the verified flash helper. Capture at least 660 seconds and inspect the final result. Restore an independently archived normal image afterwards.

The diagnostics use software fixtures and input events, not electrical button actuation or visual recognition. Muse SDK ports, catalog plays, wireless multiplayer and voice services were researched only. They are not covered by these passes.
