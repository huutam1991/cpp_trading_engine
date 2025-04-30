## C++ Real-Time Trading Engine (Personal Project)

This project is a fully self-designed, low-latency trading engine written in modern **C++20** — built entirely from scratch, without relying on third-party frameworks for core infrastructure (except boost for client websocket + https request). I built all core features from scratch, including:

---

### Core Features:

- **Custom Coroutine Engine (C++20)**
  - Built without any coroutine library
  - Includes custom `promise_type`, awaiters, and scheduling logic
  - Enables fully async flow across market data, order processing, and state transitions

- **Internal REST API System**
  - Native C++ async HTTPs server and client
  - Fully self-implemented parser and request routing
  - Used for engine control, monitoring, and inter-process communication

- **Self-Built JSON Handling Layer**
  - Lightweight JSON parser and serializer
  - Zero external dependencies
  - Used for config loading, logging, REST I/O

- **MongoDB Integration (raw driver layer)**
  - Raw BSON serialization layer with no ORM
  - Used for storing order fills, commissions, output tokens
  - All write/read flow implemented manually

- **Order Lifecycle & Matching Logic**
  - Clean abstraction of limit/market orders
  - Fill tracking, status transitions, and live audit
  - Modeled after real exchange behavior

- **Strategy Layer: Triangular Arbitrage Execution**
  - Implements spot-based triangular arbitrage across three trading pairs
  - Handles full order lifecycle: quote → execute → cross conversion → settle

- **Built-in Latency Profiling**
  - Millisecond-level timing for each phase: 
    - market data receive → decision → order send → fill confirmation

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
