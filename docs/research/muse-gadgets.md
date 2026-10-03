[简体中文](muse-gadgets.zh_CN.md) · **English**

# Muse Gadgets research

Reviewed 2026-10-03. This is source research; no Muse token, account pairing, SDK installation or hardware port was performed.

The [official SDK](https://github.com/facebookincubator/muse-gadget-sdk) offers ESP32 peripherals and Linux execution hosts. Device pairing requires a token and the Muse app. Source is Apache-2.0 with third-party exceptions; the avatar is excluded from that license.

The [ESP32 README](https://github.com/facebookincubator/muse-gadget-sdk/blob/main/esp32/README.md) specifies ESP-IDF 6.0.1. Its listed targets include C5/S3/C6/classic ESP32; AI Passport/C3 is not listed. No-PSRAM boards omit the home-network tunnel. Push-to-talk replies are text; spoken replies require an added TTS service. Thus direct AI Passport support remains unverified. Component metadata accepting IDF >=5.1 does not override the documented 6.0.1 build requirement.

The [Linux SDK](https://github.com/facebookincubator/muse-gadget-sdk/blob/main/linux/README.md) exposes shell execution, file read/write and device health. It runs with the installed account's rights, including sudo if available. This is a better place for substantial integrations and persistent services than the C3.

[Home Link](https://gadgets.muse.ai/home-link) is a local HTTP gateway based on ESP32-C5 with 8 MB PSRAM and dual-band Wi-Fi 6. Its shipped hardware accepts official firmware only; the DIY SDK is a separate path. AI Passport is C3, has no PSRAM and only 2.4 GHz Wi-Fi.

[Token terms](https://gadgets.muse.ai/sdk-terms) separate service access from source rights: personal non-commercial use, a 50-device ceiling under specified sharing conditions, no public commercial distribution without permission, and revocable unsupported access. An Apache license alone does not establish commercial service access.

## Proposed experiment

Keep the tested games baseline intact. First connect a Linux gateway and mock a tiny status/confirmation protocol. Then assess a separate C3 branch: target compilation, BLE pairing, TLS/reconnection heap, display/button adaptation and audio buffers. Measure each stage; do not promise full Home Link parity. Credentials belong in ignored local configuration. Nothing here constitutes a working Muse integration.

## Choosing a route for this board

| Route | Benefit | Cost / evidence still needed |
| --- | --- | --- |
| Direct Muse SDK port | Existing portable display, buttons and audio BSP | Unlisted C3 target, IDF 6.0.1 migration, memory profiling and pairing |
| AI Passport + Linux Muse gateway | Lightweight endpoint, complex work on gateway | A new message, identity and acknowledgement protocol |
| Independent agent + Passport endpoint | Replaceable services and offline learning | Account, permissions, audio and reconnection implementation |
| S3/PSRAM board prototype | Closer to supported display/audio targets | New hardware; does not validate the existing C3 |

Start with the gateway route and one scoped action. Add tunnel, images, TTS and UI separately, measuring peak memory and recovery each time. Product distribution would require a fresh check of service terms, licenses and regional account availability.
