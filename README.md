## C++ Real-Time Trading Engine (Personal Project)

This project is a fully self-designed, low-latency trading engine written in modern **C++20** — built entirely from scratch, without relying on third-party frameworks for core infrastructure (except boost for client websocket + https request). I built all core features from scratch, including:

---

### Core Features:

- **Custom Coroutine Engine (C++20)** ([`coroutine/`](core/coroutine))
  - Built without any coroutine library
  - Includes custom `promise_type`, awaiters (`co_await`, `co_return`), and scheduling logic
  - Enables fully async flow across market data, order processing, and state transitions

- **Internal REST API System** ([`https_server/`](core/https_server/https_server.h) + [`route/`](core/https_server/route) + [`request/`](core/https_server/request) + [`response/`](core/https_server/response) + [`app_route.cpp/`](server_app/api/app_route.cpp))
  - Native C++ HTTPs server (using `epoll`, `openssl`)
  - Fully self-implemented parser and request routing
  - Used for engine control, monitoring, and inter-process communication

- **Self-Built JSON Handling Layer** ([`json/`](core/json))
  - Lightweight JSON parser and serializer
  - Zero external dependencies
  - Used for config loading, logging, REST I/O

- **MongoDB Integration** ([`mongo_db/`](core/mongo_db))
  - Raw BSON serialization layer with no ORM
  - Used for storing order fills, commissions, output tokens
  - All write/read flow implemented manually

- **Order Lifecycle & Matching Logic** ([`order/`](server_app/order))
  - Clean abstraction of limit/market orders
  - Fill tracking, status transitions, and live audit
  - Modeled after real exchange behavior

- **Strategy Layer: Triangular Arbitrage Execution** ([`strategy_price_arbitrage/`](server_app/include/strategy_price_arbitrage))
  - Implements spot-based triangular arbitrage across three trading pairs
  - Handles full order lifecycle: quote → execute → cross conversion → settle

- **Built-in Latency Profiling**
  - `Microsecond`-level timing for each phase: 
    - market data receive → decision → order send (millisecond) → fill confirmation

- **Dockerized & Cloud-Ready** ([`z_docker/`](z_docker))
  - Fully containerized using minimal Docker image
  - Deployed and tested in live conditions on **AWS EC2**, allowing real production-like evaluation

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
