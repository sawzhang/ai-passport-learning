[简体中文](learning-guide.zh_CN.md) · **English**

# Learning path

Start with the terminal demo, then follow the pure C models into the LVGL device UI. Read `firmware/main/games_main.c`, the game models and `firmware/components/bsp/include/bsp_pins.h` together. The model owns rules; the UI owns screens and input; BSP owns hardware.

1. Run `./tools/test.sh` and `./demo-tetris/run.sh`.
2. Inspect the word bank and ten-question state transitions before changing content.
3. Build with `./tools/build.sh`; the supported game baseline is ESP-IDF 5.5.3.
4. Select the USB port explicitly and use the verified merged image helper.
5. Check boot logs, then physically inspect display and button behavior.

Hold OK for at least 0.5 seconds to leave either game. Keep diagnostic builds separate: `PASSPORT_DEVICE_SELF_TEST=1` enables synthetic input and writes test progress into NVS. The root build helper disables this flag for ordinary play.

See [test evidence](test-report.md), [capabilities](capabilities.md) and [sources](sources.md).

## Learning plan: Muse proxy and voice integration

Goal: explain and reproduce the path from the physical microphone through HTTP CONNECT, TLS and Link to Muse and a text reply on the board. Plan five 60–90 minute sessions. Keep the game ESP-IDF 5.5.3 and Muse ESP-IDF 6.0.1 environments separate.

| Session | Reading and exercise | Completion criterion |
| --- | --- | --- |
| 1. Architecture and evidence | Read the [experiment](../experiments/muse-passport/README.md) and [device report](muse-device-test.md); draw Wi-Fi, proxy, TLS, registration, upload and subscription stages. | Explain what HTTP 200, Online, registration ACK and a matching reply each prove. |
| 2. Proxy and configuration | Read `mac_proxy.py`, `configure_proxy.py` and `overlay/esp32/components/passport_proxy/`; follow the [proxy guide](../experiments/muse-passport/proxy-guide.md), run tests and read back configuration. | Distinguish the Mac listener `0.0.0.0` from its LAN address; explain certificate verification inside CONNECT. |
| 3. Memory without PSRAM | Read workspace, queue and registration-prefix changes in `muse-sdk.patch`; run prepared-SDK `test_passport*.py` and `test_link_noise_tunnel.py`. | Draw RX, service reassembly, TX and envelope lifetimes; explain the first-chunk overlap bug. |
| 4. Reproduction and regression | Prepare the pinned SDK, keep the token private, build and run `verify.py --require-proxy`; run proxy and voice protocol tests. Inspect an existing `.work` before changing it. | Record revision, toolchain, image hash, test counts and skip reasons without committing secrets, firmware or raw logs. |
| 5. Hardware acceptance | Record settings, test spoken requests and short/silent recordings, then proxy outage/recovery and reboot while idle. Inspect physical keys, display and speaker separately. | Correlate each request with its reply and identify the failing stage; Online alone is insufficient. |

Reusable host checks from the repository root, after preparing the SDK:

```sh
./tools/test.sh
python3 -m unittest discover -s experiments/muse-passport/tests -p 'test_*.py' -v
python3 -m unittest discover -s experiments/muse-passport/.work/muse-gadget-sdk/esp32/tests -p 'test_passport*.py' -v
python3 -m unittest discover -s experiments/muse-passport/.work/muse-gadget-sdk/esp32/tests -p 'test_link_noise_tunnel.py' -v
```

For each exercise, retain a sanitized hypothesis, operation, expectation, observation, conclusion and unverified scope. Remaining acceptance includes ten physical push-to-talk turns, short/silent boundaries, a continuous ten-minute online soak, all three keys/menu navigation and visual inspection. Two automated acoustic turns and the user's app confirmation do not replace these checks. This profile returns text; spoken replies are not implemented.

Prioritize easier proxy startup, configuration visibility, then long-run memory measurements. Change one item at a time and retest registration plus a real voice round trip. Keep TTS, OTA and IDF upgrades separate from memory fixes.
