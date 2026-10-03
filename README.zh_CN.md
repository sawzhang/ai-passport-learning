**简体中文** · [English](README.md)

# AI Passport 学习实验室

围绕 **FoloToy AI Passport** 汇总硬件学习、可运行 demo、俄罗斯方块与雅思词汇
固件、构建烧录流程及测试证据。目标是从“理解设备”走到“能复现、能验证”。

设备采用 ESP32-C3，8 MB Flash、无 PSRAM，拥有 240 × 320 彩屏、三枚功能按键、
麦克风、扬声器、Wi-Fi 与 BLE。

## 从这里开始

| 目标 | 入口 |
| --- | --- |
| 理解硬件及固件架构 | [学习指南](docs/learning-guide.zh_CN.md) |
| 探索更多可玩性 | [能力与玩法地图](docs/capabilities.zh_CN.md) |
| 没有设备也能运行 demo | `./demo-tetris/run.sh` |
| 执行测试 | `./tools/test.sh` |
| 在 macOS 编译真机游戏 | `./tools/build.sh` |
| 烧录指定设备 | `./tools/flash.sh /dev/cu.usbmodemXXXX` |
| 查看实际结果与未验证项 | [测试报告](docs/test-report.zh_CN.md) |
| 查看来源、版本与许可 | [来源说明](docs/sources.zh_CN.md) |

## 两个演示

**桌面俄罗斯方块**使用上游真实的纯 C 模型和终端适配器。A/D 左右移动，W 旋转，
空格落底，P 暂停，R 重开，Q 退出。终端至少 45 列、29 行。
这是电脑程序，不是 ESP32 模拟器，也不能烧录。

**真机双游戏**拥有独立的 LVGL 启动菜单和界面。俄罗斯方块：上键左移、下键右移、
OK 旋转，长按上键暂停/恢复，长按下键落底，游戏结束后 OK 重开。
**长按 OK 至少 0.5 秒即可退出，返回游戏菜单。**
雅思模式包含六个主题的 36 个原创练习词、十题一轮、分数/连胜反馈和错题复习。
上下键选择，OK 提交或继续。当前真机界面为英文，词库中的中文释义未渲染。
错题与最高分通过 NVS 保存。

## 目录结构

```text
demo-tetris/         终端适配器、纯模型、Sanitizer 与 PTY 交互测试
firmware/           完整 ESP-IDF 游戏项目及 BSP 源码快照
  main/             独立游戏界面和纯逻辑模型
  components/bsp/   屏幕、按键、音频、电池和共享总线驱动
  tests/            主机测试与可选真机自动诊断
  tools/            编译、校验、归档、烧录、监控
  docs/             保留的上游工程与参考文档
docs/               本学习项目的指南、能力汇总和测试报告
tools/              简化操作入口
.github/workflows/  主机/静态 CI，以及可选的固件构建
```

## 编译与安装

macOS 已具备 Xcode Command Line Tools、Git 和 Python 3.9+ 时：

```bash
./tools/test.sh
./tools/build.sh
./tools/flash.sh /dev/cu.usbmodemXXXX
./tools/monitor.sh /dev/cu.usbmodemXXXX
```

构建脚本在被 Git 忽略的 `.passport-toolchain/` 内安装 ESP-IDF **5.5.3** 和工具，
恢复并核验固定版本素材，执行完整验证，保存合并固件和对应 ELF/MAP 归档。
其他系统可激活 ESP-IDF 5.5.3 后执行 `cd firmware && ./tools/validate.sh`。
已有工具链可通过 `PASSPORT_TOOLCHAIN_ROOT` 指定。

仅将 `firmware/build/FoloToy-AI-Passport-full.bin` 从 **0x0** 烧录。
烧录脚本核对 Espressif USB 设备、ESP32-C3 和 8 MB Flash。
合并烧录替换原固件，并可能重置配置或学习进度；脚本不会全片擦除。
打开 USB 串口监控可能使设备重启。

## 验证方式

[测试报告](docs/test-report.zh_CN.md) 分开记录构建、主机测试和实机证据。
主机测试覆盖 100 万次随机方块操作、12,600 轮随机词汇测试、Sanitizer 和终端交互。
可选诊断固件在真实 ESP32 上检查 UI、模拟输入队列、游戏流程、NVS 和堆内存。
模拟输入不能证明真实 ADC 按键或实际屏幕像素正确。

CI 在 push、pull request 时运行主机和静态检查。手动运行工作流时勾选
`build_firmware` 可构建并保存合并镜像。CI 没有本机 USB 访问能力，不会烧录设备。

## 接下来还能玩什么

优先扩展方向：英语听说训练、BLE 双机对战、桌面 AI 伙伴。
也可探索网络电台、提醒工具、离线阅读器、电子工牌和电子宠物。
能力指南列出真实实例、资源约束和需要由应用实现的部分。

本项目是独立学习实践，非 FoloToy 官方发行版。保留 MIT 许可和原作者版权。
不提交工具链、生成固件、原始设备日志、个人设备标识或凭据。


## 研究汇总

- [Muse Gadgets](docs/research/muse-gadgets.zh_CN.md)
- [Personal agent ecosystem](docs/research/personal-agent-ecosystem.zh_CN.md)
