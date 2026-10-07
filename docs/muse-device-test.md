[简体中文](muse-device-test.zh_CN.md) · **English**

# Muse × AI Passport device test record

## 2026-10-07: supervised Mac proxy

The temporary 18087 bridge was absent while Wi-Fi and the upstream 1087 proxy remained available. Restoring it and resetting the device restored Link. The original process exit cause was not established.

The bridge now isolates connection failures, bounds cleanup and uses bidirectional idle tracking. A user LaunchAgent starts at login and restarts exited processes with a 10-second throttle; runtime code lives outside the checkout, and metadata logs rotate at 1 MiB with three backups. Eight host tests passed, including upstream outage/recovery and one-way activity. A SIGKILL fault injection replaced PID 96637 with 96689 and restored `0.0.0.0:18087`; three subsequent device status reads were Online. A subsequent physical microphone upload returned 200 and a cloud reply after 9.48 seconds, confirming traffic through the recovered bridge. Muse reported an unclear transcript, so speech recognition accuracy did not pass this round. No firmware was rebuilt or flashed. Sleep, logout, unavailable upstream and a missing Python runtime remain outside this recovery guarantee.

## 2026-10-06: Chinese reply rendering fix

The user reported unreadable Chinese on the board. The original reply label used Unscii without Chinese glyphs, counted Han characters as one Latin column and reserved compact caption height for an 8-pixel font. Replies/transcripts now use Noto Sans SC 16 px as a fallback to the monospaced Latin font, two-column CJK pagination and sufficient line height. Bounded formatting and transcript tails trim incomplete trailing UTF-8 characters.

The Flash-resident uncompressed 2 bpp asset contains 21,136 glyphs, including every one of the 6,763 GB2312 Han characters; it does not claim all Unicode. See [font provenance, OFL license and reproduction](../experiments/muse-passport/overlay/esp32/components/muse/fonts/README.md). The 3,805,184-byte app fits the 4,063,232-byte slot with 258,048 bytes remaining. This fixes reply/transcript captions, not localization of every settings widget.

Build: PASS, application SHA-256 `7eba5c2a5d964d334518db16abd9b5867508bf432adfe1cfc36e8b9e6b006423`. Host tests: 173 total, 171 passed, two environment-dependent skips; new checks cover GB2312 glyphs, mixed-width pagination and UTF-8 boundaries. Device tests: flash hash verified, configuration preserved, Online restored, active-label glyph lookup passed, and a microphone request returned 200 plus a Chinese reply after 28.06s. A complete 240×320 capture of actual LCD flush pixels shows readable Chinese/Latin in two lines without missing-glyph boxes or clipped Han characters in that reply. All six adapter host tests passed, including complete-frame validation. A second microphone turn returned 200 and a reply after 7.27s; a second LCD capture clearly shows “好的，显示正常就好。” Both replies continued the existing learning-plan context rather than strictly echoing the short test prompt. No crash or allocation failure appeared in the two test logs. Unverified: physical panel inspection remains separate. TTS remains deferred at the user's request.


## 2026-10-06: Mac proxy and microphone-to-Muse acceptance

Current application SHA-256: `3b9ab20a568d8659643152a12eb31979733b392ee8ebd6e7e804687cae1d31af`. ESP-IDF 6.0.1 build and image verification passed; app-only flashing at `0x20000` passed the device hash check and preserved pairing, Wi-Fi and proxy settings.

The Mac bridge listens on `0.0.0.0:18087` and forwards to the existing HTTP proxy on `127.0.0.1:1087`. The board establishes CONNECT tunnels, validates TLS certificates and receives a successful Link registration plus heartbeat. Fixes cover early paired-boot workspace reservation, bounded voice queue memory and registration-prefix corruption when service/WebSocket output buffers share storage. The actual-codec regression reproduced that corruption before the fix and passes afterward.

Two acoustic tests used Mac speech through the physical board microphone, with USB simulating talk-button down/up. Muse accepted both uploads (200), delivered related user-message events and returned text after 6.66s and 13.45s. The new task “请写一份三天的机器学习计划” received a corresponding three-day plan. This profile returns text, not spoken Muse replies.

The latest image also recovered automatically after the Mac proxy was stopped and restarted: Wi-Fi stayed connected, Link transitioned Offline to Online, registration and heartbeat succeeded again, and no crash was observed. A continuous ten-minute online soak and physical key/display/speaker acceptance on this image have not been performed.

Host suite: 171 tests, 169 passed, two environment-dependent skips; real-crypto pairing and Noise-core/16 KiB in-place AES-GCM tests were then run separately and passed, including tampered-tag rejection. Four proxy tests and repository regression passed. Full current results and acceptance limits are in [test-evidence.json](../experiments/muse-passport/test-evidence.json); setup is in the [proxy guide](../experiments/muse-passport/proxy-guide.md).

The user subsequently supplied a Muse app screenshot and explicitly confirmed receipt of both the voice message and the reply. This verifies app receipt, without inferring physical board display, key or speaker acceptance. The screenshot's collapsed Needs approval item does not identify its underlying action.

Lessons: the early 502 later cleared, but that observation alone does not establish its cause. Layered diagnostics and tests isolated memory and send-pressure failures. Buffer reuse temporarily introduced registration-prefix corruption; a real-codec regression reproduced it before the fix. Future memory-layout changes must validate encoded bytes, registration ACK and a real voice reply. Keep historical failures, verified behavior and pending checks distinct; upload 200 is not task completion.

Deliverables include firmware patch/overlay, Mac bridge and configuration tools, regression tests, CI, bilingual setup instructions and evidence. Credentials and binaries remain in ignored private directories. Continue with the [learning plan](learning-guide.md#learning-plan-muse-proxy-and-voice-integration).

The sections below retain earlier firmware results and pending checks as historical evidence; they do not describe the current cloud connection.

## Historical results

2026-10-04. Physical AI Passport: ESP32-C3 rev 1.1, 8MB Flash, no PSRAM. Pinned Muse SDK `693cde9a884ad1edc87251b9f8944815f8de4809`, experimental adapter, official ESP-IDF **6.0.1**.

As of 2026-10-05, **phone pairing and Wi-Fi connectivity succeeded**: live serial status reports `paired=true`, `wifi.state=connected` and RSSI −35 dBm. Muse Link remains `Connecting`; cloud and end-to-end voice acceptance are incomplete. The user reports restricted network access on the current Wi-Fi. That explanation still needs a comparison network and DNS/TLS diagnostics. Developer mode is enabled.

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
| Phone / Wi-Fi / cloud | Live status on 2026-10-05: paired=true, Wi-Fi connected, RSSI −35 dBm. Muse Link Connecting; cloud acceptance pending. |
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

That revision app SHA-256: `2a52f002666c05f14575b733a2d622c679c2d736b5ae42eda0847cbf68039e09`. **A fresh iPhone provisioning attempt on this build is pending; end-to-end Wi-Fi success is not yet established.** The 600-second evidence above applies to the preceding build. TLS and cloud voice remain pending.

## 2026-10-05: provisioning received, worker startup failed

A continuous serial capture confirms the iPhone completed the encrypted handshake and physical OK confirmation. A 2792-byte message was reassembled and reached `provision_v2`, followed by `error_operation_in_progress` before Wi-Fi association. The first receive-buffer fix works; the phone did submit provisioning data.

This error path covers both session validity and allocation of an 8 KB worker stack. Peak memory while temporary JSON, plaintext and GATT buffers coexist is the leading explanation, but the old log alone does not prove allocation failure. The new fix starts the worker after those buffers are freed, preserves session checks and failure cleanup, and logs free memory and the largest contiguous block.

The firmware was built, image-validated and flashed; all four written segments passed hash verification. Application SHA-256: `52fef5412c720b4a8276865690831493b3c993a8536d5d9644e37415e175f80d`. At that checkpoint the device was advertising and awaiting a phone retry; see the later successful status below.

New tests compile the actual C functions to verify buffer release before task creation, stale-session rejection and task-creation failure cleanup. The existing security contract test was updated for the deferred handoff. Full host suite: 159 tests, 158 passed, one skipped for missing host PSA headers, 63.675 seconds. Both memory regression tests passed after preparing the adapter from a clean upstream checkout.

Wi-Fi/DHCP, cloud connectivity and end-to-end voice acceptance remain pending. The previously recorded 600-second stability run used older firmware and is not full acceptance evidence for this build.

## 2026-10-05: pairing and Wi-Fi succeeded; cloud remains blocked

A live serial `>status` query without resetting the board returned `device.wifi.on=true`, `device.wifi.state=connected`, `device.wifi.rssi=-35`, `device.link.paired=true`, `device.link.state=Connecting` and `device.last=ready`. This confirms pairing and local Wi-Fi success after the latest fix, but does not prove cloud handshake success or capture the complete DHCP sequence. The earlier continuous capture had stopped; its old advertising heartbeats are not current status.

The user then reported that the current Wi-Fi cannot reach Muse because of restricted network access. This is user-provided environment information; the observed device fact is the Connecting state. No alternative-network comparison or independent DNS, TCP, TLS or service-authentication diagnosis has been completed, so other connection problems remain possible.

Next acceptance: use a 2.4GHz network confirmed to provide this device access to Muse, capture connection establishment, then verify a cloud session, ten speech-input/text-reply turns, ten minutes online, network recovery and restart persistence. Until then this remains an experimental adapter without full end-to-end acceptance.

Both GitHub Actions workflows for fix commit `9fa33c9` passed: Validate Muse Passport adapter and Validate learning lab. The flashed application SHA-256 remains `52fef5412c720b4a8276865690831493b3c993a8536d5d9644e37415e175f80d`. Only sanitized status is published; raw logs and credentials remain private.
