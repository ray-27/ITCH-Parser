#pragma once
#include "fields.hpp"
#include "messages.hpp"
#include "types.hpp"
#include <cstdint>
#include <cstring>
#include <optional>

namespace itch {
    //ITCH is a Big-endian. x86 is little endian. We must byte swap every multi byte integer field we read off the wire

    // inline uint16_t read_u16(const uint8_t* p) {
    //     uint16_t v;
    //     memcpy(&v, p, 2);
    //     return __builtin_bswap16(v);
    // }

    // inline uint16_t read_u32(const uint8_t* p) {
    //     uint16_t v;
    //     memcpy(&v, p, 2);
    //     return __builtin_bswap16(v);
    // }
    // These functions can be compressed into a template

    template<typename T>
    struct underlying_int {
        using type = T;
    };
    template<typename T, typename Tag>
    struct underlying_int<StrongType<T, Tag>> {
        using type = T;
    };
    template<typename T>
    using underlying_int_t = typename underlying_int<T>::type;

    template<typename T>
    inline T read_be(const uint8_t* p) {
        using Raw = underlying_int_t<T>;
        Raw v;
        memcpy(&v, p, sizeof(Raw));
        Raw swapped;
        if constexpr (sizeof(Raw) == 1) swapped = v;
        if constexpr (sizeof(Raw) == 2) swapped = __builtin_bswap16(v);
        if constexpr (sizeof(Raw) == 4) swapped = __builtin_bswap32(v);
        if constexpr (sizeof(Raw) == 8) swapped = __builtin_bswap64(v);
        return T{swapped};
    }
    // template<typename T>
    // inline T read_be(const uint8_t* p) {
    //     T v;
    //     memcpy(&v, p, sizeof(T));

    //     if constexpr (sizeof(T) == 1) return v;
    //     if constexpr (sizeof(T) == 2) return static_cast<T>(__builtin_bswap16(v));
    //     if constexpr (sizeof(T) == 4) return static_cast<T>(__builtin_bswap32(v));
    //     if constexpr (sizeof(T) == 8) return static_cast<T>(__builtin_bswap64(v));
    // }

    // Stays as-is — 6-byte timestamp has no generic solution
    // There's no standard integer type that's 6 bytes, so the template can't handle it cleanly. Keeping it explicit
    inline uint64_t read_u48(const uint8_t* p) {
        uint64_t v = 0;
        memcpy(reinterpret_cast<uint8_t*>(&v) + 2, p, 6);
        return __builtin_bswap64(v);
    }

    template<size_t N>
    inline Alpha<N> read_alpha(const uint8_t* p) {
        Alpha<N> a;
        memcpy(a.data(), p, N);
        return a;
    }

    // ── Common header fields shared by every message ──────────────────────────
    // [0]    type (1)
    // [1-2]  stock_locate (2)
    // [3-4]  tracking_number (2)
    // [5-10] timestamp_ns (6)
    // first payload byte always at offset 11

    // ── 1.1 System Event ─────────────────────────────────────────────────────
    inline SystemEvent parse_system_event(const uint8_t* b) {
        return {
            .stock_locate    = read_be<Locate>(b + 1),
            .tracking_number = read_be<TrackingNumber>(b + 3),
            .timestamp_ns    = Timestamp{read_u48(b + 5)},
            .event_code      = static_cast<EventCode>(b[11]),
        };
    }

    // ── 1.2.1 Stock Directory ────────────────────────────────────────────────
    inline StockDirectory parse_stock_directory(const uint8_t* b) {
        return {
            .stock_locate         = read_be<Locate>(b + 1),
            .tracking_number      = read_be<TrackingNumber>(b + 3),
            .timestamp_ns         = Timestamp{read_u48(b + 5)},
            .stock                = read_alpha<8>(b + 11),
            .market_category      = static_cast<MarketCategory>(b[19]),
            .financial_status     = static_cast<FinancialStatus>(b[20]),
            .round_lot_size       = read_be<Shares>(b + 21),
            .round_lots_only      = static_cast<RoundLotsOnly>(b[25]),
            .issue_classification = static_cast<IssueClassification>(b[26]),
            .issue_sub_type       = read_alpha<2>(b + 27),
            .authenticity         = static_cast<Authenticity>(b[29]),
            .short_sale_threshold = static_cast<ShortSaleThreshold>(b[30]),
            .ipo_flag             = static_cast<IPOFlag>(b[31]),
            .luld_tier            = static_cast<LULDTier>(b[32]),
            .etp_flag             = static_cast<ETPFlag>(b[33]),
            .etp_leverage_factor  = read_be<uint32_t>(b + 34),
            .inverse_indicator    = static_cast<InverseIndicator>(b[38]),
        };
    }

    // ── 1.2.2 Stock Trading Action ───────────────────────────────────────────
    inline StockTradingAction parse_stock_trading_action(const uint8_t* b) {
        return {
            .stock_locate    = read_be<Locate>(b + 1),
            .tracking_number = read_be<TrackingNumber>(b + 3),
            .timestamp_ns    = Timestamp{read_u48(b + 5)},
            .stock           = read_alpha<8>(b + 11),
            .trading_state   = static_cast<TradingState>(b[19]),
            .reserved        = static_cast<char>(b[20]),
            .reason          = read_alpha<4>(b + 21),
        };
    }

    // ── 1.2.3 Reg SHO Short Sale Price Test ─────────────────────────────────
    inline RegSHORestriction parse_reg_sho_restriction(const uint8_t* b) {
        return {
            .stock_locate    = read_be<Locate>(b + 1),
            .tracking_number = read_be<TrackingNumber>(b + 3),
            .timestamp_ns    = Timestamp{read_u48(b + 5)},
            .stock           = read_alpha<8>(b + 11),
            .reg_sho_action  = static_cast<RegSHOAction>(b[19]),
        };
    }

    // ── 1.2.4 Market Participant Position ───────────────────────────────────
    inline MarketParticipantPosition parse_market_participant_position(const uint8_t* b) {
        return {
            .stock_locate             = read_be<Locate>(b + 1),
            .tracking_number          = read_be<TrackingNumber>(b + 3),
            .timestamp_ns             = Timestamp{read_u48(b + 5)},
            .mpid                     = read_alpha<4>(b + 11),
            .stock                    = read_alpha<8>(b + 15),
            .primary_market_maker     = static_cast<PrimaryMarketMaker>(b[23]),
            .market_maker_mode        = static_cast<MarketMakerMode>(b[24]),
            .market_participant_state = static_cast<MarketParticipantState>(b[25]),
        };
    }

    // ── 1.2.5.1 MWCB Decline Level ──────────────────────────────────────────
    inline MWCBDeclineLevel parse_mwcb_decline_level(const uint8_t* b) {
        return {
            .stock_locate    = read_be<Locate>(b + 1),
            .tracking_number = read_be<TrackingNumber>(b + 3),
            .timestamp_ns    = Timestamp{read_u48(b + 5)},
            .level_1         = read_be<Price8>(b + 11),
            .level_2         = read_be<Price8>(b + 19),
            .level_3         = read_be<Price8>(b + 27),
        };
    }

    // ── 1.2.5.2 MWCB Status ─────────────────────────────────────────────────
    inline MWCBStatus parse_mwcb_status(const uint8_t* b) {
        return {
            .stock_locate    = read_be<Locate>(b + 1),
            .tracking_number = read_be<TrackingNumber>(b + 3),
            .timestamp_ns    = Timestamp{read_u48(b + 5)},
            .breached_level  = static_cast<MWCBLevel>(b[11]),
        };
    }

    // ── 1.2.6 IPO Quoting Period Update ─────────────────────────────────────
    inline IPOQuotingPeriodUpdate parse_ipo_quoting_period_update(const uint8_t* b) {
        return {
            .stock_locate                    = read_be<Locate>(b + 1),
            .tracking_number                 = read_be<TrackingNumber>(b + 3),
            .timestamp_ns                    = Timestamp{read_u48(b + 5)},
            .stock                           = read_alpha<8>(b + 11),
            .ipo_quotation_release_time      = read_be<uint32_t>(b + 19),
            .ipo_quotation_release_qualifier = static_cast<IPOQuotationReleaseQualifier>(b[23]),
            .ipo_price                       = read_be<Price4>(b + 24),
        };
    }

    // ── 1.2.7 LULD Auction Collar ───────────────────────────────────────────
    inline LULDAuctionCollar parse_luld_auction_collar(const uint8_t* b) {
        return {
            .stock_locate                   = read_be<Locate>(b + 1),
            .tracking_number                = read_be<TrackingNumber>(b + 3),
            .timestamp_ns                   = Timestamp{read_u48(b + 5)},
            .stock                          = read_alpha<8>(b + 11),
            .auction_collar_reference_price = read_be<Price4>(b + 19),
            .upper_auction_collar_price     = read_be<Price4>(b + 23),
            .lower_auction_collar_price     = read_be<Price4>(b + 27),
            .auction_collar_extension       = read_be<uint32_t>(b + 31),
        };
    }

    // ── 1.2.8 Operational Halt ──────────────────────────────────────────────
    inline OperationalHalt parse_operational_halt(const uint8_t* b) {
        return {
            .stock_locate    = read_be<Locate>(b + 1),
            .tracking_number = read_be<TrackingNumber>(b + 3),
            .timestamp_ns    = Timestamp{read_u48(b + 5)},
            .stock           = read_alpha<8>(b + 11),
            .market_code     = static_cast<OperationalMarketCode>(b[19]),
            .halt_action     = static_cast<OperationalHaltAction>(b[20]),
        };
    }

    // ── 1.3.1 Add Order ─────────────────────────────────────────────────────
    inline AddOrder parse_add_order(const uint8_t* b) {
        return {
            .stock_locate    = read_be<Locate>(b + 1),
            .tracking_number = read_be<TrackingNumber>(b + 3),
            .timestamp_ns    = Timestamp{read_u48(b + 5)},
            .order_ref       = read_be<OrderRef>(b + 11),
            .side            = static_cast<Side>(b[19]),
            .shares          = read_be<Shares>(b + 20),
            .stock           = read_alpha<8>(b + 24),
            .price           = read_be<Price4>(b + 32),
        };
    }

    // ── 1.3.2 Add Order with MPID ───────────────────────────────────────────
    inline AddOrderMPID parse_add_order_mpid(const uint8_t* b) {
        return {
            .stock_locate    = read_be<Locate>(b + 1),
            .tracking_number = read_be<TrackingNumber>(b + 3),
            .timestamp_ns    = Timestamp{read_u48(b + 5)},
            .order_ref       = read_be<OrderRef>(b + 11),
            .side            = static_cast<Side>(b[19]),
            .shares          = read_be<Shares>(b + 20),
            .stock           = read_alpha<8>(b + 24),
            .price           = read_be<Price4>(b + 32),
            .attribution     = read_alpha<4>(b + 36),
        };
    }

    // ── 1.4.1 Order Executed ────────────────────────────────────────────────
    inline OrderExecuted parse_order_executed(const uint8_t* b) {
        return {
            .stock_locate    = read_be<Locate>(b + 1),
            .tracking_number = read_be<TrackingNumber>(b + 3),
            .timestamp_ns    = Timestamp{read_u48(b + 5)},
            .order_ref       = read_be<OrderRef>(b + 11),
            .executed_shares = read_be<Shares>(b + 19),
            .match_number    = read_be<MatchNum>(b + 23),
        };
    }

    // ── 1.4.2 Order Executed With Price ─────────────────────────────────────
    inline OrderExecutedWithPrice parse_order_executed_with_price(const uint8_t* b) {
        return {
            .stock_locate    = read_be<Locate>(b + 1),
            .tracking_number = read_be<TrackingNumber>(b + 3),
            .timestamp_ns    = Timestamp{read_u48(b + 5)},
            .order_ref       = read_be<OrderRef>(b + 11),
            .executed_shares = read_be<Shares>(b + 19),
            .match_number    = read_be<MatchNum>(b + 23),
            .printable       = static_cast<Printable>(b[31]),
            .execution_price = read_be<Price4>(b + 32),
        };
    }

    // ── 1.4.3 Order Cancel ──────────────────────────────────────────────────
    inline OrderCancel parse_order_cancel(const uint8_t* b) {
        return {
            .stock_locate     = read_be<Locate>(b + 1),
            .tracking_number  = read_be<TrackingNumber>(b + 3),
            .timestamp_ns     = Timestamp{read_u48(b + 5)},
            .order_ref        = read_be<OrderRef>(b + 11),
            .cancelled_shares = read_be<Shares>(b + 19),
        };
    }

    // ── 1.4.4 Order Delete ──────────────────────────────────────────────────
    inline OrderDelete parse_order_delete(const uint8_t* b) {
        return {
            .stock_locate    = read_be<Locate>(b + 1),
            .tracking_number = read_be<TrackingNumber>(b + 3),
            .timestamp_ns    = Timestamp{read_u48(b + 5)},
            .order_ref       = read_be<OrderRef>(b + 11),
        };
    }

    // ── 1.4.5 Order Replace ─────────────────────────────────────────────────
    inline OrderReplace parse_order_replace(const uint8_t* b) {
        return {
            .stock_locate       = read_be<Locate>(b + 1),
            .tracking_number    = read_be<TrackingNumber>(b + 3),
            .timestamp_ns       = Timestamp{read_u48(b + 5)},
            .original_order_ref = read_be<OrderRef>(b + 11),
            .new_order_ref      = read_be<OrderRef>(b + 19),
            .shares             = read_be<Shares>(b + 27),
            .price              = read_be<Price4>(b + 31),
        };
    }

    // ── 1.5.1 Non-Cross Trade ───────────────────────────────────────────────
    inline NonCrossTrade parse_non_cross_trade(const uint8_t* b) {
        return {
            .stock_locate    = read_be<Locate>(b + 1),
            .tracking_number = read_be<TrackingNumber>(b + 3),
            .timestamp_ns    = Timestamp{read_u48(b + 5)},
            .order_ref       = read_be<OrderRef>(b + 11),
            .side            = static_cast<Side>(b[19]),
            .shares          = read_be<Shares>(b + 20),
            .stock           = read_alpha<8>(b + 24),
            .price           = read_be<Price4>(b + 32),
            .match_number    = read_be<MatchNum>(b + 36),
        };
    }

    // ── 1.5.2 Cross Trade ───────────────────────────────────────────────────
    inline CrossTrade parse_cross_trade(const uint8_t* b) {
        return {
            .stock_locate    = read_be<Locate>(b + 1),
            .tracking_number = read_be<TrackingNumber>(b + 3),
            .timestamp_ns    = Timestamp{read_u48(b + 5)},
            .shares          = read_be<Shares>(b + 11),
            .stock           = read_alpha<8>(b + 15),
            .cross_price     = read_be<Price4>(b + 23),
            .match_number    = read_be<MatchNum>(b + 27),
            .cross_type      = static_cast<CrossType>(b[35]),
        };
    }

    // ── 1.5.3 Broken Trade ──────────────────────────────────────────────────
    inline BrokenTrade parse_broken_trade(const uint8_t* b) {
        return {
            .stock_locate    = read_be<Locate>(b + 1),
            .tracking_number = read_be<TrackingNumber>(b + 3),
            .timestamp_ns    = Timestamp{read_u48(b + 5)},
            .match_number    = read_be<MatchNum>(b + 11),
        };
    }

    // ── 1.6 NOII ────────────────────────────────────────────────────────────
    inline NOII parse_noii(const uint8_t* b) {
        return {
            .stock_locate              = read_be<Locate>(b + 1),
            .tracking_number           = read_be<TrackingNumber>(b + 3),
            .timestamp_ns              = Timestamp{read_u48(b + 5)},
            .paired_shares             = read_be<Shares>(b + 11),
            .imbalance_shares          = read_be<Shares>(b + 15),
            .imbalance_direction       = static_cast<ImbalanceDirection>(b[19]),
            .stock                     = read_alpha<8>(b + 20),
            .far_price                 = read_be<Price4>(b + 28),
            .near_price                = read_be<Price4>(b + 32),
            .current_reference_price   = read_be<Price4>(b + 36),
            .cross_type                = static_cast<CrossType>(b[40]),
            .price_variation_indicator = static_cast<PriceVariationIndicator>(b[41]),
        };
    }

    // ── 1.7 RPII ────────────────────────────────────────────────────────────
    inline RPII parse_rpii(const uint8_t* b) {
        return {
            .stock_locate    = read_be<Locate>(b + 1),
            .tracking_number = read_be<TrackingNumber>(b + 3),
            .timestamp_ns    = Timestamp{read_u48(b + 5)},
            .stock           = read_alpha<8>(b + 11),
            .interest_flag   = static_cast<RPIIFlag>(b[19]),
        };
    }

    // ── 1.8 Direct Listing with Capital Raise ───────────────────────────────
    inline DirectListingWithCapitalRaise parse_direct_listing_capital_raise(const uint8_t* b) {
        return {
            .stock_locate             = read_be<Locate>(b + 1),
            .tracking_number          = read_be<TrackingNumber>(b + 3),
            .timestamp_ns             = Timestamp{read_u48(b + 5)},
            .stock                    = read_alpha<8>(b + 11),
            .open_eligibility_status  = static_cast<OpenEligibilityStatus>(b[19]),
            .minimum_allowable_price  = read_be<Price4>(b + 20),
            .maximum_allowable_price  = read_be<Price4>(b + 24),
            .near_execution_price     = read_be<Price4>(b + 28),
            .near_execution_time      = read_be<Timestamp>(b + 32),
            .lower_price_range_collar = read_be<Price4>(b + 40),
            .upper_price_range_collar = read_be<Price4>(b + 44),
        };
    }

    // ── Legacy value-returning dispatcher (kept for non-hot-path use) ────────
    inline std::optional<ITCHMessage> parse(const uint8_t* buf, uint16_t /*len*/) {
        switch (static_cast<MessageType>(buf[0])) {
            case MessageType::SystemEvent:               return parse_system_event(buf);
            case MessageType::StockDirectory:            return parse_stock_directory(buf);
            case MessageType::StockTradingAction:        return parse_stock_trading_action(buf);
            case MessageType::RegSHORestriction:         return parse_reg_sho_restriction(buf);
            case MessageType::MarketParticipantPosition: return parse_market_participant_position(buf);
            case MessageType::MWCBDeclineLevel:          return parse_mwcb_decline_level(buf);
            case MessageType::MWCBStatus:                return parse_mwcb_status(buf);
            case MessageType::IPOQuotingPeriodUpdate:    return parse_ipo_quoting_period_update(buf);
            case MessageType::LULDAuctionCollar:         return parse_luld_auction_collar(buf);
            case MessageType::OperationalHalt:           return parse_operational_halt(buf);
            case MessageType::AddOrder:                  return parse_add_order(buf);
            case MessageType::AddOrderMPID:              return parse_add_order_mpid(buf);
            case MessageType::OrderExecuted:             return parse_order_executed(buf);
            case MessageType::OrderExecutedWithPrice:    return parse_order_executed_with_price(buf);
            case MessageType::OrderCancel:               return parse_order_cancel(buf);
            case MessageType::OrderDelete:               return parse_order_delete(buf);
            case MessageType::OrderReplace:              return parse_order_replace(buf);
            case MessageType::NonCrossTrade:             return parse_non_cross_trade(buf);
            case MessageType::CrossTrade:                return parse_cross_trade(buf);
            case MessageType::BrokenTrade:               return parse_broken_trade(buf);
            case MessageType::NOII:                      return parse_noii(buf);
            case MessageType::RPII:                      return parse_rpii(buf);
            case MessageType::DirectListingCapitalRaise: return parse_direct_listing_capital_raise(buf);
            default:                                     return std::nullopt;
        }
    }

    // ── Zero-allocation callback dispatcher ───────────────────────────────────
    // Handler must be callable with each concrete message struct type.
    // Use a generic lambda: [&](auto&& m) { ... }
    // No ITCHMessage variant is constructed. No std::optional. No std::visit.
    // The compiler inlines the entire dispatch + parse + handler body per message type.
    template<typename Handler>
    inline bool dispatch(const uint8_t* buf, uint16_t /*len*/, Handler&& h) {
        switch (static_cast<MessageType>(buf[0])) {
            case MessageType::AddOrder:                  h(parse_add_order(buf));                    return true;
            case MessageType::OrderDelete:               h(parse_order_delete(buf));                 return true;
            case MessageType::OrderExecuted:             h(parse_order_executed(buf));               return true;
            case MessageType::OrderReplace:              h(parse_order_replace(buf));                return true;
            case MessageType::OrderCancel:               h(parse_order_cancel(buf));                 return true;
            case MessageType::AddOrderMPID:              h(parse_add_order_mpid(buf));               return true;
            case MessageType::OrderExecutedWithPrice:    h(parse_order_executed_with_price(buf));    return true;
            case MessageType::NonCrossTrade:             h(parse_non_cross_trade(buf));              return true;
            case MessageType::CrossTrade:                h(parse_cross_trade(buf));                  return true;
            case MessageType::SystemEvent:               h(parse_system_event(buf));                 return true;
            case MessageType::StockDirectory:            h(parse_stock_directory(buf));              return true;
            case MessageType::StockTradingAction:        h(parse_stock_trading_action(buf));         return true;
            case MessageType::RegSHORestriction:         h(parse_reg_sho_restriction(buf));          return true;
            case MessageType::MarketParticipantPosition: h(parse_market_participant_position(buf));  return true;
            case MessageType::MWCBDeclineLevel:          h(parse_mwcb_decline_level(buf));           return true;
            case MessageType::MWCBStatus:                h(parse_mwcb_status(buf));                  return true;
            case MessageType::IPOQuotingPeriodUpdate:    h(parse_ipo_quoting_period_update(buf));    return true;
            case MessageType::LULDAuctionCollar:         h(parse_luld_auction_collar(buf));          return true;
            case MessageType::OperationalHalt:           h(parse_operational_halt(buf));             return true;
            case MessageType::BrokenTrade:               h(parse_broken_trade(buf));                 return true;
            case MessageType::NOII:                      h(parse_noii(buf));                         return true;
            case MessageType::RPII:                      h(parse_rpii(buf));                         return true;
            case MessageType::DirectListingCapitalRaise: h(parse_direct_listing_capital_raise(buf)); return true;
            default:                                     return false;
        }
    }
}
