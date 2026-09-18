# WebSocket Examples

[Examples index](../README.md) · [WebSocket guide](../../doc/en/websocket.md)

WebSocket examples require `LIBGS_BUILD_WEBSOCKET=ON`, which also requires HTTP.
The network programs use loopback endpoints by default.

## Programs

| Program | Demonstrates | Arguments / default |
| --- | --- | --- |
| `protocol` | Offline opening-handshake and frame generation/parsing | No arguments |
| `server` | Owned WebSocket server and text echo handler | `[port]`; `8080` at `/echo` |
| `client` | Owned client, message write/read, and clean close | `[endpoint]`; `ws://127.0.0.1:8080/echo` |
| `retry_open` | Application-controlled recovery after the active stream fails | `[endpoint]`; local echo server |
| `proxy_client` | HTTP or SOCKS5 proxy connection and optional credentials | `[endpoint] [proxy-url] [username] [password]` |
| `mixed_http_server` | HTTP route that either replies normally or performs Upgrade | `[port]`; `8080` at `/mixed` |
| `mixed_http_client` | One HTTP client used for a normal request and WebSocket Upgrade | `[http-url] [websocket-url]`; local `/mixed` route |
| `wss_server` | TLS WebSocket server | `<certificate.pem> <private-key.pem> [port]`; `8443` |
| `wss_client` | TLS client context, trust configuration, and WSS stream | `[endpoint] [ca-certificate.pem]`; local WSS endpoint |

WSS programs are built only with `LIBGS_OPENSSL_SUPPORT=ON`. The basic and
mixed servers both default to port 8080, so run only one of them at a time unless
you assign another port.

## Owned client/server pair

```sh
# Terminal 1
./build/output/examples/websocket/server

# Terminal 2
./build/output/examples/websocket/client
```

The server owns the listener and opening handshake. The client owns the HTTP
connector used by `open()`.

## Mixed HTTP and WebSocket route

The same `/mixed` route first handles a normal HTTP request, then accepts an
Upgrade request with the `libgs.example` subprotocol:

```sh
# Terminal 1
./build/output/examples/websocket/mixed_http_server

# Terminal 2
./build/output/examples/websocket/mixed_http_client
```

The client prints the normal HTTP response, the Upgrade status and selected
subprotocol, one echo message, and the close result.

## Explicit recovery

`retry_open` performs the initial `client.open()` once, then runs a business
read/write loop. When the stream fails, the application shuts it down and calls
`retry_open()` with at most eight recovery attempts.

```sh
# Terminal 1
./build/output/examples/websocket/server

# Terminal 2
./build/output/examples/websocket/retry_open
```

Stop and restart the server to observe recovery. The client is intentionally
long-running and must be interrupted when the demonstration is complete.

## Proxy

The proxy program accepts HTTP, `socks5://`, and `socks5h://` proxy URLs. A
proxy must already be running:

```sh
./build/output/examples/websocket/proxy_client \
  ws://127.0.0.1:8080/echo \
  http://127.0.0.1:3128 user secret

./build/output/examples/websocket/proxy_client \
  ws://127.0.0.1:8080/echo \
  socks5://127.0.0.1:1080 user secret
```

## WSS pair

The server requires a PEM certificate chain and private key. The optional client
CA argument adds a private CA or self-signed server certificate to its trust
store.

```sh
# Terminal 1
./build/output/examples/websocket/wss_server \
  server.pem server-key.pem 8443

# Terminal 2
./build/output/examples/websocket/wss_client \
  wss://127.0.0.1:8443/echo server.pem
```
