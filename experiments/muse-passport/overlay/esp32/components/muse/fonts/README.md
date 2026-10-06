[简体中文](README.zh_CN.md) · **English**

# Chinese reply font

`passport_font_cjk_16.c` is generated from Noto Sans SC Regular supplied in the pinned LVGL managed component. The source and generated SHA-256 values, converter version and missing requested code points are recorded in `manifest.json`. The font is licensed under SIL OFL 1.1; retain `OFL.txt` with distributions.

The asset contains 21,136 glyphs, including all 6,763 GB2312 Han characters and common CJK/full-width punctuation. It does not cover all Unicode, emoji or CJK Extension B; unsupported glyphs retain visible placeholders. Latin text uses the existing 8-pixel-wide Unscii font, with this font as fallback. CJK glyphs advance 16 pixels; the two-line caption uses the font's full 31-pixel line height to avoid clipping taller punctuation.

Glyph bitmaps are uncompressed 2 bpp in Flash. Enable `CONFIG_LV_FONT_FMT_TXT_LARGE=y` because bitmap offsets exceed the compact format. No full font is loaded into RAM. The complete application must still pass partition-size verification.

From the repository root, after the pinned SDK and LVGL dependency are available:

```sh
npm install --prefix experiments/muse-passport/.local/font-tools --no-audit --no-fund lv_font_conv@1.5.3
python3 experiments/muse-passport/generate_chinese_font.py
```

The generator verifies the source and output hashes. `test_passport_chinese.py` checks coverage, width limits, Chinese/Latin pagination and UTF-8 truncation using the real text helpers. Hardware font binding and visual results require separate device checks.

To inspect actual LCD flush pixels, run `python3 experiments/muse-passport/capture_lcd.py /dev/cu.usbmodem21101 experiments/muse-passport/.local/screen.png` while the desired caption is visible. Close other serial readers first. The USB `p` command streams bounded RGB565 bands; it does not allocate a full framebuffer on the board. The host rejects incomplete captures. This checks rendered pixels, not the physical panel.
