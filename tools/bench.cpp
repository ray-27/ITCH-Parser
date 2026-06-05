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
    } cnt{};

    size_t total_frames = 0;
    size_t leftover     = 0;   // bytes carried over from the previous chunk

    fprintf(stderr, "\n  Streaming started (chunk = %zu KB)...\n\n",
            CHUNK / 1024);

    auto t0 = std::chrono::high_resolution_clock::now();

    while (true) {
        // ── Receive next chunk ────────────────────────────────────────────────
        // In production: replace read() with recv() / ring-buffer dequeue / etc.
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

            // ── Parse and dispatch ────────────────────────────────────────────
            itch::dispatch(buf + pos + 2, msg_len, [&](auto&& m) {
                using T = std::decay_t<decltype(m)>;

                if constexpr (std::is_same_v<T, itch::AddOrder>)
                    ++cnt.add_order;
                else if constexpr (std::is_same_v<T, itch::OrderDelete>)
                    ++cnt.order_delete;
                else if constexpr (std::is_same_v<T, itch::OrderReplace>)
                    ++cnt.order_replace;
                else if constexpr (std::is_same_v<T, itch::OrderExecuted>)
                    ++cnt.order_executed;
                else if constexpr (std::is_same_v<T, itch::OrderCancel>)
                    ++cnt.order_cancel;
                else if constexpr (std::is_same_v<T, itch::OrderExecutedWithPrice>)
                    ++cnt.order_executed_price;
                else if constexpr (std::is_same_v<T, itch::AddOrderMPID>)
                    ++cnt.add_order_mpid;
                else if constexpr (std::is_same_v<T, itch::NonCrossTrade>)
                    ++cnt.non_cross_trade;
                else if constexpr (std::is_same_v<T, itch::CrossTrade>)
                    ++cnt.cross_trade;
                else if constexpr (std::is_same_v<T, itch::BrokenTrade>)
                    ++cnt.broken_trade;
                else if constexpr (std::is_same_v<T, itch::NOII>)
                    ++cnt.noii;
                else if constexpr (std::is_same_v<T, itch::RPII>)
                    ++cnt.rpii;
                else if constexpr (std::is_same_v<T, itch::SystemEvent>)
                    ++cnt.system_event;
                else if constexpr (std::is_same_v<T, itch::StockDirectory>)
                    ++cnt.stock_directory;
                else if constexpr (std::is_same_v<T, itch::StockTradingAction>)
                    ++cnt.stock_trading_action;
                else if constexpr (std::is_same_v<T, itch::RegSHORestriction>)
                    ++cnt.reg_sho;
                else if constexpr (std::is_same_v<T, itch::MarketParticipantPosition>)
                    ++cnt.market_participant;
                else if constexpr (std::is_same_v<T, itch::MWCBDeclineLevel>)
                    ++cnt.mwcb_decline;
                else if constexpr (std::is_same_v<T, itch::MWCBStatus>)
                    ++cnt.mwcb_status;
                else if constexpr (std::is_same_v<T, itch::IPOQuotingPeriodUpdate>)
                    ++cnt.ipo_quoting;
                else if constexpr (std::is_same_v<T, itch::LULDAuctionCollar>)
                    ++cnt.luld_collar;
                else if constexpr (std::is_same_v<T, itch::OperationalHalt>)
                    ++cnt.operational_halt;
                else if constexpr (std::is_same_v<T, itch::DirectListingWithCapitalRaise>)
                    ++cnt.direct_listing;
            });

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
