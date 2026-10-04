[English](README.md) · **简体中文**

# Muse × AI Passport 实验

本项目为 FoloToy AI Passport 增加实验性 Muse SDK 板级适配：ESP32-C3、8MB Flash、无 PSRAM、240×320 ST7789、ADC 三键、ES8311 麦克风/扬声器、CW2017 电量计。它不是 Muse 官方支持的硬件。

固定 SDK 版本、构建及真机测试结果见 [测试报告](../../docs/muse-device-test.zh_CN.md)。游戏恢复文件留在设备开发目录的私有构建归档，重新构建游戏也可恢复；切换固件会改变分区和设置。

## 复现

1. 安装并激活官方 ESP-IDF **6.0.1**，不要使用游戏项目的 5.5.3 环境。
2. 运行 `python3 prepare.py`，它获取 `UPSTREAM` 中固定的 SDK 提交，将 `muse-sdk.patch` 和 `overlay/` 应用到被忽略的 `.work/`。已存在目录不会被覆盖。准备后阅读上游 `AGENTS.md`。
3. 把自己的 SDK token 写入仓库外的私有文本文件（权限 0600）。设置 `MUSE_TOKEN_FILE` 为该文件路径，运行 `python3 build.py`。构建配置和固件含 token，不能上传。
4. 在 `.work/muse-gadget-sdk/esp32` 运行 `python -m unittest discover -s tests -p 'test_*.py'`。只测 ADC 适配可在本实验目录运行 `python3 -m unittest discover -s overlay/esp32/tests`。
5. 运行 `python3 verify.py` 检查目标、Flash、镜像头、校验和与分区布局，再运行上游 `tools/muse/board.sh flash passport /dev/cu.usbmodemXXX`，端口以本机实际枚举为准。不使用默认 C5 固件，不执行整片擦除或 eFuse 写入。
6. Muse 手机 app 开启 Developer mode，选择广播的 `MuseGadget-…`，按设备 OK 确认，并通过 app 配置 **2.4GHz Wi-Fi**。

## 按键与功能边界

- 主界面：按住 OK 说话，松开提交；UP 或 DOWN 打开菜单。
- 菜单内：UP/DOWN 移动，OK 选择，按住 DOWN 约 0.5 秒返回。
- 保留 Muse 头像和状态界面，使用紧凑布局、8 行单显示缓冲和较小无线缓冲，语音预录 80ms。
- 无 PSRAM：语音经 Link 控制会话上传，回答为文字；独立 Hatch 会话、图片显示、网络隧道、OTA 关闭；不支持串口文字聊天。
- CW2017 可读取电量与电压，但 USB/充电状态检测尚未验证。本实验按 USB 供电报告以禁用自动电池休眠，软件关机返回不支持，请使用物理开关。这不是电池续航验证。

## 许可与凭据

原创适配、脚本和文档为 MIT；复制的 BSP 保留 FoloToy MIT。上游代码修改遵循 Apache-2.0，见 `NOTICE`。上游默认头像有单独许可，本项目不分发头像、私钥、SDK token 或生成固件。Muse 服务权限另受 SDK 服务条款约束。
