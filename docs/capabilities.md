[简体中文](capabilities.zh_CN.md) · **English**

# Capabilities and play ideas

Research date: 2026-10-03. Hardware facts come from the pinned BSP and official guides linked in [sources](sources.md).

| Capability | Useful experiments | Boundary |
| --- | --- | --- |
| 240×320 ST7789 SPI display, LVGL | Games, badges, readers, agent status | No touch input; small RAM budget |
| Three ADC buttons, 500 ms long press | Menus, quiz answers, approval | Shared ADC input requires real button testing |
| ES8311 microphone/speaker | Recording, repeat-after-me, radio | Cloud speech and TTS need a service and streaming |
| 2.4 GHz Wi-Fi and BLE | Network widgets, cooperative games | No 5 GHz or Bluetooth Classic |
| 8 MB Flash, no PSRAM | Offline words, lightweight games | Large language models belong on a phone, PC or server |
| CW2017 / 520 mAh battery | Portable learning | Battery life must be measured per application |
| NTAG213 passive NFC | Tap a URL | Not an MCU-controlled NFC reader |
| Native USB Serial/JTAG | Flashing and diagnostics | Does not establish USB keyboard/HID support |

The official catalog API returned 738 plays in this dated snapshot: games 312, productivity 161, information 101, learning 71, media 43, social 31, developer 19. These are mutable counts, not quality endorsements. The repository does not copy community descriptions or claim those plays were tested.

| Direction | Existing reference | Next experiment |
| --- | --- | --- |
| English learning | [Listening 193](https://ai-passport.folotoy.cn/plays/193/), [Travel English 745](https://ai-passport.folotoy.cn/plays/745/) | Add short recordings and playback to the current vocabulary game |
| Voice assistant | [Xiaozhi 72](https://ai-passport.folotoy.cn/plays/72/) | Evaluate a separate voice firmware; its current IDF baseline differs |
| Desktop companion | [Codex Buddy 96](https://ai-passport.folotoy.cn/plays/96/) | Show task state and require a button for an allowed action |
| Media and identity | [Radio 65](https://ai-passport.folotoy.cn/plays/65/), [Badge 22](https://ai-passport.folotoy.cn/plays/22/) | Stream audio or render a compact offline badge |
| Installation | [Miniapp installer 592](https://ai-passport.folotoy.cn/plays/592/) | Study its partition/recovery contract before replacing firmware |

Current games use no radio or audio. The default factory partition layout has no OTA slots. A full merged write may reset NVS. Camera, motion tracking and touch are not part of this board's base interfaces.

Priorities: audio vocabulary practice first, BLE quiz duels second, personal-agent status/confirmation third. See [Muse research](research/muse-gadgets.md) and [ecosystem proposal](research/personal-agent-ecosystem.md).
