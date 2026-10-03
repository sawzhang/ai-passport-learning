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
