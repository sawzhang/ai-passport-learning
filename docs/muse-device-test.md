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
