# itch-cpp

A zero-allocation, header-only C++17 parser for NASDAQ ITCH 5.0 binary market data.  
Parses messages directly from raw bytes with no heap allocation and no `std::variant` construction on the hot path.

---

## Benchmarks

Tested on a real NASDAQ ITCH 5.0 file (January 30, 2019), Apple Silicon, `-O3 -march=native`.

### Pure parse throughput — data pre-loaded and pinned in RAM (`dump`)

| File size | Messages | Throughput | ns/msg |
|---|---|---|---|
| 5.62 GB | 184,485,557 | **456 M msg/s** | 2.2 ns |
| 1.01 GB | 27,798,910 | **445 M msg/s** | 2.2 ns |

### Streaming throughput — 64 KB chunks, simulating live network feed (`stream`)

| File size | Messages | Throughput | ns/msg |
|---|---|---|---|
| 5.62 GB | 184,485,557 | **92 M msg/s** | 10.8 ns |
| 1.01 GB | 27,798,910 | **162 M msg/s** | 6.1 ns |

The streaming number is I/O bound — the bottleneck is disk read speed, not the parser.  
A live NASDAQ feed delivers ~500 MB/s peak; the parser processes it 5–10× faster than the network can supply data.

---

## Adding as a Git Submodule

```bash
git submodule add https://github.com/ray-27/ITCH-Parser.git vendor/itch-cpp
git submodule update --init
```

In your `CMakeLists.txt`:

```cmake
add_subdirectory(vendor/itch-cpp)
target_link_libraries(your_target PRIVATE itch)
```

---

## Headers

| Header | What it gives you |
|---|---|
| `#include "itch/parser.hpp"` | `dispatch()` and `parse()` — the core parsing functions |
| `#include "itch/reader.hpp"` | `SoupBinReader` — handles SoupBin framing over a pre-loaded buffer. Includes `parser.hpp`. |
| `#include "itch/messages.hpp"` | All 23 message structs and the `ITCHMessage` variant typedef |
| `#include "itch/types.hpp"` | Strong typedefs: `Price4`, `Price8`, `OrderRef`, `Timestamp`, `Shares`, `Locate`, … |
| `#include "itch/fields.hpp"` | All enums: `Side`, `EventCode`, `TradingState`, `CrossType`, … |

For almost all use cases, include only `"itch/reader.hpp"` or `"itch/parser.hpp"`.

---

## API Reference

### `itch::dispatch(buf, len, handler)`

Zero-allocation hot-path parser. Decodes the message at `buf` and calls `handler` with the
concrete struct type. No variant is constructed. Use this everywhere performance matters.

```cpp
// buf  — pointer to ITCH message bytes (after stripping the 2-byte SoupBin length prefix)
// len  — value read from the 2-byte SoupBin length prefix
// handler — any callable: lambda, functor, struct with operator() overloads
template<typename Handler>
bool itch::dispatch(const uint8_t* buf, uint16_t len, Handler&& handler);
// returns true if the type byte was recognised, false for unknown messages
```

**What the handler is:**

The handler is any C++ callable that can accept each of the 23 concrete message structs.
`dispatch` reads `buf[0]` (the ITCH type byte), decodes the matching struct, and calls
`handler(parsed_struct)` directly with that concrete type — no variant, no type erasure.

The handler is resolved entirely at **compile time**. The compiler sees the exact type being
passed into every call site and generates a separate, optimised code path per message type.

**Three valid forms:**

```cpp
// 1. Generic lambda — one body, compiler stamps out a version per type
itch::dispatch(buf, len, [](auto&& m) {
    using T = std::decay_t<decltype(m)>;
    if constexpr (std::is_same_v<T, itch::AddOrder>) { /* ... */ }
    // types with no branch are a no-op — no runtime cost
});

// 2. Struct with operator() overloads — one method per type you care about
struct MyHandler {
    void operator()(const itch::AddOrder& m)    { /* ... */ }
    void operator()(const itch::OrderDelete& m) { /* ... */ }
    // no overload for a type = that message is silently ignored
};
itch::dispatch(buf, len, MyHandler{});

// 3. Stateful handler capturing external state
MyHandler handler{ &order_book, &trade_log };
itch::dispatch(buf, len, handler);
```

**What the handler must NOT do:**

- It must not take a `const ITCHMessage&` or `std::variant` — `dispatch` passes the concrete struct, not the variant.
- It does not need to handle all 23 types. Any type with no matching overload or `if constexpr` branch is silently skipped — `dispatch` returns `true` (message was recognised) but the handler body is a no-op.

**Why this is faster than `std::visit`:**

With `std::visit` on a variant, the compiler builds a function pointer table indexed by the variant's type index and performs an indirect jump at runtime. With `dispatch`, the switch on the type byte and the handler call are both resolved at compile time — the generated code for an `AddOrder` message is a direct inline sequence with no indirection at all.

---

### `itch::parse(buf, len)`

Returns `std::optional<ITCHMessage>` (a `std::variant` of all 23 message types).  
Use this only when you need to **store** a message — e.g. push it onto a queue.

```cpp
template<typename Handler>
std::optional<itch::ITCHMessage> itch::parse(const uint8_t* buf, uint16_t len);
```

---

### `itch::SoupBinReader`

Wraps a contiguous buffer that contains a full SoupBin-framed ITCH file.  
Reads the 2-byte length prefix on each frame and calls your handler with each parsed message.

```cpp
itch::SoupBinReader reader(data, size);   // data = uint8_t*, size = byte count

size_t total = reader.run(handler);       // returns number of frames processed
// reader is stateful — calling run() again continues from where it stopped
```

---

## Parsing Patterns

### Pattern 1 — Generic lambda with `if constexpr`

Minimal boilerplate. The compiler generates a separate body per message type at compile time — no runtime branching on the type.

```cpp
#include "itch/reader.hpp"

itch::SoupBinReader reader(data, size);

reader.run([](auto&& msg) {
    using T = std::decay_t<decltype(msg)>;

    if constexpr (std::is_same_v<T, itch::AddOrder>) {
        // msg.order_ref, msg.side, msg.shares, msg.stock, msg.price
    }
    else if constexpr (std::is_same_v<T, itch::OrderDelete>) {
        // msg.order_ref
    }
    else if constexpr (std::is_same_v<T, itch::OrderReplace>) {
        // msg.original_order_ref, msg.new_order_ref, msg.shares, msg.price
    }
    // types with no branch are silently skipped
});
```

---

### Pattern 2 — Struct with `operator()` overloads

Cleaner when handling many types. One method per message type — the compiler picks the right one, no type checking at all.

```cpp
struct OrderBookHandler {
    void operator()(const itch::AddOrder& m) {
        book.add(m.order_ref, m.side, m.shares, m.price);
    }
    void operator()(const itch::OrderDelete& m) {
        book.remove(m.order_ref);
    }
    void operator()(const itch::OrderReplace& m) {
        book.replace(m.original_order_ref, m.new_order_ref, m.shares, m.price);
    }
    void operator()(const itch::OrderExecuted& m) {
        book.execute(m.order_ref, m.executed_shares);
    }
    void operator()(const itch::OrderCancel& m) {
        book.cancel(m.order_ref, m.cancelled_shares);
    }
    // types with no overload are silently ignored by dispatch
};

reader.run(OrderBookHandler{ book });
```

---

### Pattern 3 — Streaming loop (live feed / network socket)

For data arriving in chunks. Manages partial messages that straddle chunk boundaries.

```cpp
#include "itch/parser.hpp"

constexpr size_t CHUNK = 64 * 1024;
static uint8_t buf[CHUNK + 1024];  // headroom for partial messages
size_t leftover = 0;

while (true) {
    ssize_t n = recv(socket_fd, buf + leftover, CHUNK, 0);
    if (n <= 0) break;

    size_t avail = leftover + (size_t)n;
    size_t pos   = 0;

    while (pos + 2 <= avail) {
        uint16_t msg_len = (uint16_t(buf[pos]) << 8) | buf[pos + 1];
        if (pos + 2 + msg_len > avail) break;  // wait for next chunk

        itch::dispatch(buf + pos + 2, msg_len, my_handler);

        pos += 2 + msg_len;
    }

    leftover = avail - pos;
    if (leftover > 0)
        std::memmove(buf, buf + pos, leftover);
}
```

---

### Pattern 4 — Store a message for later (`parse`)

```cpp
#include "itch/parser.hpp"

std::optional<itch::ITCHMessage> msg = itch::parse(buf, len);
if (msg) {
    my_queue.push(*msg);
}
```

---

## Which function to use

| Situation | Use |
|---|---|
| Reading an ITCH file, processing every message | `SoupBinReader::run()` |
| Live network feed, kernel ring buffer, streaming source | `dispatch()` |
| Need to store a message in a struct or push to a queue | `parse()` → `ITCHMessage` |
| Routing messages to per-stock channels | `dispatch()` — access `msg.stock` directly |
| Need to inspect the type without handling it | `parse()` + `std::holds_alternative<T>()` |

---

## Message Types and Key Fields

| Message | % of traffic | Key fields |
|---|---|---|
| `AddOrder` | ~44% | `order_ref`, `side`, `shares`, `stock[8]`, `price` |
| `OrderDelete` | ~43% | `order_ref` |
| `OrderReplace` | ~7% | `original_order_ref`, `new_order_ref`, `shares`, `price` |
| `OrderExecuted` | ~2% | `order_ref`, `executed_shares`, `match_number` |
| `OrderCancel` | ~1% | `order_ref`, `cancelled_shares` |
| `AddOrderMPID` | <1% | same as `AddOrder` + `attribution[4]` (MPID string) |
| `OrderExecutedWithPrice` | <1% | `order_ref`, `executed_shares`, `match_number`, `execution_price` |
| `NonCrossTrade` | <1% | `shares`, `stock[8]`, `price`, `match_number` |
| `NOII` | ~1% | `paired_shares`, `imbalance_shares`, `imbalance_direction`, `stock[8]`, `near_price`, `far_price`, `cross_type` |
| `StockDirectory` | cold | `stock[8]`, `market_category`, `financial_status`, `round_lot_size` |
| `CrossTrade` | cold | `shares`, `stock[8]`, `cross_price`, `match_number`, `cross_type` |
| `BrokenTrade` | cold | `match_number` |
| `SystemEvent` | cold | `event_code` |

**Price fields** are fixed-point integers:
- `Price4` → divide by `10000.0` to get dollars (e.g. `185000000` → `$18.50`)
- `Price8` → divide by `100000000.0`

**Strong types** — all numeric fields are wrapped (`OrderRef`, `Shares`, `Locate`, etc.).  
Access the raw integer with `.value` or `.raw()`:
```cpp
uint64_t ref = m.order_ref.value;
uint32_t qty = m.shares.value;
double   px  = m.price.value / 10000.0;
```

**Stock field** is `std::array<char, 8>` — space-padded, not null-terminated:
```cpp
// print 8 chars: printf("%.8s", m.stock.data());
// compare:       m.stock == itch::Alpha<8>{'A','A','P','L',' ',' ',' ',' '}
```

---

## Building the Tools

```bash
cmake -B build -S .
cmake --build build
```

| Binary | Purpose |
|---|---|
| `./build/stream <file.itch>` | Streaming benchmark — 64 KB chunks, simulates live feed |
| `./build/dump <file.itch>` | Pre-load benchmark — pins file in RAM, measures pure parse speed |
| `./build/print <file.itch>` | Human-readable message dump to stdout |
| `./build/print <file.itch> -o out.txt` | Same, saved to file |

---

## Requirements

- C++17 or later
- CMake 3.20+
- GCC 9+ or Clang 10+ or MSVC 19.29+
- Little-endian host (x86-64 or ARM64)
