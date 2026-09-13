# HTTP 客户端与服务端

语言：[English](../en/http.md) | 简体中文

HTTP 模块实现 HTTP/1.0 和 HTTP/1.1 协议工具、客户端、服务端、连接与路由，
并提供可选的 TLS 和 gzip 支持。链接时使用 `gs.http`；它公开依赖 `gs.coro` 和
`gs.core`。

WebSocket 帧不属于本模块。服务端提供了通用的 HTTP Upgrade 连接移交边界，
由独立且已经实现的 [`gs.websocket`](websocket.md) 模块使用。

## 头文件索引

| 头文件 | 用途 |
| --- | --- |
| `<libgs/http/client.h>` | 高层 HTTP 客户端 |
| `<libgs/http/server.h>` | HTTP 和可选 HTTPS 服务端别名 |
| `<libgs/http/client/connection_pool.h>` | 可复用连接池 |
| `<libgs/http/protocol/types.h>` | 方法、状态码、重定向和请求目标形式 |
| `<libgs/http/protocol/utils.h>` | 解析器、生成器、cookie、表单、压缩和协议工具 |
| `<libgs/http.h>` | 客户端和服务端聚合头 |

## 可选功能

配置 LibGS 时可以启用 TLS 和 gzip：

```shell
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_OPENSSL_SUPPORT=ON \
  -DLIBGS_HTTP_ZLIB_SUPPORT=ON
cmake --build build --parallel
```

`LIBGS_OPENSSL_SUPPORT` 会启用 HTTPS 客户端连接、TLS 连接类型和 HTTPS 服务端
别名。`LIBGS_HTTP_ZLIB_SUPPORT` 会启用 HTTP gzip 自动压缩与解压缩。这两个选项
都要求配置时能够找到相应的系统依赖。

## HTTP 服务端

下面的服务端监听 8080 端口，并处理 `GET /hello`：

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
            "/hello",
            [](libgs::http::server::context_t &context)
                -> libgs::awaitable<void>
            {
                constexpr std::string_view body = "Hello from LibGS!\n";
                co_await context.response().write(
                    asio::buffer(body), libgs::use_awaitable
                );
            }
        )
        .start();

    return libgs::exec();
}
```

服务端不会创建隐藏的进程线程，也不强制使用单独的事件循环。acceptor 和 service
任务使用兼容的 Asio 执行器；本例通过 `libgs::exec()` 运行默认 context。

### 路由

`on_request` 接收一个或多个 HTTP method 和一条路径规则。规则支持：

- `*`：匹配多个字符；
- `?`：匹配单个字符；
- `{name}`：命名路径参数。

例如，`/users/{user_id}` 可以通过
`context.request().path_arg("user_id")` 读取 `user_id`。request 还提供 method、
HTTP version、target、path、query parameter、header、cookie、endpoint 和 body
读取操作。

使用 `on_default` 处理未匹配任何注册路由的请求。可以通过 `on_server_error` 和
`on_service_error` 处理服务端级错误和单次服务错误。

服务启动后仍可调用 `on_request` 增加路由，或调用 `unbound_request` 删除路由。
这些更新可以与请求处理并发执行：每次单条路由更新都会原子发布，后续请求会看到
更新前或更新后的完整路由表，已经选中处理器的请求则继续使用原处理器。纯文本路径
使用精确索引快速匹配，包含通配符或末尾路径参数的规则仍按上述规则匹配，因此不会
改变路由优先级和处理逻辑。

### 响应

Response API 可以：

- 设置 status、header 和 cookie；
- 写入固定长度或 chunked body；
- 发送文件并处理支持的 range 条件；
- 重定向或发送 `100 Continue`；
- 使用可选 trailing header 结束 chunked 输出；
- 在 zlib 可用时启用自动压缩。

与 Asio I/O 一样，非 detached 的异步写操作会借用 body buffer，直到操作完成。
必须保证存储在 `co_await` 或回调完成之前有效。detached response write 会复制并
持有自己的数据。

### 资源根目录

`server_config::resource_root` 是服务端文件路径的根目录，同时作用于
`response.send_file()` 和 `request.save_file()` 使用的相对路径：

```cpp
auto config = server.config();
config.resource_root = "/srv/my-service";
server.set_config(config);

co_await context.response().send_file(
    "public/index.html", libgs::use_awaitable
);
co_await context.request().save_file(
    "uploads/item.bin", libgs::use_awaitable
);
```

绝对路径不会拼接 root。root 为空时保留原有行为，相对路径以可执行文件目录为
基准。`resource_root` 只负责解析路径，不是静态文件路由或安全沙箱。对于来自
request 的路径，文件操作前必须验证并拒绝 `..` 等目录穿越输入。

对于基于路径调用的 `send_file()`，服务端会缓存不超过 2 MiB 的原始文件内容和
协商产生的 gzip 版本。进程内缓存总量限制为 32 MiB，每次请求都会使用文件大小
与修改时间校验缓存。更大的文件、Range 响应和调用方持有的 stream 仍使用流式
I/O。该策略用有界内存换取热点静态资源免重复读取、免重复压缩。

### 中间件与会话

`basic_aop` 提供协程形式的 `before`、`after` hook 和异常处理。
`basic_ctrlr_aop` 额外提供 controller 风格的 `service` 操作。一条路由可以接收
多个 AOP 对象。

`context.session()` 和 `context.session_or()` 与服务端 session manager 集成。
Session 支持通过继承扩展类型、任意属性、过期时间、生命周期延长、超时回调和
可配置 cookie key。

### HTTP Upgrade 移交

对于有效的 upgrade 请求，`context.hand_over_connection()` 会将底层连接移出
常规 HTTP 请求处理流程。之后由调用方负责协议处理，以及 request 中剩余字节的
处理。这是低层所有权边界；应用通常应调用 `websocket::upgrade()`，无需自行实现
RFC 6455 framing。

## 同步 HTTP 客户端

高层客户端方法默认使用同步形式：

```cpp
#include <libgs/http/client.h>

#include <iostream>
#include <string>

int main()
{
    libgs::http::client client;
    auto context = client.request_get("http://example.com/");

    auto status = context->wait_reply();
    auto body = context->reply()->read<std::string>();

    std::cout << static_cast<unsigned>(status) << '\n';
    std::cout << body;
}
```

默认 token 会在失败时抛出 `std::system_error`。如果需要非抛异常的同步处理，
可以向 request、reply 和 body 操作传入 `std::error_code` 对象。

## 协程 HTTP 客户端

传入 `libgs::use_awaitable`，即可在协程中使用同一套操作：

```cpp
#include <libgs/http/client.h>

#include <iostream>
#include <string>

int main()
{
    libgs::http::client client;

    libgs::dispatch([&client]() -> libgs::awaitable<void>
    {
        auto context = co_await client.request_get(
            "http://example.com/", libgs::use_awaitable
        );
        co_await context->wait_reply(libgs::use_awaitable);

        auto body = co_await context->reply()->read<std::string>(
            libgs::use_awaitable
        );
        std::cout << body;

        libgs::exit();
    });

    return libgs::exec();
}
```

客户端也支持 callback token，其完成签名包含 error code 和操作结果。

## 请求配置

`libgs::http::request_arg` 保存 request header、cookie、chunk attribute 和认证信息，
并提供 Basic、Bearer 和 proxy Basic 认证工具。

`libgs::http::client::req_info` 将 URL、request argument 和单次请求行为组合起来：

```cpp
#include <libgs/http/client.h>

int main()
{
    libgs::http::request_arg arguments;
    arguments.set_header("Accept", "application/json");

    libgs::http::client client;
    libgs::http::client::req_info info(
        "http://example.com/api", arguments
    );
    info.follow_redirects(5).auto_decompress(true);

    auto context = client.request_get(std::move(info));
    return context ? 0 : 1;
}
```

`set_proxy` 为请求选择 HTTP 代理。只有调用 `follow_redirects` 后才会启用自动重定向。
自动解压默认开启，但只有在编译时启用 zlib 后才能执行 gzip 解码。

高层客户端维护一个 cookie jar 和一个连接池。Reply 消费完成后，`Set-Cookie`
会保存到 jar，后续匹配 URL 的请求会自动携带相应 cookie。可以通过
`client::cookie_store()` 访问该 jar。当应用需要不同的数量限制、超时、路由或
执行器所有权时，可以提供自定义 pool 或 executor。

## Body 与文件

对于允许 body 的 method，request context 会提供 body write。Reply 对象可以将
数据读入调用方 buffer、类型化 buffer container、byte vector 或文件。高层客户端
还提供上传和下载工具，并支持可选进度回调。

HTTP 写操作返回的字节数只统计调用方提供的 body（文件操作统计源文件数据），不含
HTTP 头、chunk framing 或 multipart 边界。使用 `error_code&` 或异步完成参数时，
错误与已经写入的部分 body 字节数会同时保留；header-only 操作和 `chunk_end()` 返回
零。

如果希望复用连接，应在释放 request context 之前读完或保存完整的 reply body。
Client、context、reply、connection 和 pool 都提供取消操作。

## HTTPS

启用 OpenSSL 支持后，`https://` URL 使用相同的高层客户端接口。HTTPS 服务端通过
`libgs::https::server` 使用 `asio::ssl::context`，由应用配置证书、私钥、协议选项
和验证策略。

TLS 策略与具体应用有关。未经审核，不要将开发环境的证书或验证设置直接用于
生产环境。

## 协议范围

HTTP 模块目前实现 HTTP/1.0 和 HTTP/1.1，尚不支持 HTTP/2 和 HTTP/3。
RFC 6455 framing 由独立的 [`gs.websocket`](websocket.md) 模块通过 HTTP/1.1
Upgrade 提供。

## 相关示例

- [`examples/http/client_sync.cpp`](../../examples/http/client_sync.cpp)
- [`examples/http/client_awaitable.cpp`](../../examples/http/client_awaitable.cpp)
- [`examples/http/client_cookies.cpp`](../../examples/http/client_cookies.cpp)
- [`examples/http/client_file.cpp`](../../examples/http/client_file.cpp)
- [`examples/http/protocol.cpp`](../../examples/http/protocol.cpp)
- [`examples/http/server.cpp`](../../examples/http/server.cpp)
- [`examples/http/server_aop.cpp`](../../examples/http/server_aop.cpp)
- [`examples/http/server_file.cpp`](../../examples/http/server_file.cpp)
- [`examples/http/server_session.cpp`](../../examples/http/server_session.cpp)
- [`examples/http/https_server.cpp`](../../examples/http/https_server.cpp)
