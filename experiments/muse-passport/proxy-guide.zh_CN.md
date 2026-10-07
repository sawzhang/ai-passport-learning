[English](proxy-guide.md) · **简体中文**

# AI Passport 的 Muse 固件使用 Mac 代理

固件支持保存 HTTP CONNECT 代理的 IPv4 地址和端口，使用独立的
`passport_proxy` NVS 命名空间。通过 USB 配置，不改变 Wi-Fi 和 Muse 配对信息。
修改后重启设备，让所有服务统一重连。默认未配置代理。

此功能扩展现有 Muse 固件，保留 Link 语音上传与文字回答。代理覆盖 Muse API
和 Link WebSocket 使用的出站 ESP-TLS 连接；不提供 VPN、任意 UDP 转发，也不会
开启本板已关闭的 Hatch、OTA 或图片下载。不支持 SOCKS5 和代理认证；Mac
仅提供 SOCKS5 时，请先启用 HTTP 代理入口。

## Mac 监听入口

Mac 和 Passport 需要处于互相可达的局域网；访客 Wi-Fi 的客户端隔离会阻止连接。
现有代理已允许局域网连接时，可以直接使用它的 HTTP 端口。若只监听
`127.0.0.1:1087`，在 Mac 上运行：

```sh
python3 experiments/muse-passport/mac_proxy.py --listen 0.0.0.0 --port 18087
```

使用 Muse 时保持进程运行。脚本把字节转发到现有的 `127.0.0.1:1087` HTTP 代理，
不解密 TLS。默认接受私有地址和回环地址的客户端；可用
`--allow-client DEVICE_IP` 限制为指定设备。如 macOS 提示防火墙权限，请允许入站
连接。前台命令不修改原代理应用；长期使用请安装下文的常驻服务。

`0.0.0.0` 是监听地址，设备需要填写 Mac 的实际局域网 IPv4 地址，不能填写
`0.0.0.0` 或 `127.0.0.1`。通常可用 `ipconfig getifaddr en0` 查询，需确认活动
网卡。在路由器保留 Mac 地址，或在 DHCP 地址变化后更新设备配置。Mac 休眠时
代理连接会中断。

## 构建与配置

使用固定版本的 Muse SDK 和 ESP-IDF **6.0.1**，不要使用游戏工具链。
`prepare.py` 应用维护的补丁和 overlay，其中包含 `passport_proxy`。
SDK token 应保存在被忽略、权限为 `0600` 的私有文件中。使用项目内 macOS
工具链时，`bash experiments/muse-passport/build-macos.sh` 执行构建和镜像校验；
token 来自 `.local/sdk-token.txt` 或 `MUSE_TOKEN_FILE`。固件和构建配置含 token，
必须私有保存。

授权烧录后，使用现有 Passport 板级烧录流程安装新固件。关闭其他串口监控，再运行：

```sh
# 示例地址：请替换为本机局域网 IP 和实际串口。
python3 experiments/muse-passport/configure_proxy.py /dev/cu.usbmodemXXXX 192.168.1.10:18087
python3 experiments/muse-passport/configure_proxy.py /dev/cu.usbmodemXXXX
python3 experiments/muse-passport/configure_proxy.py /dev/cu.usbmodemXXXX off
```

对应的 USB 命令为 `>proxy=192.168.1.10:18087`、`>proxy.status` 和
`>proxy=off`，每条以换行结尾。配置入口仅在本地 USB，不新增未认证的 Wi-Fi
配置接口。CLI 检查确认信息并读回保存的值。部分设备打开 USB 串口会复位。
配置后用物理开关重启，确认 Link 变为 Online，再实际测试一次语音请求。

## 连接与失败处理

CONNECT 使用原服务域名，由 Mac 代理解析；设备在隧道内执行原有 TLS 握手，
保留 SNI、证书包和主机名校验，没有关闭 TLS 验证。CONNECT 响应头最多 2 KiB，
受连接配置的超时限制，默认 15 秒；精确读取响应头，不吞掉后续字节。
内部 RAM 和栈占用保持较小，适用于无 PSRAM 的此板。

启用后的代理失败会产生 `passport_proxy` 日志，不会静默回退到直接联网。
检查 Mac 进程、上游代理、防火墙和局域网隔离；HTTP 407 表示需要认证，当前
不支持。通过 `proxy=off` 并重启可明确恢复直连。配置保留于正常重刷和重启，
删除相应 NVS 分区才会清除；配对重置不会清除此独立命名空间。

实现包装 ESP-TLS 同步和异步客户端入口，在原 TLS 状态机前挂接代理隧道。
它使用 ESP-IDF 私有结构，明确固定于 **6.0.1**；升级前需核对源码并重新实际
构建。主机测试覆盖分段响应、TLS 字节保留、异步重入、参数校验、连接失败、关闭、
双向转发和客户端限制：

```sh
python3 -m unittest discover -s experiments/muse-passport/tests -p 'test_*.py' -v
```

Mac 上的 HTTPS 检查只证明代理连通性，不能证明设备语音成功。实机验收仍需
安装新镜像、保存配置、确认云端会话，再测试真实麦克风请求和回答。

## Passport 的 Link 内存配置

此无 PSRAM 板只在已配对启动时、UI 和 Wi-Fi 分配内存之前预留 41 KiB 会话工作区。未配对时将这部分内存留给蓝牙配网。接收密文使用原地解密，服务消息
重组保留独立空间，仍支持 16 KiB 聊天内容。发送侧只在序列化完成后复用服务编码
与 WebSocket 输出缓冲区；尚未收齐的 WebSocket 数据不会与发送缓冲区混用。
注册首包在信封缓冲区暂存后再编码，避免共用发送缓冲区覆盖注册内容。
每次会话结束清理工作区内容。

TLS 发送记录限制为 2 KiB，接收仍支持 16 KiB。此无隧道配置的 DMA 预留为 8 KiB，继续保留 2 KiB 连续块下限和 TLS 分配尺寸
检查。语音使用 1 KiB base64 数据块和 2 KiB 队列，等队列可接收后才分配数据副本。
调整这些尺寸后必须验证 Link 注册和实际语音上传；单独建立 TLS 连接不能验证
完整会话所需的内存。

## macOS 常驻服务

```sh
python3 experiments/muse-passport/mac_proxy_service.py install
python3 experiments/muse-passport/mac_proxy_service.py status
# 停止自动拉起并移除登录启动：
python3 experiments/muse-passport/mac_proxy_service.py uninstall
```

用户级 `com.musepassport.proxy` LaunchAgent 在登录时启动，进程退出后自动拉起，重启间隔至少 10 秒。安装前先停止占用 18087 的临时代理。程序复制到 `~/Library/Application Support/MusePassportProxy`，关闭终端或项目不会停止服务；修改代码后重新运行 install 部署新副本。安装时使用的 Python 路径必须持续有效。

日志位于 `~/Library/Logs/MusePassportProxy/proxy.log`，单文件最多 1 MiB，另保留 3 个备份，只记录运行事件，不记录隧道内容或 token。连接异常彼此隔离，清理操作有超时；只有双向都没有流量达 300 秒才判为空闲。上游中断会关闭受影响连接，上游恢复后新连接可继续使用；被终止进程的既有 TCP 连接无法保留。

此服务不负责启动上游代理，也不阻止 Mac 休眠，不能保证设备在长时间断网后自动重连。除了 launchd PID，还要检查 1087、Mac 局域网地址及 Link 注册。用户注销期间服务不可用。卸载保留程序副本和日志，便于诊断。
