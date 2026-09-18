# HTTP Examples

[Examples index](../README.md) · [HTTP guide](../../doc/en/http.md)

HTTP examples require `LIBGS_BUILD_HTTP=ON`. Protocol encoding is offline;
client and server programs use loopback endpoints by default.

## Programs

| Program | Demonstrates | Arguments / default |
| --- | --- | --- |
| `protocol` | Request generation and incremental response parsing | No arguments; offline |
| `client_sync` | Synchronous request, reply, and body stages | `[url]`; `http://127.0.0.1:8080/hello/LibGS` |
| `client_awaitable` | The same request path with `use_awaitable` | `[url]`; basic server URL |
| `client_cookies` | Cookie jar storage and resend | `[base-url]`; `http://127.0.0.1:8080` |
| `client_file` | Upload and download helpers | `<upload-file> [download-file] [base-url]` |
| `proxy_client` | Per-request forward proxy and optional Basic authentication | `[target-url] [proxy-url] [username] [password]` |
| `server` | Routing, path arguments, cookies, default replies, and server errors | `[port]`; `8080` |
| `server_aop` | Before/after middleware and controller-style handlers | `[port]`; `8081` |
| `server_file` | `resource_root`, file response, upload saving, and byte counts | `<download-file> [port] [upload-file]`; port `8083` |
| `server_session` | Server sessions and session cookies | `[port]`; `8082` |
| `https_server` | TLS context injection and an HTTPS server | `<certificate.pem> <private-key.pem> [port]`; port `8443` |

`https_server` is built only with `LIBGS_OPENSSL_SUPPORT=ON`. The proxy client
defaults to `http://127.0.0.1:3128`, but LibGS does not start a proxy.

## Basic client/server flow

Start the server:

```sh
# Terminal 1
./build/output/examples/http/server
```

Run clients against its routes:

```sh
# Terminal 2
./build/output/examples/http/client_sync
./build/output/examples/http/client_awaitable
./build/output/examples/http/client_cookies
```

Pass a different port to the server and a matching URL to a client:

```sh
./build/output/examples/http/server 9000
./build/output/examples/http/client_sync \
  http://127.0.0.1:9000/hello/LibGS
```

## File transfer

The server exposes `GET /download` and `PUT /upload`. The first server argument
selects the file made available for download; the optional upload path controls
where received data is stored.

```sh
# Terminal 1
./build/output/examples/http/server_file README.md 8083 /tmp/libgs-uploaded.bin

# Terminal 2
./build/output/examples/http/client_file \
  README.md /tmp/libgs-downloaded.md http://127.0.0.1:8083
```

The server's `resource_root` is the directory containing the download file.
Treat request-derived paths as untrusted in an application; `resource_root` is
path resolution, not a security sandbox.

## Middleware and sessions

`server_aop` exposes `/hello` and `/controller` on port 8081:

```sh
# Terminal 1
./build/output/examples/http/server_aop

# Terminal 2
curl http://127.0.0.1:8081/hello
curl http://127.0.0.1:8081/controller
```

`server_session` exposes `/session` on port 8082. Use a cookie jar to observe
the same session across requests:

```sh
# Terminal 1
./build/output/examples/http/server_session

# Terminal 2
curl -c /tmp/libgs-cookies.txt -b /tmp/libgs-cookies.txt \
  http://127.0.0.1:8082/session
```

## Proxy and TLS

Run the proxy client only after starting an HTTP proxy:

```sh
./build/output/examples/http/proxy_client \
  http://127.0.0.1:8080/hello/Proxy \
  http://127.0.0.1:3128 user secret
```

Start the TLS server with a PEM certificate and key:

```sh
./build/output/examples/http/https_server \
  server.pem server-key.pem 8443
```

The client used to call it must trust the certificate according to its own TLS
configuration.
