[English](learning-guide.md) · **简体中文**

# 学习路线

先运行终端 demo，再从纯 C 游戏模型读到 LVGL 真机界面。建议同时阅读 `firmware/main/games_main.c`、游戏模型和 `firmware/components/bsp/include/bsp_pins.h`：模型处理规则，界面处理页面和输入，BSP 处理硬件。

1. 运行 `./tools/test.sh` 和 `./demo-tetris/run.sh`。
2. 修改词库前，理解十题一轮、答题和错题复习的状态转换。
3. 用 `./tools/build.sh` 编译；游戏基线为 ESP-IDF 5.5.3。
4. 明确选择 USB 串口，用脚本写入已校验的合并固件。
5. 检查启动日志，再实际检查屏幕和按键。

两个游戏均长按 OK 至少 0.5 秒退出。诊断固件应单独使用：`PASSPORT_DEVICE_SELF_TEST=1` 注入模拟输入并向 NVS 写入测试记录；根目录普通构建脚本会关闭该选项。

继续阅读[测试证据](test-report.zh_CN.md)、[系统能力](capabilities.zh_CN.md)和[来源](sources.zh_CN.md)。

## Muse 代理与语音项目学习方案

目标：能独立解释“板载麦克风 → HTTP CONNECT → TLS／Link → Muse → 板端文字回答”，能复现构建和定位连接故障。建议分 5 次学习，每次 60–90 分钟；游戏使用 ESP-IDF 5.5.3，Muse 使用 6.0.1，分别维护环境。

| 阶段 | 阅读与实践 | 完成标准 |
| --- | --- | --- |
| 1. 架构与证据 | 阅读 [Muse 实验说明](../experiments/muse-passport/README.zh_CN.md)和[真机报告](muse-device-test.zh_CN.md)，画出 Wi-Fi、代理、TLS、Link 注册、语音上传和订阅回复流程。 | 能说明 HTTP 200、Online、注册确认、收到对应回答分别证明什么。 |
| 2. 代理与配置 | 阅读 `mac_proxy.py`、`configure_proxy.py` 和 `overlay/esp32/components/passport_proxy/`；按[代理指南](../experiments/muse-passport/proxy-guide.zh_CN.md)运行代理测试并读回设备配置。 | 理解 `0.0.0.0` 是 Mac 监听地址，设备使用 Mac 的实际 LAN 地址；能说明 CONNECT 后仍需 TLS 证书校验。 |
| 3. 无 PSRAM 内存 | 阅读 `muse-sdk.patch` 中的工作区、发送队列和首包修改；运行准备后的 SDK 中 `test_passport*.py` 与 `test_link_noise_tunnel.py`。 | 画出 RX、服务重组、TX、信封缓冲区的生命周期；解释为何注册首包不能放入与服务编码重叠的缓冲区。 |
| 4. 复现与回归 | 按实验说明准备固定 SDK，私有保存 token，构建并运行 `verify.py --require-proxy`；运行代理与语音协议测试。已有 `.work` 时先检查改动，不覆盖工作目录。 | 记录源码提交、工具链版本、应用 SHA-256、测试数量／跳过原因；不把 token、固件或原始日志加入 Git。 |
| 5. 真机验收 | 先记录当前设置，再做真实语音请求、短录音和静音录音；在安全空闲状态测试代理停止／恢复及设备重启。另做实体按键、屏幕和扬声器检查。 | 每次请求对应一条收到的回答；失败可明确落到网络、注册、上传或订阅阶段，不能只用 Online 判定成功。 |

可复用的主机检查（从仓库根目录执行，先按实验说明准备 SDK）：

```sh
./tools/test.sh
python3 -m unittest discover -s experiments/muse-passport/tests -p 'test_*.py' -v
python3 -m unittest discover -s experiments/muse-passport/.work/muse-gadget-sdk/esp32/tests -p 'test_passport*.py' -v
python3 -m unittest discover -s experiments/muse-passport/.work/muse-gadget-sdk/esp32/tests -p 'test_link_noise_tunnel.py' -v
```

每次练习保留一份脱敏记录：假设、操作、期望、观察、结论、未验证项。优先补齐当前未完成的十轮按键语音操作、短／静音边界、连续十分钟联网观察、三键菜单与屏幕验收。现有两轮自动声学测试和用户 app 确认不替代这些检查。当前板级配置返回文字，不把语音播报列为已实现能力。

后续改进可按“代理启动便捷性 → 配置状态提示 → 长时间内存趋势”排序；每次只改变一项，再重测注册和实际语音往返。不要把增加 TTS、OTA 或升级 IDF 混入同一轮内存修复。

## 中文显示专项练习

沿中文回复依次检查 UTF-8 截断、双列分页、LVGL 回退字形查找与 LCD 刷新像素。运行 `test_passport_chinese.py`，用 `generate_chinese_font.py` 复现字体，再按[字体指南](../experiments/muse-passport/overlay/esp32/components/muse/fonts/README.zh_CN.md)采集实际 LCD 像素。验收标准是中英混排在两行内清晰可读、UTF-8 边界完整、应用镜像通过分区容量校验。云端字符串正常或字形查找成功，都不能单独证明实际渲染正确。
