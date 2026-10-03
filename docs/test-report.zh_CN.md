[English](test-report.md) · **简体中文**

# 测试报告

测试日期：2026-10-03。各项分别说明证据类型，某一层通过不代表其他层也已验证。

| 层次 | 结果与证据 | 范围 |
| --- | --- | --- |
| 主机俄罗斯方块 | PASS：sanitizer、一百万次随机操作 | 边界、碰撞、暂停、计分、四行消除 |
| 主机词汇游戏 | PASS：12,600 轮随机测试与规则测试 | 回合、计分、错题与复习 |
| 固件检查 | ESP-IDF 5.5.3 下 PASS | 编译链接、分区布局、镜像与 ELF 配对归档 |
| ESP32 诊断 | PASS：140 轮、601 秒、零失败 | 真机 UI/游戏代码、模拟队列事件、堆与 NVS 热重启 |
| 用户实体验收 | 用户反馈两个游戏均 OK | 实体交互，未提供逐项测量记录 |
| 冷断电、ADC 电压、像素采集、续航 | 未独立测量 | 模拟诊断不能证明这些属性 |

诊断覆盖菜单进出、移动/旋转/暂停/落底、固定局面消行与边界碰撞、结束重开、十题词汇回合及答对移除错题。每轮均进出两个游戏；采样剩余堆始终为 238,656 字节。最高分与错题记录通过 `esp_restart` 恢复，这是热重启，不是断电验证。

脱敏日志摘录：

```text
SELFTEST boot stage=0 best=0 mistake1=0
SELFTEST boot stage=1 best=1234 mistake1=1
SELFTEST persistence failures=0; starting 600-second UI/game stress
SELFTEST cycles=140 elapsed=601s heap=238656 failures=0
SELFTEST RESULT PASS cycles=140 heap=238656 failures=0
```

## 构建标识

| 镜像 | SHA-256 | 合并镜像大小 |
| --- | --- | --- |
| 已测诊断固件 | `a76f0084b2d6b63781cb6857ad1fef4f99b6737c40e50cb2ccf8b7eecbe1e714` | 726,368 字节 |
| 恢复的普通游戏 | `3cbd047bc0a6dbb0f28201c5d5c6272f8fc9ba4c5bfde8c62b6b4af60c451088` | 721,744 字节 |

普通应用大小 656,208 字节，匹配 ELF 的 SHA-256 为 `cad5f6b7ba40883d4f99e6bc363a65d7a7fbaef4f5224b5dc2d56cba90e00b5d`。硬件为 ESP32-C3 revision 1.1、8MB Flash。factory 应用起点 0x10000，合并镜像写到 0x0。未执行整片擦除。打开 USB 串口可能让该板复位，单次 USB 复位不代表固件崩溃。

诊断会修改 NVS 测试记录。测试后恢复普通固件，不把诊断分数当作用户进度。重新编译可能因为路径或构建元数据产生不同 hash，应使用自己构建对应的校验归档。

## 复现

执行 `./tools/test.sh` 和 `./tools/build.sh`；普通构建会关闭设备诊断。主动运行诊断时，先激活 ESP-IDF 5.5.3，进入 `firmware`，为 `./tools/validate.sh --firmware` 设置 `PASSPORT_DEVICE_SELF_TEST=1`，再用已校验烧录工具写入。至少采集 660 秒并检查最终结果，之后恢复独立归档的普通镜像。

诊断使用软件局面和输入事件，不是实体按键电气驱动或视觉识别。Muse 移植、社区玩法、无线联机、语音服务仅做调研，不在上述通过范围内。
