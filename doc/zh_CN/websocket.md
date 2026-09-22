# WebSocket

语言：[English](../en/websocket.md) | 简体中文

Target：`gs.websocket`。启用 `LIBGS_BUILD_WEBSOCKET`，包含
`<libgs/websocket.h>`，并链接 `gs.websocket`。该模块在 HTTP/1.1 上实现
RFC 6455，依赖 `gs.http`。

## 公共 API

| 头文件 | 作用 |
| --- | --- |
| `<libgs/websocket/client.h>` | WS/WSS 打开、重定向、Cookie、代理、诊断 |
| `<libgs/websocket/server.h>` | 自有 Server 与 HTTP Upgrade |
| `<libgs/websocket/stream.h>` | 消息/Frame I/O、控制帧、关闭、生命周期 |
| `<libgs/websocket/retry.h>` | 打开失败后的显式重试 |
| `<libgs/websocket/protocol/...>` | 握手与 Frame 编解码 |
| `<libgs/websocket/types.h>` | Stream、消息、关闭与压缩配置 |

`LIBGS_OPENSSL_SUPPORT` 启用 WSS。WebSocket zlib 支持启用 RFC 7692
`permessage-deflate`。

## 客户端与恢复

`websocket::client` 持有一个 HTTP client。`client.open()` 打开一条连接；自由
函数 `websocket::open(http_client, ...)` 复用应用持有的 HTTP client 及其
connector、Cookie、连接池和代理策略。

`connect_request` 配置 Header/认证、代理、Stream 限制、握手超时、重定向、
子协议与扩展。默认代理策略依次检查 `ws_proxy`/`wss_proxy`、HTTP/HTTPS 代理
变量和 `all_proxy`，并遵循 `no_proxy`。

初次打开只尝试一次。应用判断活动 Stream 已失效后，可以通过 `retry_open()` 对新
连接执行带指数退避、上限、抖动、次数限制、决策回调和观察器的重试。
`retry_open()` 不监控 Stream，不重放消息，不恢复认证/订阅，也不运行业务接收
循环。

## 服务端与 HTTP Upgrade

`websocket::server` 持有 HTTP listener 并处理握手。连接应通过 `accept()`，
或通过 `on_connection()`/`on_default()` 注册处理器交付；同一 Server 不要混用
两种模式。

混合服务使用 `http::server` 路由，检查 `is_upgrade_request()`，再调用
`websocket::upgrade()`。成功结果包含 Stream、不可变请求快照以及选中的子协议/
扩展。

`upgrade_options` 配置 Stream 限制、握手超时、响应 Header、协议/扩展白名单，
以及同步或 awaitable 验证器和选择器。同步 `upgrade()` 不能执行 awaitable 回调。

## Stream

| 操作 | 结果 |
| --- | --- |
| `read<Buffer>()` | 一个完整文本或二进制消息 |
| `consume()` | 以临时分块交付一个消息 |
| `read_frame<Buffer>()` | 一个数据帧 |
| `write_text()`、`write_binary()`、`write()` | 一个完整消息 |
| `write_frame()` | 一个显式分片的数据帧 |
| `ping()`、`pong()` | 手动控制帧 |
| `close()` | RFC 关闭握手 |
| `shutdown()` | 立即关闭传输 |
| `wait_written()` | 观察已接收的排队写入 |
| `wait_closed()`、`on_closed()` | 观察最终关闭信息 |

同一 Stream 的规则：

- `read()`、`consume()`、`read_frame()` 中最多有一个处于活动状态。
- 一个读和一个写可以重叠；消息写入通过按字节数和操作数限制的队列串行化。
- `message_chunk::body` 只在对应 `consume()` 回调期间有效。
- 控制回调由活动读操作处理控制帧时触发。
- 完整消息读取会组合 continuation frame 并检查消息限制；Frame 读取保留帧边界。
- 多线程运行事件循环时，同一 Stream 的所有访问必须放在一个 strand 上。

`stream_config` 控制 Frame/消息限制、缓冲、分片、写队列、关闭超时、保活与压缩。
`ping_interval` 大于零时启用自动 Ping/Pong，且必须有活动读处理 Pong；只有间隔为
零时才能手动调用 `ping()`/`pong()`。

## 压缩与边界

`permessage-deflate` 必须由双方协商。`permessage_deflate_options` 控制窗口与
上下文策略，`compression_config` 控制自动压缩与阈值。

本模块支持掩码、分片、UTF-8 校验、Ping/Pong/Close、WS/WSS、重定向、Cookie、
子协议、HTTP/SOCKS5 代理、超时、取消和有界写入。不提供 HTTP/2 或 HTTP/3 上的
WebSocket、`permessage-deflate` 之外的扩展、持久重放或应用路由。

通用所有权和并发规则见[执行与 I/O 模型](io-model.md)。

## 示例

[WebSocket 示例索引](../../examples/README.md#websocket)包含自有客户端/服务端、
混合 HTTP Upgrade、重试、代理、协议编解码与 WSS。
