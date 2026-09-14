# 快速入门

语言：[English](../en/getting-started.md) | 简体中文

本文介绍 LibGS 当前的源码构建与安装流程。

## 环境要求

| 组件 | 要求 |
| --- | --- |
| CMake | 3.13 或更高版本 |
| 语言标准 | C++20 |
| GCC | 13 或更高版本 |
| Clang | 17 或更高版本，并支持 `std::format` |
| MSVC | 19.30 或更高版本 / Visual Studio 2022 或更高版本，包括 MSVC 19.50 / Visual Studio 2026 |

仓库中已内置 standalone Asio、spdlog 和 nlohmann/json。OpenSSL 和 zlib
是可选的系统依赖。

## 从源码构建

```shell
git clone https://gitee.com/jin-xiaoqiang/libgs.git
cd libgs

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/path/to/libgs-install
cmake --build build --parallel
cmake --install build
```

使用 Visual Studio 等多配置生成器时，需要在构建和安装阶段选择配置：

```shell
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=C:/libgs
cmake --build build --config Release --parallel
cmake --install build --config Release
```

默认构建会生成共享的 `gs.core`、`gs.coro`、`gs.http`、`gs.websocket` 和
`gs.utils` 库。生成文件和二进制文件位于 `build/output` 下。

## 构建选项

| 选项 | 默认值 | 适用范围 | 用途 |
| --- | :---: | --- | --- |
| `LIBGS_BUILD_STATIC` | `OFF` | 所有库 | 构建静态库而不是共享库 |
| `LIBGS_ADD_LIBRARY_VERSION` | `ON` | 共享库构建 | 在库名称中加入项目版本和 ABI 版本 |
| `LIBGS_BUILD_EXAMPLES` | `OFF` | 示例 | 构建当前 CMake 配置已启用的示例 |
| `LIBGS_BUILD_FUZZERS` | `OFF` | 测试 | 构建 Clang libFuzzer parser harness；需要 `BUILD_TESTING=ON` |
| `LIBGS_OPENSSL_SUPPORT` | `OFF` | Core、HTTP 和 WebSocket | 通过 OpenSSL 启用 TLS、HTTPS 与 WSS 支持 |
| `LIBGS_HTTP_ZLIB_SUPPORT` | `OFF` | HTTP 与 WebSocket | 启用 HTTP gzip，并让 WebSocket 继承 zlib 支持 |
| `LIBGS_WEBSOCKET_ZLIB_SUPPORT` | `OFF`* | WebSocket | HTTP zlib 关闭时单独启用 `permessage-deflate` |
| `LIBGS_USE_LIBCXX` | `OFF` | Clang | 使用 libc++ 编译和链接 |
| `LIBGS_USE_LLD` | `OFF` | Clang | 使用 lld 链接 |
| `LIBGS_ENABLE_LTO` | `OFF` | GCC | 启用链接时优化 |

仅在需要时启用可选协议依赖：

```shell
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_OPENSSL_SUPPORT=ON \
  -DLIBGS_HTTP_ZLIB_SUPPORT=ON
cmake --build build --parallel
```

只有 `LIBGS_HTTP_ZLIB_SUPPORT=OFF` 时才提供
`LIBGS_WEBSOCKET_ZLIB_SUPPORT` 选项。启用 HTTP zlib 后，WebSocket 会自动继承
并启用基于 zlib 的受支持压缩配置，无需再设置第二个开关。如果 HTTP 不需要
zlib，可通过 `-DLIBGS_WEBSOCKET_ZLIB_SUPPORT=ON` 仅启用 WebSocket 压缩。

不要手工定义生成的功能宏。应通过 CMake 选项配置，以保证库与安装后的配置
头文件保持一致。

## 构建和运行示例

```shell
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

示例可执行文件位于 `build/output/examples` 的各个子目录中。示例源码分组如下：

- [`examples/core`](../../examples/core)
- [`examples/coro`](../../examples/coro)
- [`examples/http`](../../examples/http)
- [`examples/websocket`](../../examples/websocket)
- [`examples/utils`](../../examples/utils)

启用 `LIBGS_BUILD_EXAMPLES` 后会构建上述全部示例，包括 HTTP 服务端。HTTPS 和
WSS 还需要启用 `LIBGS_OPENSSL_SUPPORT=ON`。运行方法和能力覆盖情况见
[示例指南](../../examples/README.md)。

## 使用已安装的构建

安装过程会将公共头文件和生成的头文件放到 `<prefix>/include`，将共享库放到
平台对应的库目录或运行时目录。项目目前不会安装 CMake package configuration，
因此使用方需要显式提供头文件和库的位置。

下面是使用已安装 HTTP 构建的最小 CMake 配置：

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

如果 CMake 无法找到这些文件，请将安装前缀加入 `CMAKE_PREFIX_PATH`。使用
`gs.utils` 的应用需要同时查找并链接 `gs.utils`、`gs.coro` 和 `gs.core`。使用
WebSocket 的应用应链接 `gs.websocket`、`gs.http`、`gs.coro` 和 `gs.core`。启动
程序时，还应确保平台的运行时加载器能够找到共享库目录。

## 启动默认运行时

```cpp
#include <libgs/core/execution.h>

#include <chrono>

int main()
{
    using namespace std::chrono_literals;

    libgs::post([]
    {
        // 在默认事件循环中立即执行的任务。
    });

    libgs::post(1s, []
    {
        libgs::exit();
    });

    return libgs::exec();
}
```

`libgs::exec()` 会在运行默认事件循环时阻塞。`libgs::exit()` 请求结束运行，
并设置 `exec()` 的返回值。

## 选择错误处理方式

部分 API 同时提供会抛异常的默认同步形式和非抛异常的 error-code 形式。例如：

```cpp
#include <libgs/http/client.h>

int main()
{
    std::error_code error;
    libgs::http::client client;

    auto context = client.request_get("http://example.com/", error);
    if (error)
    {
        // 处理连接或请求错误。
        return 1;
    }

    return context ? 0 : 1;
}
```

协程调用方传入 `libgs::use_awaitable` 并对结果使用 `co_await`。回调形式的完成
签名以对应头文件中的说明为准。

## 后续阅读

- 了解[核心运行时与通用设施](core.md)。
- 使用[协程同步功能](coroutines.md)。
- 构建 [HTTP 客户端或服务端](http.md)。
- 探索[应用工具](utilities.md)。
