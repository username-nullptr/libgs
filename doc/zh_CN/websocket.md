# WebSocket 客户端、服务端与 Stream

语言：[English](../en/websocket.md) | 简体中文

`gs.websocket` 模块实现基于 HTTP/1.1 的 RFC 6455 WebSocket 基础版。它提供独立
WS/WSS 客户端与服务端、HTTP 混合应用的 Upgrade helper、协议 codec、完整消息与
流式消息读取、对称的数据帧 IO、控制帧、超时、取消和有界写队列。

链接时使用 `gs.websocket`；它的公开依赖链包含 `gs.http`、`gs.coro` 和
`gs.core`。客户端/服务端可包含 `<libgs/websocket.h>` 聚合头，只使用协议 codec
或 `websocket::stream` 时可包含更具体的头文件。

## 已实现范围

- `ws://`，以及启用 `LIBGS_OPENSSL_SUPPORT` 后的 `wss://` endpoint；
- HTTP/1.1 client/server opening handshake；
- 独立的 `websocket::client` 和 `websocket::server` 所有权模型；
- 通过 `websocket::open()`、`websocket::upgrade()` 混合使用 HTTP/WebSocket；
- text、binary、continuation、Ping、Pong 和 Close frame；
- fragmented message 聚合和可配置的出站自动分片；
- 完整消息 `read()`、单消息流式 `consume()`，以及数据帧级
  `read_frame()`/`write_frame()` 接口；
- 客户端 masking、UTF-8 校验、frame/message 大小限制和协议失败 Close；
- subprotocol 协商、同步/异步 Upgrade validator、redirect、Cookie 和 opening
  diagnostics；
- 可选 RFC 7692 `permessage-deflate`，两个方向都关闭 context takeover；
- 同步操作及 Asio completion-token 完成方式；
- 取消、关闭 deadline、写队列限制和 write barrier。

## 客户端示例

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

`websocket::client` 持有自己的 HTTP client。如果希望普通 HTTP request 与
WebSocket 共用连接策略、Cookie jar 或 connector，可以把 HTTP/1.1 client 传给
自由函数 `websocket::open()`。

`websocket::client_config::no_delay` 是三态配置，默认值为 `true`：`true` 开启
`TCP_NODELAY`，`false` 关闭，`nullopt` 则保留底层 HTTP 连接继承来的设置。自由
函数 `websocket::open(http_client, ...)` 始终保留 HTTP client 的设置。

## 服务端与混合 Upgrade

`websocket::server` 持有 listener，可通过 `accept()` 主动取得连接，或通过
`on_connection()`/`on_default()` handler 接收 stream。同一 server 的两种交付
模式互斥。

已有的 `http::server` route 可以调用 `websocket::upgrade(context, options)`。
成功时 helper 会校验并写出 opening response，从 HTTP 层移交连接及 pending
bytes，然后返回已经 adopt 的 WebSocket stream。

`upgrade_options` 支持 request/Origin validator、subprotocol policy、额外的非
协议 response header、stream 限制和握手 deadline。`request_validator` 与
`origin_validator` 是同步回调；`async_request_validator` 与
`async_origin_validator` 可以在异步 `upgrade()` 或 owned server 握手中挂起。
如果同步 `upgrade()` 配置了异步 validator，会返回
`std::errc::operation_not_supported`。selector 仍为同步回调。

## Stream 行为

`stream::read<Buffer>()` 每次返回一条完整的 text 或 binary message。它会隐藏 TCP
分段和 continuation frame，并在聚合消息时消费控制帧。如果应用需要处理传入的
Ping/Pong，应保持 read 运行；`wait_ctrl()` 只观察控制事件，不会单独启动 transport
read。

`stream::consume()` 每次流式消费一条完整消息。回调收到 `message_chunk`，其中
`first`/`last` 表示消息边界，`offset` 是 chunk 在当前消息中的字节偏移；chunk
大小不超过 `stream_config::read_buffer_size`，实际边界由当前 transport read 决定，
不对应 TCP 分段或 WebSocket frame。`body` 是非持有视图，只在当前回调返回前有效。
回调在 stream executor 上同步执行，必须及时返回；在它返回前，后续 Ping 也无法被
读取。需要长期保留数据时应在回调内复制或转移到应用自己的有界存储。若消息后半段
发生协议错误、取消或 EOF，先前 chunk 已经交付，应用应把最终完成结果作为整条消息
是否成功的提交点。完成结果 `message_info` 包含消息类型和总字节数。

```cpp
auto info = co_await stream.consume(
    [&output](const libgs::websocket::message_chunk &chunk)
    {
        output.write(static_cast<const char*>(chunk.body.data()),
            static_cast<std::streamsize>(chunk.body.size()));
    },
    libgs::use_awaitable
);
```

`stream::read_frame<Buffer>()` 逐个返回 text、binary 或 continuation 数据帧；
`stream::write_frame()` 接受相同的 `basic_data_frame<Buffer>` 结构。结果或参数包含
`fin`、`continuation`，以及 continuation frame 从首帧继承的有效消息类型。
`write_frame()` 校验分片顺序、跨帧 text UTF-8 和整条消息大小；同一分片消息的
frame write 必须等待前一次完成后再发起。frame IO 与完整消息 IO 不可在一条未完成
的分片消息中切换。

`read()`、`read_frame()` 与 `consume()` 不能并发。由于解压会改变 frame/chunk
payload 边界，压缩连接上的 `read_frame()`、`write_frame()` 和 `consume()` 当前会
返回 `std::errc::operation_not_supported`；完整消息 `read()`/`write()` 不受影响。

默认会自动回复传入的 Ping。stream 提供 `ping()` 和 `pong()`，但不负责周期 Ping、
idle timeout 或 Pong deadline；应用可以使用 Asio timer 和 cancellation slot 组合
这些策略。

并发 write 会在 frame 边界串行化，并受 `stream_config` 队列上限约束。
`wait_written()` 可观察此前接受的 write（包括 detached write）产生的错误。
`close()` 执行 RFC close handshake；`shutdown()` 会立即中止操作并关闭 transport。

stream、client 和 server 遵循 Asio shared-object-unsafe 约定。来自多个线程的调用
必须通过 strand 或外部锁串行化。

## 可选 permessage-deflate

启用 `LIBGS_HTTP_ZLIB_SUPPORT=ON` 时，WebSocket 会自然继承 zlib 支持。HTTP zlib
关闭时，可用 `LIBGS_WEBSOCKET_ZLIB_SUPPORT=ON` 仅为 WebSocket 启用压缩。双方
需要显式选择当前实现的配置：

```cpp
auto compression = libgs::websocket::permessage_deflate_extension();

libgs::websocket::connect_request request("ws://127.0.0.1:8080/echo");
request.extensions = {compression};

libgs::websocket::upgrade_options options;
options.supported_extensions = {compression};
```

只有客户端发出该 offer 时服务端才会选择；服务端不接受压缩时，客户端允许空的
extension response。协商结果可通过 `stream::negotiated_extensions()` 和
`accept_result::handshake.extensions` 查看。

## 当前限制

当前 RFC 7692 配置要求同时包含 `server_no_context_takeover` 与
`client_no_context_takeover`。context takeover、window bits 协商、其他参数组合及
其他 WebSocket extension 会作为不支持的配置拒绝。

目前也不支持 HTTP/2、HTTP/3 extended CONNECT、WebSocket 专用 proxy 字段、自动
keepalive、重连和应用消息路由。注入的 HTTP connector 所提供的代理或路由行为仍可
继续使用。

## 相关资料

- [WebSocket 设计与可观察行为](../../libgs/websocket/design.md)
- [示例](../../examples/websocket)
- [功能测试](../../test/functional/websocket)
- [项目路线图](roadmap.md)
