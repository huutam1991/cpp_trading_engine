## C++ Real-Time Trading Engine (Personal Project)

This project is a fully self-designed, low-latency trading engine written in modern **C++20** — built entirely from scratch, without relying on third-party frameworks for core infrastructure (except boost for client websocket + https request). I built all core features from scratch, including:

---

### Core Features:

- **Custom Coroutine Engine (C++20)** ([`coroutine/`](core/coroutine))
  - Built without any coroutine library
  - Includes custom `promise_type`, awaiters (`co_await`, `co_return`), and scheduling logic
  - Enables fully async flow across market data, order processing, and state transitions
  - Task dispatch latency of `1–3 µs (p90 < 10 µs, p99 < 150 µs)`, `outperforming` or matching `Folly EventBase` in real-world benchmarks

- **Internal REST API System** ([`https_server/`](core/https_server/https_server.h) + [`route/`](core/https_server/route) + [`request/`](core/https_server/request) + [`response/`](core/https_server/response) + [`app_route.cpp/`](server_app/api/app_route.cpp))
  - Native C++ HTTPs server (using `epoll`, `openssl`)
  - Fully self-implemented parser and request routing
  - Used for engine control, monitoring, and inter-process communication

- **Self-Built JSON Handling Layer** ([`json/`](core/json))
  - Lightweight JSON parser and serializer
  - Zero external dependencies
  - Used for config loading, logging, REST/Websocket I/O
  - Achieved `40–70 µs` parsing time per object, comparable to `RapidJSON` in performance

- **WebSocket** ([`websocket/`](core/websocket))
  - Built on top of Boost.Asio with fully `asynchronous design` (using custom `co_await` / `co_return` coroutine flow)

- **Cache Pool** ([`cache_pool.h`](core/cache/cache_pool.h))
  - Custom `lock-free memory pool` designed for `high-frequency` object allocation and reuse
  - Achieves `40–50 ns` acquire/release time in `hot path` and `1–2 µs` in `cold path`
  - Minimizes heap contention and improves cache locality across concurrent threads

- **MongoDB Integration** ([`mongo_db/`](core/mongo_db))
  - Raw BSON serialization layer with no ORM
  - Used for storing order fills, commissions, output tokens
  - All write/read flow implemented manually

- **Order Lifecycle & Matching Logic** ([`order/`](server_app/order))
  - Clean abstraction of limit/market orders
  - Fill tracking, status transitions, and live audit
  - Modeled after real exchange behavior

- **Strategy Layer: Market Maker** ([`strategy_market_maker/`](server_app/strategy_market_maker))
  - Implements a fully autonomous market-making strategy that continuously quotes bid/ask orders based on live order book updates and 15-minute rolling trade volume statistics.
  - Dynamically adjusts quoting depth, price gap, and order sizes according to liquidity imbalance (buy/sell ratio) and market volatility.
  - Dynamically recalculates min_trade_volume, price_gap, and quote density every few seconds based on recent liquidity.

- **Dockerized & Cloud-Ready** ([`z_docker/`](z_docker))
  - Fully containerized using minimal Docker image
  - Deployed and tested in live conditions on **AWS EC2**, allowing real production-like evaluation

---

# Performance Summary
  - End-to-end processing latency of `<100 µs` per WebSocket event, measured in production with live Binance data feeds
  - Demonstrates stable latency distribution across threads under sustained load (no spikes `>150 µs`)

---

This project is **not a simulation** — it's a complete engine that mirrors real-world behavior in crypto spot markets


# Build command (Linux)
```
./install_library.sh
./build_bash.sh
```

# Run command  (Linux)
```
sudo service mongod start # need to have MongoDB running in background
./run_bash.sh
```
