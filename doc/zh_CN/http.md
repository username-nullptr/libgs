# HTTP

语言：[English](../en/http.md) | 简体中文

`gs.http` 实现 HTTP/1.0 与 HTTP/1.1。链接 `gs.http` 即可；它的公共依赖是
`gs.coro`。

## 公共结构

| 头文件区域 | 作用 |
| --- | --- |
| `<libgs/http/client.h>` | 高层客户端、请求上下文、响应、Cookie、连接池、连接器与代理 |
| `<libgs/http/server.h>` | HTTP/HTTPS 服务端、路由、请求/响应、AOP 与会话 |
| `<libgs/http/protocol/...>` | HTTP 值、Header、Cookie、解析器、生成器、Range、表单与压缩 |
| `<libgs/http/utils/...>` | TCP/TLS 连接与 I/O completion 工具 |
| `<libgs/http.h>` | 客户端与服务端聚合头 |

`LIBGS_OPENSSL_SUPPORT` 启用 TLS 类型、HTTPS 客户端与 HTTPS 服务端别名；
`LIBGS_HTTP_ZLIB_SUPPORT` 启用 HTTP gzip。

## 客户端

一次请求分为三步：创建并发送请求、等待响应、消费响应 Body。

```cpp
#include <libgs/http/client.h>

#include <iostream>
#include <string>

int main()
{
    libgs::http::client client;
    std::error_code error;

    auto context = client.request_get("http://127.0.0.1:8080/", error);
    if (error)
        return 1;

    auto status = context->wait_reply(error);
    auto body = context->reply()->read<std::string>(error);
    if (error)
        return 1;

    std::cout << static_cast<unsigned>(status) << '\n' << body;
}
```

默认 token 为同步形式。每一步传入 `libgs::use_awaitable` 可用于协程，也可传入
签名兼容的回调。高层客户端持有连接池与 Cookie Jar；希望复用连接时，应在释放
请求上下文前完整消费响应 Body。

### 请求配置

| 类型 | 控制内容 |
| --- | --- |
| `request_arg` | Header、Cookie、Chunk 属性、Basic/Bearer 认证与代理 Basic 认证 |
| `client::req_info` | URL、请求参数、单次代理、重定向与解压缩 |
| `client_config` | 默认代理与 `TCP_NODELAY` 策略 |
| `connection_pool_config` | 连接池容量与连接生命周期策略 |
| `connector` | 直连、TLS、HTTP CONNECT、SOCKS5 或应用自定义连接 |

`max_redirects` 为零时不跟随重定向。`req_info` 默认开启自动解压，但 gzip
解码需要构建时启用 zlib。

`client_config::default_proxy` 默认为 `use_global_proxy`。内置解析器读取
`http_proxy`、`https_proxy`、`all_proxy` 及其大写形式，并应用 `no_proxy`。
设置 `no_proxy` 可强制直连。单次请求可以继承客户端设置、绕过代理、使用 HTTP
正向代理，或指定 HTTP CONNECT/SOCKS5 `proxy_tunnel`。

客户端还提供上传/下载与进度回调。Body 字节计数不包含 Header、Chunk framing
与 multipart boundary。

## 服务端

```cpp
#include <libgs/http/server.h>

#include <string_view>

int main()
{
    asio::ip::tcp::acceptor acceptor(libgs::get_executor());
    libgs::http::server server(std::move(acceptor));

    server
        .bind({libgs::ip_type::v4, 8080})
        .on_request<libgs::http::method::get>(
            "/hello/{name}",
            [](libgs::http::server::context_t &context)
                -> libgs::awaitable<void>
            {
                auto name = context.request().path_arg("name");
                const auto body = name ? name->to_string() : "unknown";
                co_await context.response().write(
                    asio::buffer(body), libgs::use_awaitable
                );
            }
        )
        .start();

    return libgs::exec();
}
```

路由规则支持字面路径、`*`、`?` 与命名 `{参数}`。使用 `on_default`、
`on_server_error`、`on_service_error` 处理未匹配请求与错误。路由可以附加
`basic_aop` 的 before/after/exception hook，或使用控制器式 `basic_ctrlr_aop`。

请求 API 提供 method、version、target、路径/查询/路径参数、Header、Cookie、
端点以及 Body/文件读取。响应 API 提供状态、Header、Cookie、定长或 Chunked
写入、重定向、文件响应、Range 与可选压缩。

`server_config::resource_root` 是 `request.save_file()` 与
`response.send_file()` 相对路径的根目录。它只负责路径解析，不是安全沙箱；
所有来自请求数据的路径都必须先验证。

通过 `service_context::session()` 与 `session_or()` 使用会话。Session manager
管理 Cookie key、过期与超时处理。

## HTTP Upgrade

`service_context::hand_over_connection()` 会把 Upgrade 连接及待处理字节移出
HTTP 请求生命周期。WebSocket 应用通常应调用
[`websocket::upgrade()`](websocket.md)，而不是直接使用这个底层边界。

## I/O 规则

- 非 detached 异步写入会借用缓冲区直到完成；detached 响应写入会复制数据。
- error-code 与异步写入形式会在出错时保留已处理的 Body 字节数。
- 客户端、请求上下文、响应、连接、连接池以及服务端请求/响应在适用处提供取消。
- TLS 证书、验证与协议策略由应用的 `asio::ssl::context` 配置。

当前不实现 HTTP/2 与 HTTP/3。WebSocket framing 属于独立的
`gs.websocket` 模块。

## 示例

- [同步客户端](../../examples/http/client_sync.cpp)
- [协程客户端](../../examples/http/client_awaitable.cpp)
- [Cookie](../../examples/http/client_cookies.cpp)
- [文件](../../examples/http/client_file.cpp)
- [代理客户端](../../examples/http/proxy_client.cpp)
- [协议编解码](../../examples/http/protocol.cpp)
- [服务端](../../examples/http/server.cpp)
- [AOP](../../examples/http/server_aop.cpp)
- [文件服务端](../../examples/http/server_file.cpp)
- [会话](../../examples/http/server_session.cpp)
- [HTTPS 服务端](../../examples/http/https_server.cpp)
