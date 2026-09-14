# WebSocket

语言：[English](../en/websocket.md) | 简体中文

`gs.websocket` 实现基于 HTTP/1.1 的 RFC 6455，提供协议编解码、消息 Stream、
自有客户端/服务端，以及升级现有 HTTP 连接的辅助接口。

## 公共结构

| 头文件 | 作用 |
| --- | --- |
| `<libgs/websocket/protocol/...>` | Opening handshake 与 Frame 解析/生成 |
| `<libgs/websocket/stream.h>` | 消息、Frame、控制帧、关闭与生命周期 I/O |
| `<libgs/websocket/client.h>` | WS/WSS 建连、重定向、Cookie、代理与诊断 |
| `<libgs/websocket/server.h>` | 自有服务端与 HTTP Upgrade 接口 |
| `<libgs/websocket/types.h>` | Stream、消息、Frame、压缩与关闭配置 |
| `<libgs/websocket.h>` | 客户端与服务端聚合头 |

链接 `gs.websocket`；它公共依赖 `gs.http`。`LIBGS_OPENSSL_SUPPORT` 启用
WSS 与 TLS 服务端别名。

## 客户端

```cpp
#include <libgs/websocket/client.h>

#include <iostream>

int main()
{
    libgs::websocket::client client;

    libgs::dispatch([&client]() -> libgs::awaitable<void>
    {
        auto stream = co_await client.open(
            "ws://127.0.0.1:8080/echo", libgs::use_awaitable
        );
        co_await stream.write_text("hello", libgs::use_awaitable);
        auto reply = co_await stream.read<std::string>(libgs::use_awaitable);
        std::cout << reply.body << '\n';
        co_await stream.close(libgs::use_awaitable);
        libgs::exit();
    });

    return libgs::exec();
}
```

`client` 内部持有 HTTP/1.1 客户端。自由函数
`websocket::open(http_client, ...)` 可复用应用自己的 HTTP 客户端、connector、
Cookie Jar 与连接策略。

`connect_request` 控制 Header/认证、代理、Stream 配置、握手超时、重定向、
子协议与扩展。`open_diagnostics` 保存最终端点和 HTTP 响应，用于检查握手失败。

默认代理策略先读取 `ws_proxy`/`wss_proxy`，再读取对应的 HTTP/HTTPS 代理变量，
最后读取 `all_proxy`，同时遵守 `no_proxy`。显式 `proxy_config` 支持带 Basic
认证的 HTTP 正向/CONNECT 与 SOCKS5；HTTP 代理也支持 Bearer 认证。

## 服务端与 Upgrade

`websocket::server` 持有 HTTP listener 并完成 opening handshake。连接可以通过
`accept()` 交付，也可以交给 `on_connection()`/`on_default()` 注册的路径处理器；
同一个服务端不要混用这两种模式。

混合 HTTP/WebSocket 服务应由 `http::server` 路由，使用
`is_upgrade_request()` 判断后调用 `websocket::upgrade(context, options)`。
升级成功返回 `accept_result`，其中包含 Stream、不可变请求快照以及选中的
子协议/扩展。

`upgrade_options` 控制：

- Stream 配置与握手超时；
- 支持或要求的子协议与扩展；
- 附加响应 Header；
- 同步或可等待的请求/Origin 校验；
- 同步或可等待的子协议/扩展选择。

同步 `upgrade()` 不能执行可等待的校验器或选择器。选中的子协议或扩展也必须
存在于配置的 allowlist 中。

## Stream

| 操作 | 结果 |
| --- | --- |
| `read<Buffer>()` | 一个完整文本或二进制消息 |
| `consume()` | 以临时 Chunk 交付一个消息 |
| `read_frame<Buffer>()` | 一个文本、二进制或 continuation 数据帧 |
| `write_text()`、`write_binary()`、`write()` | 一个完整消息 |
| `write_frame()` | 带显式分片状态的一个数据帧 |
| `ping()`、`pong()` | 显式控制帧 |
| `close()` | RFC Close handshake |
| `shutdown()` | 立即关闭传输层 |
| `wait_written()` | 已接收队列写入的完成状态/错误 |
| `wait_closed()`、`on_closed()` | 最终关闭信息 |

关键规则：

- `read()`、`consume()`、`read_frame()` 同时只能有一个处于活动状态。
- `message_chunk::body` 只在对应 `consume()` 回调期间有效。
- Ping/Pong 与关闭回调只观察活动读操作处理到的控制流量，不会自行发起传输层读。
- 完整消息读取会组装 continuation frame 并检查消息上限；Frame 读取保留数据帧边界。
- 写入会串行化，并受 `max_queued_write_bytes` 与
  `max_queued_write_operations` 限制。
- 多线程共享访问必须通过 strand 或外部锁串行化。

`stream_config` 设置 Frame/消息上限、读缓冲区、发送分片大小、写队列限制、
自动 Ping/Pong、关闭超时与压缩策略。

## 压缩

直接启用 WebSocket zlib，或从 HTTP zlib 继承支持后，可使用 RFC 7692
`permessage-deflate`。双方都必须参与协商：

```cpp
auto extension = libgs::websocket::permessage_deflate_extension();

libgs::websocket::connect_request request("ws://127.0.0.1:8080/echo");
request.extensions = {extension};

libgs::websocket::upgrade_options options;
options.supported_extensions = {extension};
```

`permessage_deflate_options` 控制 context takeover 与 window bits。
`compression_config` 控制自动选择、消息阈值与 zlib level；`write_options` 可为
单个完整消息覆盖压缩策略。

## 实现边界

当前支持客户端/服务端 masking 规则、分片、UTF-8 校验、Ping/Pong/Close、
WS/WSS、重定向、Cookie、子协议协商、`permessage-deflate`、HTTP 代理、
SOCKS5、超时、取消与有界队列。

当前不支持基于 HTTP/2 或 HTTP/3 的 WebSocket、`permessage-deflate` 之外的
扩展、Pong deadline 策略、自动重连或应用消息路由。

## 示例

- [客户端](../../examples/websocket/client.cpp)
- [服务端](../../examples/websocket/server.cpp)
- [混合 HTTP 客户端](../../examples/websocket/mixed_http_client.cpp)
- [混合 HTTP 服务端](../../examples/websocket/mixed_http_server.cpp)
- [代理客户端](../../examples/websocket/proxy_client.cpp)
- [协议编解码](../../examples/websocket/protocol.cpp)
- [WSS 客户端](../../examples/websocket/wss_client.cpp)
- [WSS 服务端](../../examples/websocket/wss_server.cpp)
