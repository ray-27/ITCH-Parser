// tools/bench.cpp
// Streaming mode — simulates real-time data arriving from a server in chunks.
// Usage: ./build/bench <file.itch>
#include "itch/parser.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <limits>

// Simulated network chunk size — mirrors a typical kernel TCP socket buffer.
// In production this would be the size of data arriving per recv() call.
static constexpr size_t CHUNK   = 64 * 1024;

// Largest possible ITCH message is ~50 bytes. This headroom holds any partial
// message that straddles two chunks without a heap allocation.
static constexpr size_t HEADROOM = 1024;

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: bench <file.itch>\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    // Working buffer — [leftover from prev chunk | new chunk data]
    // Static so it doesn't eat stack space.
    static uint8_t buf[CHUNK + HEADROOM];

    struct Counters {
        size_t system_event         = 0;
        size_t stock_directory      = 0;
        size_t stock_trading_action = 0;
        size_t reg_sho              = 0;
        size_t market_participant   = 0;
        size_t mwcb_decline         = 0;
        size_t mwcb_status          = 0;
        size_t ipo_quoting          = 0;
        size_t luld_collar          = 0;
        size_t operational_halt     = 0;
        size_t add_order            = 0;
        size_t add_order_mpid       = 0;
        size_t order_executed       = 0;
        size_t order_executed_price = 0;
        size_t order_cancel         = 0;
        size_t order_delete         = 0;
        size_t order_replace        = 0;
        size_t non_cross_trade      = 0;
        size_t cross_trade          = 0;
        size_t broken_trade         = 0;
        size_t noii                 = 0;
        size_t rpii                 = 0;
        size_t direct_listing       = 0;

        // One operator() per message type — compiler picks the right overload.
        // No if constexpr chain, no type checking at runtime.
        void operator()(const itch::AddOrder&)                      { ++add_order; }
        void operator()(const itch::OrderDelete&)                   { ++order_delete; }
        void operator()(const itch::OrderReplace&)                  { ++order_replace; }
        void operator()(const itch::OrderExecuted&)                 { ++order_executed; }
        void operator()(const itch::OrderCancel&)                   { ++order_cancel; }
        void operator()(const itch::OrderExecutedWithPrice&)        { ++order_executed_price; }
        void operator()(const itch::AddOrderMPID&)                  { ++add_order_mpid; }
        void operator()(const itch::NonCrossTrade&)                 { ++non_cross_trade; }
        void operator()(const itch::CrossTrade&)                    { ++cross_trade; }
        void operator()(const itch::BrokenTrade&)                   { ++broken_trade; }
        void operator()(const itch::NOII&)                          { ++noii; }
        void operator()(const itch::RPII&)                          { ++rpii; }
        void operator()(const itch::SystemEvent&)                   { ++system_event; }
        void operator()(const itch::StockDirectory&)                { ++stock_directory; }
        void operator()(const itch::StockTradingAction&)            { ++stock_trading_action; }
        void operator()(const itch::RegSHORestriction&)             { ++reg_sho; }
        void operator()(const itch::MarketParticipantPosition&)     { ++market_participant; }
        void operator()(const itch::MWCBDeclineLevel&)              { ++mwcb_decline; }
        void operator()(const itch::MWCBStatus&)                    { ++mwcb_status; }
        void operator()(const itch::IPOQuotingPeriodUpdate&)        { ++ipo_quoting; }
        void operator()(const itch::LULDAuctionCollar&)             { ++luld_collar; }
        void operator()(const itch::OperationalHalt&)               { ++operational_halt; }
        void operator()(const itch::DirectListingWithCapitalRaise&) { ++direct_listing; }
    } cnt{};

    size_t total_frames = 0;
    size_t leftover     = 0;   // bytes carried over from the previous chunk

    fprintf(stderr, "\n  Streaming started (chunk = %zu KB)...\n\n",
            CHUNK / 1024);

    auto t0 = std::chrono::high_resolution_clock::now();

    while (true) {
        // ── Receive next chunk ────────────────────────────────────────────────
        ssize_t n = read(fd, buf + leftover, CHUNK);
        if (n <= 0) break;

        size_t avail = leftover + (size_t)n;
        size_t pos   = 0;

        // ── Drain all complete SoupBin frames in this window ──────────────────
        while (pos + 2 <= avail) {
            // SoupBin 2-byte big-endian length prefix
            uint16_t msg_len = (static_cast<uint16_t>(buf[pos]) << 8)
                             |  static_cast<uint16_t>(buf[pos + 1]);

            if (pos + 2 + (size_t)msg_len > avail)
                break;   // message is incomplete — wait for next chunk

            itch::dispatch(buf + pos + 2, msg_len, cnt);

            pos += 2 + msg_len;
            ++total_frames;
        }

        // ── Carry incomplete tail to the front for the next chunk ─────────────
        leftover = avail - pos;
        if (leftover > 0)
            std::memmove(buf, buf + pos, leftover);
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();

    // ── Summary ───────────────────────────────────────────────────────────────
    fprintf(stderr, "═══════════════════════════════════════════\n");
    fprintf(stderr, "  itch-cpp  streaming bench\n");
    fprintf(stderr, "═══════════════════════════════════════════\n");
    fprintf(stderr, "  total frames  : %zu\n",   total_frames);
    fprintf(stderr, "  elapsed       : %.1f ms\n", elapsed * 1e3);
    fprintf(stderr, "  throughput    : %.2f M msg/s  (%.1f ns/msg)\n",
            total_frames / elapsed / 1e6,
            elapsed * 1e9 / total_frames);
    fprintf(stderr, "───────────────────────────────────────────\n");
    fprintf(stderr, "  message breakdown:\n");

    struct Entry { const char* name; size_t count; };
    Entry entries[] = {
        { "AddOrder",           cnt.add_order             },
        { "OrderDelete",        cnt.order_delete          },
        { "OrderReplace",       cnt.order_replace         },
        { "OrderExecuted",      cnt.order_executed        },
        { "OrderCancel",        cnt.order_cancel          },
        { "OrderExecutedPrice", cnt.order_executed_price  },
        { "NonCrossTrade",      cnt.non_cross_trade       },
        { "AddOrderMPID",       cnt.add_order_mpid        },
        { "NOII",               cnt.noii                  },
        { "CrossTrade",         cnt.cross_trade           },
        { "BrokenTrade",        cnt.broken_trade          },
        { "RPII",               cnt.rpii                  },
        { "StockDirectory",     cnt.stock_directory       },
        { "MarketParticipant",  cnt.market_participant    },
        { "StockTradingAction", cnt.stock_trading_action  },
        { "RegSHORestriction",  cnt.reg_sho               },
        { "MWCBDeclineLevel",   cnt.mwcb_decline          },
        { "MWCBStatus",         cnt.mwcb_status           },
        { "IPOQuotingPeriod",   cnt.ipo_quoting           },
        { "LULDAuctionCollar",  cnt.luld_collar           },
        { "OperationalHalt",    cnt.operational_halt      },
        { "SystemEvent",        cnt.system_event          },
        { "DirectListing",      cnt.direct_listing        },
    };
    for (auto& e : entries) {
        if (e.count == 0) continue;
        fprintf(stderr, "    %-22s  %9zu  (%5.2f%%)\n",
                e.name, e.count,
                100.0 * e.count / total_frames);
    }
    fprintf(stderr, "═══════════════════════════════════════════\n\n");

    close(fd);
    return 0;
}
