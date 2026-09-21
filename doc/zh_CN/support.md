# 支持范围

语言：[English](../en/support.md) | 简体中文

本页说明已交付代码的边界。各模块的具体行为和限制以对应模块指南为准。

## 语言与构建集成

| 范围 | 支持 | 未提供的集成方式 |
| --- | --- | --- |
| C++ | C++20；GCC 13+、Clang 17+ 或 MSVC 19.30+ | 更早的编译器与语言模式 |
| 库形态 | 共享库或静态库 | Header-only 使用方式 |
| 源码树 CMake | 通过 `add_subdirectory` 使用模块 Target | — |
| 安装树 | 公共头文件与已启用模块的库 | CMake package config 与 imported target |
| 事件运行时 | 进程级默认 `asio::io_context` 或应用自己的 executor | 独立于 Asio 的执行模型 |

公共 API 尚未到达 1.0，在不同版本之间可能变化。

## 平台相关能力

| 能力 | 可用条件 |
| --- | --- |
| POSIX 系统工具 | Unix-like 构建 |
| Windows 系统工具 | Windows 构建 |
| TLS | 配置 OpenSSL 的构建 |
| io_uring | 配置 liburing 的 Linux 构建 |
| libc++ 与 lld 开关 | Clang 构建 |
| LTO 开关 | GCC 构建 |

底层能力不可用时，部分操作会返回平台错误。每项功能的 API 指南会说明其返回值与
错误模型。

## 协议

| 范围 | 支持 | 未提供 |
| --- | --- | --- |
| HTTP | HTTP/1.0 与 HTTP/1.1 客户端、服务端、解析、生成、文件、会话、重定向、Cookie 与代理 | HTTP/2 与 HTTP/3 传输 |
| HTTPS | 使用应用配置的 OpenSSL context 承载 HTTP | 由 LibGS 接管的自动证书策略 |
| HTTP 压缩 | `LIBGS_HTTP_ZLIB_SUPPORT=ON` 时支持 gzip | 不依赖 zlib 的压缩 |
| WebSocket | RFC 6455 客户端、服务端、Upgrade、Frame、消息、Mask、分片、控制帧、子协议与有界写队列 | 基于 HTTP/2 或 HTTP/3 的 WebSocket |
| WebSocket 压缩 | 启用 WebSocket zlib 后支持 RFC 7692 `permessage-deflate` | 其他 WebSocket 扩展 |
| WebSocket 代理 | 客户端 API 提供 HTTP forward/CONNECT 与 SOCKS5 路径 | 由库管理的系统级代理服务 |
| UDP 软总线 | 带版本的 IPv4 多播帧、进程/LAN/路由作用域、Topic 路由、按来源入口限流、有界负载重组与有界回调投递 | 可靠投递、认证、加密以及由 LibGS 提供多播路由 |

WebSocket API 管理单条连接。应用层会话守护、跨连接持久队列、可靠消息重放和业务
路由由应用负责。

## 并发契约

- 不同 I/O 对象可以并发运行。
- 除非模块指南另有说明，同一个有状态 I/O 对象必须串行访问。
- 协程同步对象支持多个等待者，但对象必须比等待者存活更久。
- 发出取消请求后，仍需保留借用的 Buffer 与状态，直到 completion handler 执行。

完整的逐模块规则见[执行与 I/O 模型](io-model.md)。

## 模块依赖边界

Core 是基础模块；Coroutines 依赖 Core；HTTP 与 Utilities 依赖 Coroutines；
WebSocket 依赖 HTTP。Utilities 不依赖协议模块。
