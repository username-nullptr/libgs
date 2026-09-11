# 项目路线图

语言：[English](../en/roadmap.md) | 简体中文

LibGS 的目标是成为可复用的异步应用基础库，而不是绑定到单一网络协议的库。
本文档将当前实现与规划方向明确分开。

路线图条目表达的是方向，并不代表发布日期或兼容性承诺。

## 当前实现

| 领域 | 状态 | 范围 |
| --- | :---: | --- |
| Core 运行时 | 已实现 | 事件循环、调度、定时器、completion token 和 executor-aware API |
| Core 通用设施 | 已实现 | Value、INI、参数解析、算法、容器、MIME 检查和系统工具 |
| 协程支持 | 已实现 | Awaitable wait、同步原语和执行器切换 |
| HTTP/HTTPS | 已实现 | HTTP/1.0 和 HTTP/1.1 客户端、服务端、协议工具、TLS 和可选 gzip |
| 应用工具 | 已实现 | 日志、设置、信号、观察者、模块、进程，以及默认使用进程内传输的可扩展软总线 |
| WebSocket | 已实现 | HTTP/1.1 WS/WSS、opening handshake、消息/帧/控制 IO、分片、可选的受限 `permessage-deflate`、关闭、取消、超时、队列和背压 |

## WebSocket 范围与规划扩展

`gs.websocket` 已实现 RFC 6455 的 HTTP/1.1 基础版，并与 LibGS 现有的执行模型、
连接、TLS 和 completion-token 约定集成。

已经实现的基础范围包括：

- 客户端和服务端角色；
- `ws://` 和 `wss://` 传输；
- HTTP Upgrade 验证和连接移交；
- text、binary、continuation、ping、pong 和 close frame；
- fragmented message 组装；
- 数据帧级读取及完整消息读取；
- Upgrade 期间的异步 request 与 Origin 校验；
- 可选 RFC 7692 `permessage-deflate`，关闭两个方向的 context takeover；
- callback 和 coroutine 完成方式；
- cancel、timeout、queue 和 backpressure 行为；
- 明确的 buffer 与移交连接所有权。

`read()` 返回完整 data message，`read_frame()` 在未协商 extension 时返回单个
数据帧；读取过程中 Ping/Pong 由控制事件保留；正常 Close 后继续读取会返回 EOF。并发操作必须遵循
stream 文档中的串行化约定。底层 parser 采用增量解析并借用输入缓冲区，只在
聚合消息和协议变换需要所有权时复制 payload。

下列能力不属于当前实现范围，仍在规划中：

- 通用 extension capability registry，以及包括 context takeover 和 window bits
  协商在内的其他 RFC 7692 参数配置；
- HTTP/2、HTTP/3 extended CONNECT 传输；
- 可选的 WebSocket 层代理配置与认证；
- 应用层自动 keepalive、重连和消息路由工具。

## 现有 Upgrade 准备

HTTP 服务端已经通过 `service_context::hand_over_connection()` 提供协议无关的
所有权边界。它允许完成 HTTP Upgrade 的连接和剩余输入离开常规 HTTP 请求生命
周期，使 HTTP parser 与升级后的协议保持独立。应用通常应调用
`websocket::upgrade()`，由它完成 RFC 6455 握手并将移交的连接纳入 WebSocket
stream。

## 架构方向

未来协议模块应保持现有分层：

1. `gs.core` 负责执行模型、错误和通用设施。
2. 纯头文件 coroutine helper 提供非阻塞同步。
3. 协议模块负责 parser、generator、连接行为、客户端和服务端，不重新定义 Core
   运行时。
4. `gs.utils` 提供可选的应用层服务，不成为协议模块的依赖。

新的异步 API 应继续与 Asio executor 和 completion token 保持一致。同步便捷 API
应明确说明阻塞和错误行为。

## 文档策略

- 根 README 描述稳定的项目范围和已实现模块。
- 模块文档描述源码树中已经存在的 API。
- 规划中的协议工作应与已经实现的基础范围分开标注。
- 文档代码片段应能在其所在分支上编译。

## 版本策略

尚未声明 1.0 稳定里程碑。在此之前，协议和工具
API 仍可能随着模块边界和完成语义的完善而调整。
