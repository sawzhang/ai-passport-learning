[English](README.md) · **简体中文**

# 中文回复字体

`passport_font_cjk_16.c` 来自固定 LVGL 托管组件中的 Noto Sans SC Regular。`manifest.json` 记录源字体与生成文件的 SHA-256、转换器版本和请求范围中缺失的码点。字体使用 SIL OFL 1.1 许可，分发时保留 `OFL.txt`。

资产包含 21,136 个字形，覆盖 GB2312 的全部 6763 个汉字和常见中文／全角标点。不覆盖全部 Unicode、emoji 或汉字扩展 B；不支持的字形仍显示占位符。英文沿用宽 8 像素的 Unscii，中文回退到该字体，字宽 16 像素。两行字幕使用完整的 31 像素行高，避免裁切较高的标点。

字形采用无压缩 2 bpp 位图，存放于 Flash。位图偏移超出紧凑格式，必须启用 `CONFIG_LV_FONT_FMT_TXT_LARGE=y`。运行时不将整套字体载入 RAM，完整应用仍须通过分区大小校验。

固定 SDK 与 LVGL 依赖就绪后，在仓库根目录执行：

```sh
npm install --prefix experiments/muse-passport/.local/font-tools --no-audit --no-fund lv_font_conv@1.5.3
python3 experiments/muse-passport/generate_chinese_font.py
```

生成器核对源文件和输出哈希。`test_passport_chinese.py` 验证字形覆盖、宽度限制，并运行真实文本函数检查中英文分页和 UTF-8 截断。真机字体绑定和屏幕效果需要独立验收。

需要检查实际 LCD 刷新像素时，在目标字幕显示期间运行 `python3 experiments/muse-passport/capture_lcd.py /dev/cu.usbmodem21101 experiments/muse-passport/.local/screen.png`。先关闭其他串口读取程序。USB `p` 命令分块传输 RGB565 像素，不在板端分配完整帧缓冲；主机拒绝不完整截图。此方法核对渲染像素，不替代物理面板检查。
