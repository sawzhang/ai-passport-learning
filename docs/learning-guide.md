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
