[简体中文](README.zh_CN.md) · **English**

# AI Passport Learning Lab

A practical learning repository for **FoloToy AI Passport**: understand the board,
run a desktop Tetris demo, build Tetris and IELTS vocabulary games, and distinguish
automated checks from real hardware acceptance.

The device is an ESP32-C3 wearable with 8 MB Flash, no PSRAM, a 240 × 320 color
display, three buttons, a microphone, a speaker, Wi-Fi and BLE.

## Start here

| Goal | Entry point |
| --- | --- |
| Learn the hardware and firmware architecture | [Learning guide](docs/learning-guide.md) |
| Explore what else the board can do | [Capabilities and play ideas](docs/capabilities.md) |
| Run a demo without a device | `./demo-tetris/run.sh` |
| Run tests | `./tools/test.sh` |
| Build the device games on macOS | `./tools/build.sh` |
| Flash an explicitly selected device | `./tools/flash.sh /dev/cu.usbmodemXXXX` |
| Review actual test evidence and limitations | [Test report](docs/test-report.md) |
| Understand source provenance and licenses | [Sources](docs/sources.md) |

## Two demonstrations

**Desktop Tetris** uses the actual upstream pure-C game model and a terminal
adapter. A/D move, W rotates, Space drops, P pauses, R restarts, Q exits.
Use a terminal at least 45 columns by 29 rows. This is a host program, not an
ESP32 emulator or firmware.

**Device Games** has its own LVGL launcher and screens. In Tetris, UP/DOWN move
left/right, OK rotates, hold UP pauses/resumes, hold DOWN drops, and OK restarts
after game over. **Hold OK for at least 0.5 seconds to return to the launcher.**
IELTS mode contains 36 original practice words across six themes, ten-question
rounds, score/streak feedback and mistake review. UP/DOWN select; OK submits or
continues. The current device UI is English; Chinese translations in the word
bank are not rendered. Mistakes and the highest score use NVS.

## Repository map

```text
demo-tetris/         Terminal adapter, pure model, sanitizer and PTY tests
firmware/           Complete ESP-IDF game project and maintained BSP snapshot
  main/             Independent game UI and pure game models
  components/bsp/   Display, buttons, audio, battery and shared bus drivers
  tests/            Host tests and an opt-in on-device diagnostic harness
  tools/            Build, verification, archival, flashing and monitoring
  docs/             Original upstream engineering/reference documentation
docs/               This learning lab's guides, capabilities and test report
tools/              Short entry points for this repository
.github/workflows/  Host/static CI and an optional firmware build
```

## Build and install

On macOS, with Xcode Command Line Tools, Git and Python 3.9+ already available:

```bash
./tools/test.sh
./tools/build.sh
./tools/flash.sh /dev/cu.usbmodemXXXX
./tools/monitor.sh /dev/cu.usbmodemXXXX
```

The build helper downloads ESP-IDF **5.5.3** and its tools into the repository's
ignored `.passport-toolchain/`, restores/verifies fixed-version assets, runs
the complete gate, and archives a verified merged image with matching ELF/MAP.
On other platforms, activate ESP-IDF 5.5.3 and run `cd firmware && ./tools/validate.sh`.
An existing toolchain can be selected with `PASSPORT_TOOLCHAIN_ROOT`.

Flash only `firmware/build/FoloToy-AI-Passport-full.bin` at **0x0**. The flash helper
checks the Espressif USB target, ESP32-C3 chip and 8 MB geometry. A merged write
replaces the current firmware and can reset stored settings/progress. No full-chip
erase is performed. Opening USB serial monitoring may reset this device.

## Validation

The [test report](docs/test-report.md) identifies builds and evidence separately.
Host checks include one million randomized Tetris actions, 12,600 randomized
word-game rounds, sanitizers and terminal interaction tests. An optional diagnostic
build exercises the real ESP32 UI, synthetic input queue, gameplay, NVS and heap.
Synthetic input does not establish physical ADC-button or visible-pixel correctness.

CI runs host/static checks on pushes and pull requests. Select `build_firmware`
when manually dispatching the workflow to build and retain the merged image.
CI has no access to your USB device; it does not flash hardware.

## More possibilities

The strongest extensions are audio-assisted English learning, BLE multiplayer,
and a desktop AI companion. The board can also support radio streaming, reminders,
offline readers, electronic badges and pets. See the capability guide for existing
examples, resource limits and what each application must implement.

This is an independent learning project, not an official FoloToy release. The
MIT license and original copyright notices are preserved. Toolchains, generated
firmware, raw device logs, personal device IDs and credentials are excluded.


## Research

- [Muse Gadgets](docs/research/muse-gadgets.md)
- [Personal agent ecosystem](docs/research/personal-agent-ecosystem.md)
