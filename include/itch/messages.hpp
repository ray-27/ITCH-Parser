#pragma once
#include "types.hpp"
#include "fields.hpp"

namespace itch {

    struct SystemEvent {
        Locate stock_locate;
        TrackingNumber tracking_number;
        Timestamp timestamp_ns;
        EventCode event_code;
    };

    struct StockDirectory {
        Locate stock_locate;
        TrackingNumber tracking_number;
        Timestamp timestamp_ns;
        Alpha<8> stock;
        MarketCategory market_category;
        FinancialStatus financial_status;
        Shares round_lot_size;
        RoundLotsOnly round_lots_only; // Y or N
        IssueClassification issue_classification;
        Alpha<2> issue_sub_types;
    };

    struct AddOrder {
        Locate stock_locate;
        TrackingNumber tracking_number;
        Timestamp timestamp_ns;
        OrderRef order_ref;
        Side side;
        Shares shares;
        Alpha<8> stock;
        Price4 price;
    };
}
