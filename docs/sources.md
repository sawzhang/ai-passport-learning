[简体中文](sources.zh_CN.md) · **English**

# Sources and provenance

Reviewed 2026-10-03.

- [FoloToy AI Passport](https://github.com/FoloToy/ai-passport): firmware snapshot `0b9e4c81ee4421c0bac39ca3561d65a8285acd4a`; see `firmware/SOURCE_SNAPSHOT.json`. Missing brand assets were restored with Git blob verification. Local additions include games, tests and build/device helpers.
- [Upstream Tetris branch](https://github.com/FoloToy/ai-passport/tree/demo/tetris-game): model snapshot `f2f6693b1b962bc3f9ab3f854b9598e8eb002372`; local terminal adapter and tests.
- [Official guides](https://ai-passport.folotoy.cn/guides/), [play catalog](https://ai-passport.folotoy.cn/), [installer 592](https://ai-passport.folotoy.cn/plays/592/).
- [Xiaozhi board project](https://github.com/FoloToy/folo-ai-passport-xiaozhi): a separate voice stack, not included or device-tested here.
- [Muse SDK](https://github.com/facebookincubator/muse-gadget-sdk), [ESP32 README](https://github.com/facebookincubator/muse-gadget-sdk/blob/main/esp32/README.md), [Linux README](https://github.com/facebookincubator/muse-gadget-sdk/blob/main/linux/README.md), [Home Link](https://gadgets.muse.ai/home-link), [token terms](https://gadgets.muse.ai/sdk-terms).

FoloToy MIT notices are preserved. This lab adds documentation and original practice material under the root MIT license. Muse code is not vendored: its Apache-2.0 source license, third-party exceptions and service/token terms must be considered separately if integrating later. Community play links retain their own licenses. No toolchain, SDK token, Wi-Fi credential, generated firmware or raw personal device log is published.
