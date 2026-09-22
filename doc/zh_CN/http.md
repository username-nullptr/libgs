# HTTP

语言：[English](../en/http.md) | 简体中文

Target：`gs.http`。启用 `LIBGS_BUILD_HTTP`，包含 `<libgs/http.h>`，
并链接 `gs.http`。该模块实现 HTTP/1.0 与 HTTP/1.1，依赖 `gs.coro`。

## 公共 API

| 头文件区域 | 作用 |
| --- | --- |
| `<libgs/http/client.h>` | Client、请求上下文、reply、Cookie、连接池、connector、代理 |
| `<libgs/http/server.h>` | HTTP/HTTPS server、路由、request/response、AOP、session |
| `<libgs/http/protocol/...>` | 值、Header、Cookie、解析器、生成器、Range、Form |
| `<libgs/http/utils/...>` | TCP/TLS connection 与 I/O 辅助 |

`LIBGS_OPENSSL_SUPPORT` 启用 TLS/HTTPS，`LIBGS_HTTP_ZLIB_SUPPORT` 启用
gzip。

## 客户端

一个高层请求分为三步：

1. 调用 `request_get()`、`request_post()` 或其他请求方法。
2. 对返回的请求上下文调用 `wait_reply()`。
3. 读取或保存响应体。

默认 completion token 为同步模式。每一步传入 `libgs::use_awaitable` 可使用协程，
也可以传入兼容回调。如果要复用连接池中的连接，应在释放上下文前完整消费响应体。

| 配置 | 控制内容 |
| --- | --- |
| `request_arg` | Header、Cookie、Chunk 属性、Basic/Bearer 认证、代理认证 |
| `client::req_info` | URL、请求参数、代理、重定向、解压 |
| `client_config` | 默认代理与 `TCP_NODELAY` |
| `connection_pool_config` | 连接池大小与连接生命周期 |
| `connector` | 直连、TLS、HTTP CONNECT、SOCKS5 或自定义连接建立 |

全局代理策略读取大小写形式的
`http_proxy`/`https_proxy`/`all_proxy`，并遵循 `no_proxy`。单个请求可以
继承、绕过或指定 forward/tunnel 代理。

上传/下载辅助返回响应体字节数，不计 Header、Chunk framing 与 multipart 边界。

## 服务端

`http::server` 持有 acceptor，并按方法和路径路由请求。路径规则支持字面值、
`*`、`?` 与命名 `{arguments}`。

| 区域 | 主要 API |
| --- | --- |
| 路由 | `on_request()`、`on_default()` |
| 错误 | `on_server_error()`、`on_service_error()` |
| 中间件 | `basic_aop`、`basic_ctrlr_aop` |
| Request | 方法/版本/target、Header、Cookie、参数、Body 与文件读取 |
| Response | 状态、Header、Cookie、定长/Chunk 写入、重定向、文件、Range |
| Session | `service_context::session()`、`session_or()`、session manager |

`server_config::resource_root` 用于解析文件 API 的相对路径，但不是安全沙箱。任何
来自请求的路径都必须由应用验证。

协议升级时，`service_context::hand_over_connection()` 把连接及待处理字节移出
HTTP 生命周期。WebSocket 服务通常应使用
[`websocket::upgrade()`](websocket.md)。

## I/O 约束

- 同一 connection 最多同时有一个读和一个写；不能重叠两个读或两个写。
- Request/reply 阶段变化，以及每个 client、server、context 对象的访问必须串行化。
- 多线程运行事件循环时，为服务使用 strand，并从该 strand 发起操作。
- 非 detached 异步写借用 Buffer 到完成；detached response 写会持有副本。
- 取消不会在完成处理器执行前释放借用状态。
- 证书、验证和 TLS 策略由应用在 `asio::ssl::context` 中配置。

本模块不实现 HTTP/2 或 HTTP/3。通用规则见[执行与 I/O 模型](io-model.md)。

## 示例

[HTTP 示例索引](../../examples/README.md#http)包含客户端、服务端、文件、Cookie、
Session、AOP、代理、协议编解码与 HTTPS。
