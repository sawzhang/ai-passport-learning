[简体中文](proxy-guide.zh_CN.md) · **English**

# Mac proxy for Muse on AI Passport

The Passport firmware can persist an HTTP CONNECT proxy IPv4 address and port in
its own `passport_proxy` NVS namespace. USB commands configure it without changing
Wi-Fi credentials or Muse pairing. Restart the device after changing the setting
so every service reconnects consistently. No proxy is configured by default.

This extends the existing Muse firmware, including Link voice uploads and text
responses. The proxy covers outbound ESP-TLS connections used by the Muse API and
Link WebSocket. It does not implement a VPN, route arbitrary UDP, or enable Hatch,
OTA or image downloads on this board. SOCKS5 and proxy authentication are not
supported. If the Mac exposes only SOCKS5, enable an HTTP proxy listener first.

## Mac listener

Keep Mac and Passport on a mutually reachable LAN; guest Wi-Fi client isolation
can prevent access. If your proxy already accepts LAN clients, use its HTTP port.
When it listens only on `127.0.0.1:1087`, run this forwarding process on the Mac:

```sh
python3 experiments/muse-passport/mac_proxy.py --listen 0.0.0.0 --port 18087
```

Keep the process running while using Muse. It forwards bytes to the existing
`127.0.0.1:1087` HTTP proxy without decrypting TLS. It accepts private/loopback
client addresses by default; `--allow-client DEVICE_IP` restricts it to a device.
Allow incoming connections if macOS requests firewall permission. The foreground command does not modify the original proxy application. For persistent use, install the supervised service below.

`0.0.0.0` is the listening address. Configure the device with the Mac's actual LAN
IPv4 address, never `0.0.0.0` or `127.0.0.1`. `ipconfig getifaddr en0` usually
shows it; confirm the active interface. Reserve the address in your router or
update the device when DHCP changes it. Mac sleep stops the proxy connection.

## Build and configure

Use the pinned Muse SDK and ESP-IDF **6.0.1**, not the game toolchain. `prepare.py`
applies the maintained patch and overlay, including `passport_proxy`. Keep the
SDK token in an ignored private file with mode `0600`. For the project-local
macOS toolchain, `bash experiments/muse-passport/build-macos.sh` builds and verifies
the firmware, taking the token from `.local/sdk-token.txt` or `MUSE_TOKEN_FILE`.
Firmware and build configuration contain the token and must remain private.

Install the newly built firmware with the existing Passport board flashing
workflow only after authorizing flashing. Then close other serial monitors and run:

```sh
# Example addresses: substitute this Mac's LAN IP and the actual serial port.
python3 experiments/muse-passport/configure_proxy.py /dev/cu.usbmodemXXXX 192.168.1.10:18087
python3 experiments/muse-passport/configure_proxy.py /dev/cu.usbmodemXXXX
python3 experiments/muse-passport/configure_proxy.py /dev/cu.usbmodemXXXX off
```

Equivalent USB commands are `>proxy=192.168.1.10:18087`, `>proxy.status` and
`>proxy=off`, each followed by a newline. Configuration is intentionally local
over USB; it does not add unauthenticated Wi-Fi configuration endpoints.
The CLI checks the acknowledgment and reads the stored value back. Opening a
USB port can reset some boards. After configuration, restart with the physical
switch, then check Link becomes Online and test a spoken request.

## Connection and failure behavior

The device sends the original service hostname in CONNECT, so the Mac proxy
resolves it. It then starts the existing TLS handshake inside the tunnel with
the original SNI, certificate bundle and hostname checks. There is no TLS
verification bypass. CONNECT is limited to a 2 KiB response header and the
connection's configured timeout, with a 15-second default. Its header is read
exactly, preserving any bytes that follow it. Internal RAM and stack usage stay
small for this board without PSRAM.

Enabled proxy failures produce `passport_proxy` logs and do not silently fall
back to direct internet access. Check the Mac process, upstream proxy, firewall
and LAN isolation; HTTP 407 means authentication is required and unsupported.
Use `proxy=off` and restart to explicitly restore direct connections. The
configuration survives normal reflashing and restarts; deleting its NVS partition
removes it. Pairing reset does not clear this separate namespace.

The integration wraps synchronous and asynchronous ESP-TLS client entry points
and attaches the tunnel before the original TLS state machine. It uses a private
ESP-IDF structure and is specifically pinned to **6.0.1**. Revalidate against IDF
source and run a real build before upgrading. Host tests exercise fragmented
headers, TLS-byte preservation, async reentry, validation, failures, disabling,
bidirectional forwarding and client restriction:

```sh
python3 -m unittest discover -s experiments/muse-passport/tests -p 'test_*.py' -v
```

Mac HTTPS checks prove proxy connectivity, not on-device voice success. Hardware
acceptance still needs the new image, saved configuration, a confirmed cloud
session, and an actual microphone request/reply.

## Passport Link memory profile

This no-PSRAM board reserves a 41 KiB session workspace on paired boots, before UI and Wi-Fi allocations. Unpaired boots leave that memory available for BLE provisioning. Incoming
ciphertext is decrypted in place; service reassembly remains separate, including
16 KiB chat payloads. Outbound service encoding and WebSocket output reuse one
buffer only after serialization finishes. Partial inbound WebSocket data never
shares the outbound buffer. Registration prefixes are staged in envelope scratch
before service encoding, so sharing the output buffer cannot overwrite them.
Session cleanup clears the workspace.

Outgoing TLS records are limited to 2 KiB; incoming records retain 16 KiB support.
The DMA reserve is 8 KiB for this board without a network tunnel; the 2 KiB
contiguous-block floor and the TLS allocation-size check remain enabled. Voice
uses 1 KiB base64 chunks and a 2 KiB queue, admitting data before allocating
the queued copy. Validate Link registration and
an actual voice upload after changing these sizes; a TLS connection alone does
not exercise the full memory requirement.

## Persistent macOS service

```sh
python3 experiments/muse-passport/mac_proxy_service.py install
python3 experiments/muse-passport/mac_proxy_service.py status
# Stop automatic restarts and remove login startup:
python3 experiments/muse-passport/mac_proxy_service.py uninstall
```

The per-user `com.musepassport.proxy` LaunchAgent starts at login and restarts after exit with a 10-second throttle. Stop any foreground process using 18087 before installation. Runtime code is copied into `~/Library/Application Support/MusePassportProxy`, so closing the terminal or this checkout does not stop it. Re-run install after changes to deploy the updated copy. Python must remain available at the installed executable path.

Metadata-only logs live in `~/Library/Logs/MusePassportProxy/proxy.log`, capped at 1 MiB plus three backups. No tunnel contents or tokens are logged. Connection failures are isolated, cleanup is bounded, and a connection times out only after 300 seconds without traffic in either direction. An upstream outage closes affected connections; new ones can connect when the upstream returns. Existing TCP sessions cannot survive a killed process.

The service does not start the upstream proxy, prevent Mac sleep, or guarantee device reconnection after an extended outage. Check port 1087, the Mac LAN address and Link registration as well as the launchd PID. A login LaunchAgent is unavailable while the user is logged out. Uninstall retains runtime files and logs for diagnosis.
