# Roadmap

Language: English | [简体中文](../zh_CN/roadmap.md)

LibGS is currently a pre-1.0 C++20 library. Implemented modules are documented
in the [documentation index](README.md); the items below are unimplemented
areas, not promised releases or dates.

## Open areas

- HTTP/2 and HTTP/3 transports
- WebSocket extended CONNECT over HTTP/2 or HTTP/3
- WebSocket extensions beyond `permessage-deflate`
- Higher-level reconnect, liveness-deadline, and message-routing policies
- Installed CMake package configuration and a stable 1.0 API contract

## Constraint

New work should preserve the dependency direction documented in the index:
Core at the base, WebSocket above HTTP, and Utilities independent of protocol
modules.
