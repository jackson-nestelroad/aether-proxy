# Aether Proxy
**Aether** is a TCP proxy server for viewing and intercepting web traffic. It is implemented in **C++** using the **Boost.Asio** library for socket communication and **OpenSSL** for TLS functionality.

### Current Features
- HTTP Parsing
- HTTP Forwarding
- HTTP Interception
- TCP Tunneling (for HTTPS/TLS and WebSockets)
- TLS "Man-in-the-middle" Capability
- TLS Certificate Generation
- HTTPS Interception
- WebSocket Parsing
- WebSocket Interception
- Command-line Options
- Interactive Logs and Command-line Interface

### Examples
The proxy exposes a large handful of events that intercepting functions or objects can be attached to. To see how this API works, check out the [`interceptors::examples`](https://github.com/jackson-nestelroad/aether-proxy/tree/master/aether/interceptors/examples) namespace.
 
