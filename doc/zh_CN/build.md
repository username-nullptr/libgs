# 构建与配置

语言：[English](../en/build.md) | 简体中文

本页集中说明构建输入与应用集成。第一次构建可以直接从[快速入门](getting-started.md)
开始。

## 工具链与依赖

| 组件 | 要求 |
| --- | --- |
| CMake | 3.15 或更高版本 |
| 语言 | C++20 |
| GCC | 13 或更高版本 |
| Clang | 17 或更高版本 |
| MSVC | 19.30 或更高版本 / Visual Studio 2022+ |

Standalone Asio、spdlog 与 nlohmann/json 已包含在 `3rd_party/` 中。只有启用对应
功能时才需要下列系统依赖：

| 依赖 | 功能 |
| --- | --- |
| OpenSSL | TLS 连接、HTTPS 与 WSS |
| zlib | HTTP gzip 与 WebSocket `permessage-deflate` |
| liburing | Linux 上的 Asio io_uring 后端 |

## 模块选择

| CMake 开关 | 默认值 | Target | 要求 |
| --- | :---: | --- | --- |
| Core 始终构建 | ON | `gs.core` | — |
| `LIBGS_BUILD_CORO` | ON | `gs.coro` | Core |
| `LIBGS_BUILD_HTTP` | OFF | `gs.http` | Coroutines 必须为 ON |
| `LIBGS_BUILD_WEBSOCKET` | OFF | `gs.websocket` | HTTP 必须为 ON |
| `LIBGS_BUILD_UTILITIES` | OFF | `gs.utils` | Coroutines 必须为 ON |

启用高层模块却关闭其底层依赖时，CMake 会拒绝该配置。模块 Target 会公开传递依赖，
因此源码树集成只需链接应用直接使用的最高层模块。

## 功能与工具链开关

| CMake 开关 | 默认值 | 可用条件 | 作用 |
| --- | :---: | --- | --- |
| `LIBGS_BUILD_STATIC` | OFF* | 所有平台 | 构建静态库而不是共享库 |
| `LIBGS_ADD_LIBRARY_VERSION` | ON | 共享库构建 | 为共享库文件名添加版本信息 |
| `LIBGS_OPENSSL_SUPPORT` | OFF | OpenSSL | 启用 TLS 连接类型、HTTPS 与 WSS |
| `LIBGS_HTTP_ZLIB_SUPPORT` | OFF | HTTP + zlib | 启用 HTTP gzip；WebSocket 继承 zlib 支持 |
| `LIBGS_WEBSOCKET_ZLIB_SUPPORT` | OFF | WebSocket + zlib | 只启用 WebSocket `permessage-deflate` |
| `LIBGS_BUILD_UTILITIES_SBUS_UDP` | ON | Utilities | 启用基于 Asio 的 UDP 多播软总线接口 |
| `LIBGS_UTILS_SBUS_DEFAULT_INTERFACE` | `local` | Utilities | 为未指定接口的软总线 API 选择 `local` 或 `udp` |
| `LIBGS_IO_URING_SUPPORT` | OFF | Linux + liburing | 使用 Asio io_uring 后端并关闭 epoll |
| `LIBGS_USE_LIBCXX` | OFF | Clang | 使用 libc++ 编译和链接 |
| `LIBGS_USE_LLD` | OFF | Clang | 使用 lld 链接 |
| `LIBGS_ENABLE_LTO` | OFF | GCC | 启用链接时优化 |
| `LIBGS_BUILD_EXAMPLES` | OFF | 已启用模块 | 构建仓库示例 |
| `LIBGS_HEAVY_COMPILE_JOBS` | 平台默认值 | 支持的构建生成器 | 限制 HTTP/WebSocket 并行编译数；`0` 表示不限制 |
| `LIBGS_LOW_MEMORY_DEBUG_INFO` | OFF | GCC | 减少调试信息以降低编译内存占用 |

只有在启用 WebSocket 且关闭 HTTP zlib 时，才会提供
`LIBGS_WEBSOCKET_ZLIB_SUPPORT`。请通过 CMake 配置功能，不要在应用代码中自行
定义生成的配置宏。

*Windows GNU 工具链如果找不到共享 `libstdc++-6.dll`，默认使用静态构建；此时显式
请求共享构建会失败。*

测试、压力、Fuzz、性能和 Sanitizer 开关集中在[测试指南](../../test/README.md)。

## 常用构建配置

默认的 Core 与 Coroutines：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

不启用可选系统依赖，构建全部模块与示例：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

构建 HTTP、WebSocket、TLS 与压缩支持：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_OPENSSL_SUPPORT=ON \
  -DLIBGS_HTTP_ZLIB_SUPPORT=ON
cmake --build build --parallel
```

Visual Studio 等多配置生成器需要在构建和安装时选择配置：

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=C:/libgs
cmake --build build --config Release --parallel
cmake --install build --config Release
```

## 构建产物与安装

构建树中的产物位于：

| 产物 | 目录 |
| --- | --- |
| 共享库与运行时二进制 | `build/output/bin` |
| 静态库/导入库 | `build/output/lib` |
| 示例 | `build/output/examples/<module>` |
| 生成的配置头文件 | `build/output/config_include` |

不使用默认系统前缀时，显式指定安装目录：

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/path/to/libgs-install
cmake --build build --parallel
cmake --install build
```

安装树包含公共头文件、已启用模块对应的库，以及
`LIBGS_BUILD_EXAMPLES=ON` 时构建的示例。

## 使用源码树

```cmake
add_subdirectory(path/to/libgs)

add_executable(my_app main.cpp)
target_compile_features(my_app PRIVATE cxx_std_20)
target_link_libraries(my_app PRIVATE gs.http)
```

LibGS 选项需要在 `add_subdirectory` 之前设置；上例要求外层构建启用
`LIBGS_BUILD_HTTP=ON`。

## 使用安装树

安装树不包含 CMake package config，需要显式查找头文件与每个依赖库：

```cmake
find_path(LIBGS_INCLUDE_DIR NAMES libgs.h REQUIRED)
find_library(LIBGS_CORE_LIBRARY NAMES gs.core REQUIRED)
find_library(LIBGS_CORO_LIBRARY NAMES gs.coro REQUIRED)
find_library(LIBGS_HTTP_LIBRARY NAMES gs.http REQUIRED)

add_executable(my_app main.cpp)
target_compile_features(my_app PRIVATE cxx_std_20)
target_include_directories(my_app PRIVATE "${LIBGS_INCLUDE_DIR}")
target_link_libraries(my_app PRIVATE
  "${LIBGS_HTTP_LIBRARY}"
  "${LIBGS_CORO_LIBRARY}"
  "${LIBGS_CORE_LIBRARY}"
)
```

如果启用了可选系统依赖，还要在应用的链接配置中提供对应库。
