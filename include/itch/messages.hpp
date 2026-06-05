#pragma once
#include "types.hpp"
#include "fields.hpp"
#include <variant>

namespace itch {

    // ── 1.1 System Event ──────────────────────────────────────────────────────
    struct SystemEvent {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        EventCode      event_code;
    };
    
    // ── 1.2.1 Stock Directory ─────────────────────────────────────────────────
    struct StockDirectory {
        Locate              stock_locate;
        TrackingNumber      tracking_number;
        Timestamp           timestamp_ns;
        Alpha<8>            stock;
        MarketCategory      market_category;
        FinancialStatus     financial_status;
        Shares              round_lot_size;
        RoundLotsOnly       round_lots_only;
        IssueClassification issue_classification;
        Alpha<2>            issue_sub_type;
        Authenticity        authenticity;
        ShortSaleThreshold  short_sale_threshold;
        IPOFlag             ipo_flag;
        LULDTier            luld_tier;
        ETPFlag             etp_flag;
        uint32_t            etp_leverage_factor;   // numeric quantity, not a code
        InverseIndicator    inverse_indicator;
    };
    
    // ── 1.2.2 Stock Trading Action ────────────────────────────────────────────
    struct StockTradingAction {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        Alpha<8>       stock;
        TradingState   trading_state;
        char           reserved;                   // always space, ignore
        Alpha<4>       reason;                     // open-ended code, not enum
    };
    
    // ── 1.2.3 Reg SHO Short Sale Price Test Restricted Indicator ─────────────
    struct RegSHORestriction {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        Alpha<8>       stock;
        RegSHOAction   reg_sho_action;
    };
    
    // ── 1.2.4 Market Participant Position ─────────────────────────────────────
    struct MarketParticipantPosition {
        Locate                 stock_locate;
        TrackingNumber         tracking_number;
        Timestamp              timestamp_ns;
        Alpha<4>               mpid;               // e.g. "GSCO" — open-ended
        Alpha<8>               stock;
        PrimaryMarketMaker     primary_market_maker;
        MarketMakerMode        market_maker_mode;
        MarketParticipantState market_participant_state;
    };
    
    // ── 1.2.5.1 MWCB Decline Level Message ───────────────────────────────────
    struct MWCBDeclineLevel {
        Locate         stock_locate;               // always 0
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        Price8         level_1;                    // 7% breach threshold
        Price8         level_2;                    // 13% breach threshold
        Price8         level_3;                    // 20% breach threshold
    };
    
    // ── 1.2.5.2 MWCB Status Message ──────────────────────────────────────────
    struct MWCBStatus {
        Locate         stock_locate;               // always 0
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        MWCBLevel      breached_level;             // '1', '2', or '3'
    };
    
    // ── 1.2.6 IPO Quoting Period Update ──────────────────────────────────────
    struct IPOQuotingPeriodUpdate {
        Locate              stock_locate;          // always 0
        TrackingNumber      tracking_number;
        Timestamp           timestamp_ns;
        Alpha<8>            stock;
        uint32_t            ipo_quotation_release_time;  // seconds since midnight
        IPOQuotationReleaseQualifier ipo_quotation_release_qualifier;
        Price4              ipo_price;
    };
    
    // ── 1.2.7 LULD Auction Collar ─────────────────────────────────────────────
    struct LULDAuctionCollar {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        Alpha<8>       stock;
        Price4         auction_collar_reference_price;
        Price4         upper_auction_collar_price;
        Price4         lower_auction_collar_price;
        uint32_t       auction_collar_extension;   // number of extensions granted
    };
    
    // ── 1.2.8 Operational Halt ────────────────────────────────────────────────
    // NOTE: message type byte is lowercase 'h' (0x68), distinct from 'H' (0x48)
    struct OperationalHalt {
        Locate               stock_locate;
        TrackingNumber       tracking_number;
        Timestamp            timestamp_ns;
        Alpha<8>             stock;
        OperationalMarketCode market_code;         // 'Q', 'B', or 'X'
        OperationalHaltAction halt_action;         // 'H' halted, 'T' resumed
    };
    
    // ── 1.3.1 Add Order – No MPID Attribution ────────────────────────────────
    struct AddOrder {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        OrderRef       order_ref;
        Side           side;
        Shares         shares;
        Alpha<8>       stock;
        Price4         price;
    };
    
    // ── 1.3.2 Add Order with MPID Attribution ────────────────────────────────
    struct AddOrderMPID {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        OrderRef       order_ref;
        Side           side;
        Shares         shares;
        Alpha<8>       stock;
        Price4         price;
        Alpha<4>       attribution;                // MPID e.g. "GSCO"
    };
    
    // ── 1.4.1 Order Executed ──────────────────────────────────────────────────
    struct OrderExecuted {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        OrderRef       order_ref;
        Shares         executed_shares;
        MatchNum    match_number;
    };
    
    // ── 1.4.2 Order Executed With Price ───────────────────────────────────────
    struct OrderExecutedWithPrice {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        OrderRef       order_ref;
        Shares         executed_shares;
        MatchNum    match_number;
        Printable      printable;                  // 'Y' or 'N'
        Price4         execution_price;
    };
    
    // ── 1.4.3 Order Cancel ────────────────────────────────────────────────────
    struct OrderCancel {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        OrderRef       order_ref;
        Shares         cancelled_shares;           // shares removed, not remaining
    };
    
    // ── 1.4.4 Order Delete ────────────────────────────────────────────────────
    struct OrderDelete {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        OrderRef       order_ref;
    };
    
    // ── 1.4.5 Order Replace ───────────────────────────────────────────────────
    // Original order ref is dead. New order ref is born. Side/stock/MPID unchanged.
    struct OrderReplace {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        OrderRef       original_order_ref;
        OrderRef       new_order_ref;
        Shares         shares;
        Price4         price;
    };
    
    // ── 1.5.1 Trade Message (Non-Cross) ──────────────────────────────────────
    // Reflects execution of a non-displayable order.
    // order_ref is always 0 since Dec 2010 (per spec note).
    // side is always 'B' since July 2014 (per spec note).
    struct NonCrossTrade {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        OrderRef       order_ref;                  // always 0 — kept for wire fidelity
        Side           side;                       // always Buy — kept for wire fidelity
        Shares         shares;
        Alpha<8>       stock;
        Price4         price;
        MatchNum    match_number;
    };
    
    // ── 1.5.2 Cross Trade Message ─────────────────────────────────────────────
    // Single bulk print per symbol for Opening/Closing/IPO/Halt cross events.
    struct CrossTrade {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        Shares         shares;                     // total shares matched in cross
        Alpha<8>       stock;
        Price4         cross_price;
        MatchNum    match_number;
        CrossType      cross_type;                 // 'O','C','H','I'
    };
    
    // ── 1.5.3 Broken Trade / Order Execution Message ──────────────────────────
    // Sent when a previously reported execution is busted.
    // Has no impact on the order book — only affects time-and-sales.
    struct BrokenTrade {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        MatchNum    match_number;               // refers to a prior E, C, or P message
    };
    
    // ── 1.6 Net Order Imbalance Indicator (NOII) ──────────────────────────────
    // Disseminated 9:25–9:30 AM (opening) and 3:50–4:00 PM (closing).
    // Best public predictor of Nasdaq opening/closing cross price.
    struct NOII {
        Locate              stock_locate;
        TrackingNumber      tracking_number;
        Timestamp           timestamp_ns;
        Shares              paired_shares;         // eligible to match at ref price
        Shares              imbalance_shares;      // unpaired excess
        ImbalanceDirection  imbalance_direction;   // 'B','S','N','O','P'
        Alpha<8>            stock;
        Price4              far_price;             // cross-orders only clearing price
        Price4              near_price;            // cross + continuous orders
        Price4              current_reference_price;
        CrossType           cross_type;            // 'O','C','H','A'
        PriceVariationIndicator price_variation_indicator;
    };
    
    // ── 1.7 Retail Price Improvement Indicator (RPII) ─────────────────────────
    struct RPII {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        Alpha<8>       stock;
        RPIIFlag       interest_flag;              // 'B','S','A','N'
    };
    
    // ── 1.8 Direct Listing with Capital Raise Price Discovery ─────────────────
    // Only disseminated for DLCR securities after volatility test passes.
    struct DirectListingWithCapitalRaise {
        Locate         stock_locate;
        TrackingNumber tracking_number;
        Timestamp      timestamp_ns;
        Alpha<8>       stock;
        OpenEligibilityStatus open_eligibility_status;  // 'Y' or 'N'
        Price4         minimum_allowable_price;    // 20% below registration lower
        Price4         maximum_allowable_price;    // 80% above registration highest
        Price4         near_execution_price;
        Timestamp      near_execution_time;        // spec: 8-byte integer timestamp
        Price4         lower_price_range_collar;   // 10% below near execution
        Price4         upper_price_range_collar;   // 10% above near execution
    };
    
    // ── Tagged union — add new structs above, then add them here ──────────────
    using ITCHMessage = std::variant<
        SystemEvent,
        StockDirectory,
        StockTradingAction,
        RegSHORestriction,
        MarketParticipantPosition,
        MWCBDeclineLevel,
        MWCBStatus,
        IPOQuotingPeriodUpdate,
        LULDAuctionCollar,
        OperationalHalt,
        AddOrder,
        AddOrderMPID,
        OrderExecuted,
        OrderExecutedWithPrice,
        OrderCancel,
        OrderDelete,
        OrderReplace,
        NonCrossTrade,
        CrossTrade,
        BrokenTrade,
        NOII,
        RPII,
        DirectListingWithCapitalRaise
    >;

}