[简体中文](README.zh_CN.md) · **English**

# Muse × AI Passport experiment

Experimental board adapter for FoloToy AI Passport: ESP32-C3, 8MB Flash, no PSRAM, 240×320 ST7789, three ADC keys, ES8311 audio and CW2017 battery meter. This is not officially supported Muse hardware. See the [test report](../../docs/muse-device-test.md) for measured results and pending acceptance checks.

## Reproduce

1. Install and activate official **ESP-IDF 6.0.1** in a separate environment from the games project (5.5.3).
2. Run `python3 prepare.py`. It checks out the SDK commit in `UPSTREAM`, applies `muse-sdk.patch`, and copies `overlay/` into ignored `.work/`. It refuses to overwrite an existing workspace. Read upstream AGENTS.md before building.
3. Save your SDK token in a private file outside the repository (0600). Set `MUSE_TOKEN_FILE` to its path, then run `python3 build.py`. Generated configuration and firmware contain the token: keep them private.
4. In `.work/muse-gadget-sdk/esp32`, run `python -m unittest discover -s tests -p 'test_*.py'`. For the ADC helper alone, run `python3 -m unittest discover -s overlay/esp32/tests` from this experiment.
5. Run `python3 verify.py` to validate target, flash size, image headers, checksums and partition bounds, then run upstream `tools/muse/board.sh flash passport /dev/cu.usbmodemXXX`, using the actual enumerated port. Never flash the default C5 image, erase the whole chip or burn eFuses.
6. Enable Developer mode in the Muse phone app, select `MuseGadget-…`, confirm with physical OK, then provision **2.4GHz Wi-Fi** through the app.

## Controls and limits

Hold OK to speak; release to submit. UP/DOWN opens the menu, then navigates it; OK selects and holding DOWN for 0.5 seconds returns. The compact Muse UI uses an eight-line single display buffer and reduced radio pools; audio pre-roll is 80ms.

Without PSRAM, voice uploads use Link's control session and replies are text. The independent Hatch session, downloaded images, network tunnel and OTA are disabled. Serial typed chat is unavailable. CW2017 battery percentage/voltage are read, but charger/USB detection is unvalidated: this bench profile reports USB present to prevent automatic battery sleep. Software power-off is unsupported; use the physical switch. Battery endurance is untested.

The prior games image remains in the private device-development archive; it can also be rebuilt from the games sources. Switching profiles changes partitions/settings.

## Licensing

Original adapter, scripts and docs are MIT; copied BSP retains FoloToy MIT. The SDK patch modifies Apache-2.0 sources. See NOTICE. The upstream avatar has separate terms and is not distributed here. Tokens, signing keys, firmware and private logs are excluded. Muse service access has separate SDK terms.
