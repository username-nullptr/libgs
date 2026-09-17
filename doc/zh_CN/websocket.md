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
| `<libgs/websocket/retry.h>` | 显式重试失败的连接建立操作 |
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

### 显式连接恢复

连接是否失效由业务判断。因此首次连接仍由用户直接调用 `client.open()`，只尝试
一次；库也不接管业务读取循环。当业务确认当前 Stream 已不可用后，可以先暂停业务，
再显式调用 `retry_open()`：

```cpp
libgs::websocket::client client;
auto stream = co_await client.open(
    libgs::websocket::connect_request("wss://example.test/events"),
    libgs::use_awaitable); // 首次只尝试一次

auto error = co_await run_business(stream);
if (connection_is_invalid(error)) {
    pause_business();

    libgs::websocket::retry_open_options options;
    options.initial_delay = std::chrono::milliseconds(500);
    options.max_delay = std::chrono::seconds(30);
    options.max_attempts = 10;

    auto recovered = co_await libgs::websocket::retry_open(
        client,
        [](const libgs::websocket::retry_open_context &previous) {
            return refreshed_connect_request(previous);
        }, options, libgs::use_awaitable);

    stream = std::move(recovered.stream);
    co_await restore_authentication_and_subscriptions(stream);
    resume_business();
}
```

`retry_open()` 只重试指定 `client` 的 `open()`；它不会监视 Stream、判断业务错误、
运行会话回调或重放消息。调用方可以传固定 `connect_request`，也可以传同步或可等待
的 request factory。factory 会收到上一次失败信息，因此每次尝试前都可以刷新 URL、
认证 Header 或 Token。

`retry_open_options` 配置指数退避、上限、抖动、总尝试次数、决策器和观察器；
`max_attempts = 0` 表示不限次数。第一次尝试立即进行，失败后才退避。默认重试临时
网络错误及 HTTP 408/429/5xx；协议、认证和 TLS 错误会停止。成功结果包含新的
Stream、尝试次数、最终 opening 诊断，以及存在时的上一次失败。取消 completion
token 关联的 cancellation slot 可以停止正在进行的连接或退避等待。

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
| `ping()`、`pong()` | 自定义保活模式下的显式控制帧 |
| `close()` | RFC Close handshake |
| `shutdown()` | 立即关闭传输层 |
| `wait_written()` | 已接收队列写入的完成状态/错误 |
| `wait_closed()`、`on_closed()` | 最终关闭信息 |

关键规则：

- `read()`、`consume()`、`read_frame()` 同时只能有一个处于活动状态。
- `message_chunk::body` 只在对应 `consume()` 回调期间有效。
- Ping/Pong 与关闭回调只观察活动读操作处理到的控制流量，不会自行发起传输层读。
- `on_ping()` 和 `on_pong()` 接受同步或返回 `awaitable` 的回调；返回值不受限制且
  会被忽略。参数为可变的 `control_payload&`；`text()` 提供便捷的字符串视图，
  `bytes()` 提供可变/只读字节视图，`as_mutable_buffer()` 和
  `as_const_buffer()` 提供 Asio buffer 视图，`assign()` 或字符串赋值可以替换内容。
  这些视图只在 payload 未被调整大小且回调尚未返回时有效。自动模式下，
  `on_ping()` 修改后的 payload 会作为自动 Pong 的 payload。
- 协程控制回调必须由异步 `read()`、`consume()` 或 `read_frame()` 驱动；同步读
  遇到协程控制回调会以 `std::errc::operation_not_supported` 失败。
- 完整消息读取会组装 continuation frame 并检查消息上限；Frame 读取保留数据帧边界。
- 写入会串行化，并受 `max_queued_write_bytes` 与
  `max_queued_write_operations` 限制。
- Stream 遵循 [Asio 兼容的 I/O 模型](io-model.md)：不同 stream 可以并行，共享同一个
  stream 不安全。事件泵有多个线程时，让 stream 以及访问它的所有 handler/协程使用
  同一个 strand；只锁住发起调用无法串行化内部 read、write、timer 和 close 完成。

`stream_config` 设置 Frame/消息上限、读缓冲区、发送分片大小、写队列限制、
自动 Ping/Pong、关闭超时与压缩策略。正值 `ping_interval` 同时表示自动 Ping
周期和匹配 Pong 的期限；每个 Ping 携带唯一 8 字节 payload。活动读操作必须在
下一个周期前处理到匹配 Pong，否则计为一次连续超时。`pong_timeout_retries = N`
允许前 N 次连续超时继续发送新 Ping，第 N+1 次使 Stream 以
`asio::error::timed_out` 失败；匹配 Pong 会清零计数，默认值 0 表示首次超时即失败。

`ping_interval = 0` 同时关闭自动 Ping 和自动 Pong，由应用使用 `ping()`、`pong()`
实现自己的保活流程。`ping_interval > 0` 时调用显式 `ping()` 或 `pong()` 会返回
`std::errc::operation_not_permitted`，避免手工控制帧破坏自动保活状态机。

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
扩展、跨连接透明消息重放或应用消息路由。

## 示例

- [客户端](../../examples/websocket/client.cpp)
- [显式连接恢复](../../examples/websocket/retry_open.cpp)
- [服务端](../../examples/websocket/server.cpp)
- [混合 HTTP 客户端](../../examples/websocket/mixed_http_client.cpp)
- [混合 HTTP 服务端](../../examples/websocket/mixed_http_server.cpp)
- [代理客户端](../../examples/websocket/proxy_client.cpp)
- [协议编解码](../../examples/websocket/protocol.cpp)
- [WSS 客户端](../../examples/websocket/wss_client.cpp)
- [WSS 服务端](../../examples/websocket/wss_server.cpp)
