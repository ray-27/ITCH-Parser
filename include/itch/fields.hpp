#pragma once

namespace itch {

    // ── 1.1 System Event ──────────────────────────────────────────────────
    enum class EventCode : char {
        StartOfMessages    = 'O',
        StartOfSystemHours = 'S',
        StartOfMarketHours = 'Q',
        EndOfMarketHours   = 'M',
        EndOfSystemHours   = 'E',
        EndOfMessages      = 'C',
    };

    // ── 1.2.1 Stock Directory ─────────────────────────────────────────────
    enum class MarketCategory : char {
        NasdaqGlobalSelect  = 'Q',
        NasdaqGlobalMarket  = 'G',
        NasdaqCapitalMarket = 'S',
        NYSE                = 'N',
        NYSEAmerican        = 'A',
        NYSEArca            = 'P',
        BATS                = 'Z',
        IEX                 = 'V',
        NotAvailable        = ' ',
    };

    enum class FinancialStatus : char {
        Deficient                   = 'D',
        Delinquent                  = 'E',
        Bankrupt                    = 'Q',
        Suspended                   = 'S',
        DeficientAndBankrupt        = 'G',
        DeficientAndDelinquent      = 'H',
        DelinquentAndBankrupt       = 'J',
        DeficientDelinquentBankrupt = 'K',
        ETFRedemptionSuspended      = 'C',
        Normal                      = 'N',
        NotAvailable                = ' ',
    };

    enum class RoundLotsOnly : char {
        Yes = 'Y',
        No  = 'N',
    };

    enum class IssueClassification : char {
        AmericanDepositaryShare = 'A',
        Bond                    = 'B',
        CommonStock             = 'C',
        DepositoryReceipt       = 'F',
        Rule144A                = 'I',
        LimitedPartnership      = 'L',
        Notes                   = 'N',
        OrdinaryShare           = 'O',
        PreferredStock          = 'P',
        OtherSecurities         = 'Q',
        Right                   = 'R',
        SharesOfBeneficialInt   = 'S',
        ConvertibleDebenture    = 'T',
        Unit                    = 'U',
        UnitsOrBenifInt         = 'V',
        Warrant                 = 'W',
    };

    enum class Authenticity : char {
        Live = 'P',
        Test = 'T',
    };

    enum class ShortSaleThreshold : char {
        Restricted    = 'Y',
        NotRestricted = 'N',
        NotAvailable  = ' ',
    };

    enum class IPOFlag : char {
        IsIPO        = 'Y',
        NotIPO       = 'N',
        NotAvailable = ' ',
    };

    enum class LULDTier : char {
        Tier1        = '1',
        Tier2        = '2',
        NotAvailable = ' ',
    };

    enum class ETPFlag : char {
        IsETP        = 'Y',
        NotETP       = 'N',
        NotAvailable = ' ',
    };

    enum class InverseIndicator : char {
        Inverse    = 'Y',
        NotInverse = 'N',
    };

    // ── 1.2.2 Stock Trading Action ────────────────────────────────────────
    enum class TradingState : char {
        Halted        = 'H',
        Paused        = 'P',
        QuotationOnly = 'Q',
        Trading       = 'T',
    };

    // ── 1.2.4 Market Participant Position ─────────────────────────────────
    enum class PrimaryMarketMaker : char {
        Yes = 'Y',
        No  = 'N',
    };

    enum class MarketMakerMode : char {
        Normal       = 'N',
        Passive      = 'P',
        Syndicate    = 'S',
        PreSyndicate = 'R',
        Penalty      = 'L',
    };

    enum class MarketParticipantState : char {
        Active           = 'A',
        ExcusedWithdrawn = 'E',
        Withdrawn        = 'W',
        Suspended        = 'S',
        Deleted          = 'D',
    };

    // ── 1.2.3 Reg SHO ─────────────────────────────────────────────────────
    // Values are ASCII digit characters, not letters
    enum class RegSHOAction : char {
        NoPriceTest         = '0',
        RestrictionInEffect = '1',
        RestrictionRemains  = '2',
    };

    // ── 1.2.5 Market-Wide Circuit Breaker ─────────────────────────────────
    // Values are ASCII digit characters
    enum class MWCBLevel : char {
        Level1 = '1',
        Level2 = '2',
        Level3 = '3',
    };

    // ── 1.2.6 IPO Quoting Period Update ──────────────────────────────────
    enum class IPOQuotationReleaseQualifier : char {
        Anticipated         = 'A',
        CanceledOrPostponed = 'C',
    };

    // ── 1.2.8 Operational Halt ────────────────────────────────────────────
    enum class OperationalMarketCode : char {
        Nasdaq = 'Q',
        BX     = 'B',
        PSX    = 'X',
    };

    enum class OperationalHaltAction : char {
        Halted  = 'H',
        Resumed = 'T',
    };

    // ── 1.3 Add Order ─────────────────────────────────────────────────────
    enum class Side : char {
        Buy  = 'B',
        Sell = 'S',
    };

    // ── 1.4.2 Order Executed With Price ───────────────────────────────────
    enum class Printable : char {
        Yes = 'Y',
        No  = 'N',
    };

    // ── 1.5.2 Cross Trade / 1.6 NOII ─────────────────────────────────────
    enum class CrossType : char {
        Opening             = 'O',
        Closing             = 'C',
        IPOOrHalted         = 'H',
        ExtendedTradingClose = 'A',
    };

    // ── 1.6 NOII ──────────────────────────────────────────────────────────
    enum class ImbalanceDirection : char {
        Buy                = 'B',
        Sell               = 'S',
        NoImbalance        = 'N',
        InsufficientOrders = 'O',
        Paused             = 'P',
    };

    enum class PriceVariationIndicator : char {
        LessThan1Pct  = 'L',
        Pct1To2       = '1',
        Pct2To3       = '2',
        Pct3To4       = '3',
        Pct4To5       = '4',
        Pct5To6       = '5',
        Pct6To7       = '6',
        Pct7To8       = '7',
        Pct8To9       = '8',
        Pct9To10      = '9',
        Pct10To20     = 'A',
        Pct20To30     = 'B',
        Pct30OrMore   = 'C',
        CannotCalculate = ' ',
    };

    // ── 1.7 RPII ──────────────────────────────────────────────────────────
    enum class RPIIFlag : char {
        BuySide  = 'B',
        SellSide = 'S',
        BothSides = 'A',
        None     = 'N',
    };

    // ── 1.8 Direct Listing with Capital Raise ─────────────────────────────
    enum class OpenEligibilityStatus : char {
        Eligible    = 'Y',
        NotEligible = 'N',
    };

}
