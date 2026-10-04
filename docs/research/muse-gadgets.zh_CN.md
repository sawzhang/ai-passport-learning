[English](muse-gadgets.md) · **简体中文**

# Muse Gadgets 调研

原调研日期：2026-10-03；移植实验更新：2026-10-04。现已使用 ESP-IDF 6.0.1 完成 C3 适配、构建、烧录与启动/音频驱动测试。端到端配对、联网和语音验收尚未完成，见 [真机报告](../muse-device-test.zh_CN.md) 和 [复现源码](../../experiments/muse-passport/README.zh_CN.md)。

[官方 SDK](https://github.com/facebookincubator/muse-gadget-sdk) 提供 ESP32 外设端和 Linux 执行端。设备配对需要 token 与 Muse app。源码采用 Apache-2.0，存在第三方许可例外，头像资源不包含在该许可内。

[ESP32 文档](https://github.com/facebookincubator/muse-gadget-sdk/blob/main/esp32/README.md) 指定 ESP-IDF 6.0.1。已列出的目标包括 C5/S3/C6/经典 ESP32，没有 AI Passport/C3。无 PSRAM 板不运行家庭网络隧道。按键语音输入的回复为文字；语音回复需额外接入 TTS。上游未列出 AI Passport/C3；本仓库增加实验适配，已在 C3 启动，但尚未验证完整云端链路。固定提交的 Muse 组件声明 IDF >=5.5，项目仍要求 6.0.1。

[Linux SDK](https://github.com/facebookincubator/muse-gadget-sdk/blob/main/linux/README.md) 提供 shell 执行、文件读写与设备健康信息，权限等同安装时选定的系统账号，包括该账号可能拥有的 sudo 权限。复杂集成和长期服务更适合放在这里，而非 C3。

[Home Link](https://gadgets.muse.ai/home-link) 是本地 HTTP 网关，采用 ESP32-C5、8MB PSRAM 和双频 Wi-Fi 6。成品仅接受官方固件，DIY SDK 是另一条路线。AI Passport 则是 C3、无 PSRAM、仅 2.4GHz Wi-Fi。

[token 条款](https://gadgets.muse.ai/sdk-terms) 将服务权限与源码许可分开：个人非商业用途，在指定分享条件下最多 50 台设备，公开商业分发需许可，访问可以撤销且不属于受支持平台。Apache 源码许可不等于商业服务授权。

## 原始实验建议（2026-10-03）

保留已验证的游戏基线。先通过 Linux 网关模拟简短状态及按键确认协议，再独立评估 C3 分支：目标编译、BLE 配对、TLS/重连内存、屏幕按键适配、音频缓冲。每一步实测，不承诺完整 Home Link 等价能力。凭据只进入被忽略的本地配置。C3 移植已推进至启动及驱动验证，完整集成状态以真机报告为准。

## 对本设备的选型判断

| 路线 | 优点 | 成本与待验证项 |
| --- | --- | --- |
| AI Passport 直接移植 Muse SDK | 随身屏幕、三键和音频已有 BSP | C3 未列为目标；需 IDF 6.0.1 迁移、内存剖析与新配对流程 |
| AI Passport + Linux Muse 网关 | 保留轻量设备端，复杂执行放在网关 | 需自行定义端到端消息、身份和回执协议 |
| 独立 agent + AI Passport 终端 | 服务可替换，离线学习可保留 | 自行实现账号、工具权限、音频和重连 |
| 换 S3/PSRAM 板做 Muse 原型 | 更接近已支持的显示/音频目标 | 新硬件成本，不能作为当前 C3 的测试证据 |

建议先做网关原型，以最小的状态消息和一个限定动作验证价值。网络隧道、图片、TTS 与 UI 不应一次性叠加；每增加一项均记录峰值内存与失败恢复。若未来分发成品，再单独核对服务条款、许可和账户地区可用性。
