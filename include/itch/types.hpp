#pragma once
#include <cstddef>
#include <cstdint>
#include <array>

namespace itch {

    template<size_t N>
    using Alpha = std::array<char, N>;

    template<typename T, typename Tag>
    struct StrongType {
        explicit StrongType(T v) : value(v) {}
        StrongType(): value(0) {}

        T value;

        bool operator==(const StrongType& other) const {return value == other.value;}
        bool operator!=(const StrongType& other) const {return value != other.value;}
        bool operator< (const StrongType& other) const {return value < other.value;}
        bool operator> (const StrongType& other) const {return value > other.value;}

        T raw() const { return value; }
    };

    //Tags (empty structs, exist only to create distinct types)
    struct Price4Tag {};
    struct Price8Tag {};
    struct TimestampTag {};
    struct OrderRefTag {};
    struct MatchNumTag {};
    struct SharesTag {};
    struct LocateTag {};
    struct TrackingNumberTag {};

    // Named types
    using Price4 = StrongType<uint32_t, Price4Tag>;
    using Price8 = StrongType<uint64_t, Price8Tag>;
    using Timestamp = StrongType<uint64_t, TimestampTag>;
    using OrderRef = StrongType<uint64_t, OrderRefTag>;
    using MatchNum = StrongType<uint64_t, MatchNumTag>;
    using Shares = StrongType<uint32_t, SharesTag>;
    using Locate = StrongType<uint16_t, LocateTag>;
    using TrackingNumber = StrongType<uint16_t, TrackingNumberTag>;



    enum class MessageType: uint8_t {
        //System
        SystemEvent = 'S',

        //Stock Related Messages
        StockDirectory = 'R',
        StockTradingAction = 'H',
        RegSHORestriction = 'Y',
        MarketParticipantPos = 'L',

        // Market---Wide Circuit Breaker (MWCB) Messaging
        MWCBeclineLevel = 'V',
        MWCBStatus = 'W',
        IPOQuotingPeriod = 'K',
        LULDAuctionCollar = 'J',
        OperationalHalt = 'h',

        //Add Order Message
        AddOrder = 'A',
        AddOrderMPID = 'F',

        //Modify Order
        OrderExecuted = 'E',
        OrderExecutedWithPrice = 'C',
        OrderCancel = 'X',
        OrderDelete = 'D',
        OrderReplace = 'U',

        //Trades Messages
        NonCrossTrade = 'P',
        CrossTrade = 'Q',
        BrokerTrade = 'B',

        //Auction
        NOII = 'I',
        RPII = 'N',

        //Diret listing
        DirectListingCapRaise = 'O',
    };

}
