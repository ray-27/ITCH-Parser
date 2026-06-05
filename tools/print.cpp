// tools/print.cpp
// Usage:
//   ./build/print <file.itch>            — print to stdout
//   ./build/print <file.itch> -o out.txt — save to file
#include "itch/parser.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <chrono>

// ── Per-type print functions ──────────────────────────────────────────────────
// Each overload receives the concrete struct — no variant, no std::visit.

static void print_msg(FILE* out, const itch::SystemEvent& m) {
    fprintf(out, "SystemEvent          | locate=%-5u  ts=%-15llu  event=%c\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        static_cast<char>(m.event_code));
}
static void print_msg(FILE* out, const itch::StockDirectory& m) {
    fprintf(out, "StockDirectory       | locate=%-5u  ts=%-15llu  stock=%.8s  mktcat=%c  status=%c  lot=%u\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        m.stock.data(), static_cast<char>(m.market_category),
        static_cast<char>(m.financial_status), m.round_lot_size.value);
}
static void print_msg(FILE* out, const itch::StockTradingAction& m) {
    fprintf(out, "StockTradingAction   | locate=%-5u  ts=%-15llu  stock=%.8s  state=%c  reason=%.4s\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        m.stock.data(), static_cast<char>(m.trading_state), m.reason.data());
}
static void print_msg(FILE* out, const itch::RegSHORestriction& m) {
    fprintf(out, "RegSHORestriction    | locate=%-5u  ts=%-15llu  stock=%.8s  action=%c\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        m.stock.data(), static_cast<char>(m.reg_sho_action));
}
static void print_msg(FILE* out, const itch::MarketParticipantPosition& m) {
    fprintf(out, "MarketParticipant    | locate=%-5u  ts=%-15llu  mpid=%.4s  stock=%.8s  pmm=%c  mode=%c  state=%c\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        m.mpid.data(), m.stock.data(),
        static_cast<char>(m.primary_market_maker),
        static_cast<char>(m.market_maker_mode),
        static_cast<char>(m.market_participant_state));
}
static void print_msg(FILE* out, const itch::MWCBDeclineLevel& m) {
    fprintf(out, "MWCBDeclineLevel     | ts=%-15llu  L1=%.4f  L2=%.4f  L3=%.4f\n",
        (unsigned long long)m.timestamp_ns.value,
        m.level_1.value / 10000.0, m.level_2.value / 10000.0, m.level_3.value / 10000.0);
}
static void print_msg(FILE* out, const itch::MWCBStatus& m) {
    fprintf(out, "MWCBStatus           | ts=%-15llu  breached=%c\n",
        (unsigned long long)m.timestamp_ns.value, static_cast<char>(m.breached_level));
}
static void print_msg(FILE* out, const itch::IPOQuotingPeriodUpdate& m) {
    fprintf(out, "IPOQuoting           | ts=%-15llu  stock=%.8s  release=%u  qualifier=%c  price=%.4f\n",
        (unsigned long long)m.timestamp_ns.value, m.stock.data(),
        m.ipo_quotation_release_time,
        static_cast<char>(m.ipo_quotation_release_qualifier),
        m.ipo_price.value / 10000.0);
}
static void print_msg(FILE* out, const itch::LULDAuctionCollar& m) {
    fprintf(out, "LULDAuctionCollar    | locate=%-5u  ts=%-15llu  stock=%.8s  ref=%.4f  upper=%.4f  lower=%.4f\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value, m.stock.data(),
        m.auction_collar_reference_price.value / 10000.0,
        m.upper_auction_collar_price.value / 10000.0,
        m.lower_auction_collar_price.value / 10000.0);
}
static void print_msg(FILE* out, const itch::OperationalHalt& m) {
    fprintf(out, "OperationalHalt      | locate=%-5u  ts=%-15llu  stock=%.8s  market=%c  action=%c\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value, m.stock.data(),
        static_cast<char>(m.market_code), static_cast<char>(m.halt_action));
}
static void print_msg(FILE* out, const itch::AddOrder& m) {
    fprintf(out, "AddOrder             | locate=%-5u  ts=%-15llu  ref=%-12llu  side=%c  shares=%-7u  stock=%.8s  price=%.4f\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        (unsigned long long)m.order_ref.value, static_cast<char>(m.side),
        m.shares.value, m.stock.data(), m.price.value / 10000.0);
}
static void print_msg(FILE* out, const itch::AddOrderMPID& m) {
    fprintf(out, "AddOrderMPID         | locate=%-5u  ts=%-15llu  ref=%-12llu  side=%c  shares=%-7u  stock=%.8s  price=%.4f  mpid=%.4s\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        (unsigned long long)m.order_ref.value, static_cast<char>(m.side),
        m.shares.value, m.stock.data(), m.price.value / 10000.0, m.attribution.data());
}
static void print_msg(FILE* out, const itch::OrderExecuted& m) {
    fprintf(out, "OrderExecuted        | locate=%-5u  ts=%-15llu  ref=%-12llu  exec_shares=%-7u  match=%llu\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        (unsigned long long)m.order_ref.value, m.executed_shares.value,
        (unsigned long long)m.match_number.value);
}
static void print_msg(FILE* out, const itch::OrderExecutedWithPrice& m) {
    fprintf(out, "OrderExecutedPrice   | locate=%-5u  ts=%-15llu  ref=%-12llu  exec_shares=%-7u  match=%-12llu  printable=%c  price=%.4f\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        (unsigned long long)m.order_ref.value, m.executed_shares.value,
        (unsigned long long)m.match_number.value, static_cast<char>(m.printable),
        m.execution_price.value / 10000.0);
}
static void print_msg(FILE* out, const itch::OrderCancel& m) {
    fprintf(out, "OrderCancel          | locate=%-5u  ts=%-15llu  ref=%-12llu  cancelled=%u\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        (unsigned long long)m.order_ref.value, m.cancelled_shares.value);
}
static void print_msg(FILE* out, const itch::OrderDelete& m) {
    fprintf(out, "OrderDelete          | locate=%-5u  ts=%-15llu  ref=%llu\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        (unsigned long long)m.order_ref.value);
}
static void print_msg(FILE* out, const itch::OrderReplace& m) {
    fprintf(out, "OrderReplace         | locate=%-5u  ts=%-15llu  old_ref=%-12llu  new_ref=%-12llu  shares=%-7u  price=%.4f\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        (unsigned long long)m.original_order_ref.value,
        (unsigned long long)m.new_order_ref.value,
        m.shares.value, m.price.value / 10000.0);
}
static void print_msg(FILE* out, const itch::NonCrossTrade& m) {
    fprintf(out, "NonCrossTrade        | locate=%-5u  ts=%-15llu  side=%c  shares=%-7u  stock=%.8s  price=%.4f  match=%llu\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        static_cast<char>(m.side), m.shares.value, m.stock.data(),
        m.price.value / 10000.0, (unsigned long long)m.match_number.value);
}
static void print_msg(FILE* out, const itch::CrossTrade& m) {
    fprintf(out, "CrossTrade           | locate=%-5u  ts=%-15llu  shares=%-7u  stock=%.8s  price=%.4f  match=%-12llu  type=%c\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        m.shares.value, m.stock.data(), m.cross_price.value / 10000.0,
        (unsigned long long)m.match_number.value, static_cast<char>(m.cross_type));
}
static void print_msg(FILE* out, const itch::BrokenTrade& m) {
    fprintf(out, "BrokenTrade          | locate=%-5u  ts=%-15llu  match=%llu\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        (unsigned long long)m.match_number.value);
}
static void print_msg(FILE* out, const itch::NOII& m) {
    fprintf(out, "NOII                 | locate=%-5u  ts=%-15llu  stock=%.8s  paired=%-7u  imbal=%-7u  dir=%c  near=%.4f  far=%.4f\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value, m.stock.data(),
        m.paired_shares.value, m.imbalance_shares.value,
        static_cast<char>(m.imbalance_direction),
        m.near_price.value / 10000.0, m.far_price.value / 10000.0);
}
static void print_msg(FILE* out, const itch::RPII& m) {
    fprintf(out, "RPII                 | locate=%-5u  ts=%-15llu  stock=%.8s  flag=%c\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value,
        m.stock.data(), static_cast<char>(m.interest_flag));
}
static void print_msg(FILE* out, const itch::DirectListingWithCapitalRaise& m) {
    fprintf(out, "DirectListingCR      | locate=%-5u  ts=%-15llu  stock=%.8s  eligible=%c  near=%.4f\n",
        m.stock_locate.value, (unsigned long long)m.timestamp_ns.value, m.stock.data(),
        static_cast<char>(m.open_eligibility_status),
        m.near_execution_price.value / 10000.0);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: print <file.itch> [-o output.txt]\n");
        return 1;
    }

    const char* out_path = nullptr;
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
            out_path = argv[++i];
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    FILE* out = stdout;
    if (out_path) {
        out = fopen(out_path, "w");
        if (!out) { perror("fopen"); return 1; }
        fprintf(stderr, "writing to %s\n", out_path);
    }

    // Stream in 64 KB chunks — print.cpp is output-bound so there is no benefit
    // to pre-loading the whole file, and streaming keeps RAM use minimal.
    constexpr size_t CHUNK    = 64 * 1024;
    constexpr size_t HEADROOM = 1024;
    static uint8_t buf[CHUNK + HEADROOM];

    size_t total    = 0;
    size_t leftover = 0;

    auto t_start = std::chrono::high_resolution_clock::now();

    while (true) {
        ssize_t n = read(fd, buf + leftover, CHUNK);
        if (n <= 0) break;

        size_t avail = leftover + (size_t)n;
        size_t pos   = 0;

        while (pos + 2 <= avail) {
            uint16_t msg_len = (static_cast<uint16_t>(buf[pos]) << 8)
                             |  static_cast<uint16_t>(buf[pos + 1]);
            if (__builtin_expect(pos + 2 + (size_t)msg_len > avail, 0)) break;

            itch::dispatch(buf + pos + 2, msg_len, [&](auto&& m) {
                print_msg(out, m);
            });

            pos += 2 + msg_len;
            ++total;
        }

        leftover = avail - pos;
        if (leftover > 0)
            std::memmove(buf, buf + pos, leftover);
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    double elapsed_s = std::chrono::duration<double>(t_end - t_start).count();

    if (out_path) fclose(out);

    fprintf(stderr, "frames: %zu  elapsed: %.1f ms\n", total, elapsed_s * 1e3);

    close(fd);
    return 0;
}
