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
| WebSocket | 开发中 | 已有协议 frame、握手及基础 stream 发送 IO；读取状态机仍在实现 |

## 规划中的 WebSocket 支持

WebSocket 是下一个明确规划的协议扩展。计划将其构建为协议模块，并与现有 LibGS
执行模型、连接、TLS 和 completion-token 约定集成。

预期设计范围包括：

- 客户端和服务端角色；
- `ws://` 和 `wss://` 传输；
- HTTP Upgrade 验证和连接移交；
- text、binary、continuation、ping、pong 和 close frame；
- fragmented message 组装；
- callback 和 coroutine 完成方式；
- cancel、timeout、queue 和 backpressure 行为；
- 明确的 buffer 与移交连接所有权。

当前已落地的基础约定：`read()` 只返回完整 data message；Ping/Pong 由控制事件
暂存并继续读取；收到 Close 后后续读取返回 `0 + eof`。同一方向的并发 IO 由调用方
负责管理，底层 parser 保持增量、借用输入缓冲区，消息仅在需要聚合时复制。

## 现有 Upgrade 准备

HTTP 服务端已经通过 `service_context::hand_over_connection()` 提供协议无关的
所有权边界。它允许完成 HTTP Upgrade 的连接和剩余输入离开常规 HTTP 请求生命
周期，从而降低 HTTP parser 与未来升级协议之间的耦合，但它不会解析或生成
WebSocket frame。

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
- 只有本路线图会将尚未实现的协议功能描述为规划范围。
- 文档代码片段应能在其所在分支上编译。

## 版本策略

LibGS 当前版本为 `0.11.1-dev`，尚未声明 1.0 稳定里程碑。在此之前，协议和工具
API 仍可能随着模块边界和完成语义的完善而调整。
