[简体中文](muse-device-test.zh_CN.md) · **English**

# Muse × AI Passport device test record

2026-10-04. Physical AI Passport: ESP32-C3 rev 1.1, 8MB Flash, no PSRAM. Pinned Muse SDK `693cde9a884ad1edc87251b9f8944815f8de4809`, experimental adapter, official ESP-IDF **6.0.1**.

The firmware was built, flashed and booted. Display, ADC input and ES8311 initialization succeeded; BLE advertises for pairing. **End-to-end acceptance is incomplete**: phone pairing, provisioning, cloud session and real voice replies require further evidence. The user confirmed Developer mode is enabled.

| Check | Evidence / status |
| --- | --- |
| Host suite | 157 tests: 156 passed, 1 skipped, 68.339s. NoiseCore PSA compilation skipped because system PSA Crypto development headers/libraries are missing. The real-crypto pairing state-machine test separately passed. |
| ADC logic | Actual C helper compiled with strict cc warnings; assertions cover boundaries, debounce, invalid samples, press/release, menu navigation and long back. Physical keys remain unverified. |
| Image | C3 image headers, 8MB configuration, partition MD5, all bounds and private token match passed. |
| Flash | All four segments passed device hash verification; no whole-chip erase or eFuse writes. |
| Boot | Muse Gadget starting, correct board, 240×320 UI and ready state. Final stability statistics will follow. |
| Battery | CW2017 initialized; status returned 99%. Precision, charger detection and endurance untested. |
| Audio driver | ES8311 ready, 16kHz stereo. Mic L/R -54.9dBFS, peak 231, correlation 1.00; capture 15817Hz, playback 15920Hz. Playback self-test writes silence and does not validate audible sound. |
| Serial | Valid status/power JSON, serial typed chat unavailable. One z/w sleep/wake cycle logged successfully without errors. |
| Phone / Wi-Fi / cloud | Pending. Initial paired=false, wifi=no_network. User phone actions are needed; serial output cannot substitute for them. |
| Visual / physical keys / voice answer | Pending physical acceptance. |
| Persistence / reconnection | Pending after pairing and joining the network. |
| Long run | 600s capture completed, 120 heartbeats, one boot, zero panic/error logs. Unpaired state only; does not validate online stability. |

App **2,232,320 bytes**, slot **4,063,232 bytes**, about **45% free**. Flashed app SHA-256: `648ea4085d3c6d045b0dfbfb5f139429ba7d4469d713c22112ee54eb0715e230`. Firmware contains a private SDK token; binaries and raw logs are not published.

The initial build rebooted after UI allocation left 2048 bytes and input task creation failed. A second build booted but failed I2S RX DMA allocation. The final profile uses compact UI, an eight-line single display buffer, no optional 8KB dirty-cell cache, smaller radio pools, 80ms pre-roll and 3×120 DMA frames per direction. Free internal heap: 40412 bytes after UI, 12984 after audio startup; after Wi-Fi scanning, heartbeats show roughly 25–26KB total and 7–8KB largest block. **These are unpaired figures, not evidence of TLS/upload stability.**

After phone pairing and 2.4GHz provisioning, validate cloud session establishment; ten push-to-talk turns and short/silent recordings; all three keys and ten menu cycles; ten minutes online, network recovery and restart persistence; real display and audible audio quality. Hold OK to speak, release to submit. Replies are text on this no-PSRAM profile.

See [source and reproduction](../experiments/muse-passport/README.md). Tunnel, downloaded images, independent Hatch TLS and OTA are disabled. Charger/USB detection is unvalidated; the bench reports USB present. Software power-off is unsupported. The private games recovery archive remains available. Tokens, avatar assets and signing keys are excluded from publication.

## 600-second unpaired run

Capture lasted 600s; heartbeats span 0–596s. Free internal heap 25–30KB; largest block 7–17KB, rounded by the log. One boot, zero panics and zero E-level logs. One sleep/wake cycle. Still unpaired/unprovisioned at completion. See [machine-readable evidence](../experiments/muse-passport/test-evidence.json).

## Follow-up: iPhone reports Couldn't connect to device

The captured retry completed BLE connection, MTU 256, encrypted pairing, physical OK confirmation and Wi-Fi scanning. Twelve provisioning fragments arrived (11×250+37=2787 bytes), but no RX reassembled/provision_v2 event followed: router association had not begun. The receiver always allocated 8192 contiguous bytes while the observed largest block was about 7KB, consistent with its silent allocation-failure path.

The fix sizes initial allocation to the message (3000 bytes here), grows for variable fragments within the unchanged 8192-byte limit, and logs allocation failures. Passport avatar decoding uses four-row strips instead of sixteen. A regression harness executes the actual receiver with a 4KB allocation ceiling, the iPhone packet sequence, varying fragment sizes, invalid ordering, allocation failures, recovery and size boundaries. Host suite: 158 tests, 157 passed, the same PSA development-library test skipped, 65.455s. Build, image verification, four flash hashes and audio startup passed. Free heap: 46364 bytes after UI, 19420 after audio; largest idle block about 11KB.

Current flashed app SHA-256: `2a52f002666c05f14575b733a2d622c679c2d736b5ae42eda0847cbf68039e09`. **A fresh iPhone provisioning attempt on this build is pending; end-to-end Wi-Fi success is not yet established.** The 600-second evidence above applies to the preceding build. TLS and cloud voice remain pending.

## 2026-10-05: provisioning received, worker startup failed

A continuous serial capture confirms the iPhone completed the encrypted handshake and physical OK confirmation. A 2792-byte message was reassembled and reached `provision_v2`, followed by `error_operation_in_progress` before Wi-Fi association. The first receive-buffer fix works; the phone did submit provisioning data.

This error path covers both session validity and allocation of an 8 KB worker stack. Peak memory while temporary JSON, plaintext and GATT buffers coexist is the leading explanation, but the old log alone does not prove allocation failure. The new fix starts the worker after those buffers are freed, preserves session checks and failure cleanup, and logs free memory and the largest contiguous block.

The firmware was built, image-validated and flashed; all four written segments passed hash verification. Application SHA-256: `52fef5412c720b4a8276865690831493b3c993a8536d5d9644e37415e175f80d`. The device is running and advertising; a fresh phone provisioning attempt is pending.

New tests compile the actual C functions to verify buffer release before task creation, stale-session rejection and task-creation failure cleanup. The existing security contract test was updated for the deferred handoff. Full host suite: 159 tests, 158 passed, one skipped for missing host PSA headers, 63.675 seconds. Both memory regression tests passed after preparing the adapter from a clean upstream checkout.

Wi-Fi/DHCP, cloud connectivity and end-to-end voice acceptance remain pending. The previously recorded 600-second stability run used older firmware and is not full acceptance evidence for this build.
