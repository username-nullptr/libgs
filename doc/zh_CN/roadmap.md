# 路线图

语言：[English](../en/roadmap.md) | 简体中文

LibGS 当前是尚未到达 1.0 的 C++20 库。已实现模块见[文档索引](README.md)；
下列内容只是尚未实现的方向，不代表版本或日期承诺。

## 未实现方向

- HTTP/2 与 HTTP/3 传输
- 基于 HTTP/2 或 HTTP/3 extended CONNECT 的 WebSocket
- `permessage-deflate` 以外的 WebSocket 扩展
- 高层会话守护、跨连接持久消息队列、可靠重放与应用消息路由策略
- 安装版 CMake package config 与稳定的 1.0 API 契约

## 约束

新增功能应保持文档索引中的依赖方向：Core 位于底层，WebSocket 位于 HTTP
之上，而 Utilities 不依赖协议模块。
