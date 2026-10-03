[English](capabilities.md) · **简体中文**

# 系统能力与玩法

调研日期：2026-10-03。硬件事实依据固定版本 BSP 与[官方来源](sources.zh_CN.md)。

| 能力 | 可玩方向 | 边界 |
| --- | --- | --- |
| 240×320 ST7789 SPI 屏幕、LVGL | 游戏、徽章、阅读器、agent 状态 | 无触摸；内存预算有限 |
| 三个 ADC 按键，500ms 长按 | 菜单、答题、操作确认 | 共用 ADC，需实体按键验收 |
| ES8311 麦克风和扬声器 | 录音、跟读、网络电台 | 语音识别和 TTS 需要服务及流式传输 |
| 2.4GHz Wi-Fi、BLE | 网络小工具、联机游戏 | 不支持 5GHz 或蓝牙 Classic |
| 8MB Flash、无 PSRAM | 离线词库、轻量游戏 | 大语言模型放在手机、电脑或服务器 |
| CW2017、520mAh 电池 | 随身学习 | 续航需按应用测量 |
| NTAG213 被动 NFC | 碰一碰打开链接 | 不是 MCU 可控的 NFC 读卡器 |
| USB Serial/JTAG | 烧录、诊断 | 不代表支持 USB 键盘/HID |

本次官方玩法 API 快照共 738 项：游戏 312、效率 161、信息 101、学习 71、媒体 43、社交 31、开发 19。数量会变化；没有据此保证质量，也没有复制社区介绍或声称全部真机测试。

| 方向 | 现有参考 | 下一步实验 |
| --- | --- | --- |
| 英语学习 | [听力 193](https://ai-passport.folotoy.cn/plays/193/)、[旅游英语 745](https://ai-passport.folotoy.cn/plays/745/) | 给现有词汇游戏加入短录音和回放 |
| 语音助手 | [小智 72](https://ai-passport.folotoy.cn/plays/72/) | 独立评估语音固件；当前 IDF 基线不同 |
| 桌面助手 | [Codex Buddy 96](https://ai-passport.folotoy.cn/plays/96/) | 展示任务状态，用按键确认限定动作 |
| 媒体和身份 | [电台 65](https://ai-passport.folotoy.cn/plays/65/)、[徽章 22](https://ai-passport.folotoy.cn/plays/22/) | 流式音频或离线身份卡 |
| 安装生态 | [小程序安装器 592](https://ai-passport.folotoy.cn/plays/592/) | 替换固件前研究分区与恢复规则 |

现有游戏未启用音频和无线。默认 factory 分区没有 OTA 槽；合并固件写入可能重置 NVS。摄像头、运动追踪和触摸不是该设备的基础接口。

优先建议：英语听说练习、BLE 双人答题、personal agent 状态与确认终端。继续阅读 [Muse 调研](research/muse-gadgets.zh_CN.md)和[生态思考](research/personal-agent-ecosystem.zh_CN.md)。
