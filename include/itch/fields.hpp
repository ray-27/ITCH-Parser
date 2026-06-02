#pragma once
#include <cstdint>

namespace itch {

    enum class EventCode : char {
        StartOfMessages = 'O',
        StartOfSystemHours = 'S',
        StartOfMarketHours = 'Q',
        EndOfMarketHours = 'M',
        EndOfSystemHours = 'E',
        EndOfMessages = 'C',
    };

    //Stock Directory
    enum class MarketCategory : char {
        NasdaqGlobalSelect = 'Q',
        NasdaqGlobalMarket = 'G',
        NasdaqCapitalMarket = 'S',
        NYSE = 'N',
        NYSEAmerican = 'A',
        NYSEArca = 'P',
        BATS = 'Z',
        IEX = 'V',
        NotAvailable = ' ',
    };

    enum class FinancialStatus : char {
        Deficient = 'D',
        Delinquent = 'E',
        Bankrupt = 'Q',
        Suspended = 'S',
        DeficientAndBankrupt = 'G',
        DeficientAndDelinquent = 'H',
        DelinquentAndBankrupt = 'J',
        DeficientDelinquentBankrupt = 'K',
        ETFRedemptionSuspended = 'C',
        Normal = 'N',
        NotAvailable = ' ',
    };

    enum class RoundLotsOnly : char {
        Yes = 'Y',
        No  = 'N',
    };

    enum class IssueClassification : char {
        AmericanDepositaryShare  = 'A',
        Bond = 'B',
        Notes = 'N',
        CommonStock = 'C',
        DepositoryReceipt = 'F',
        Rule144A = 'I',
        LimitedPartnership = 'L',
        OrdinaryShare = 'O',
        PreferredStock = 'P',
        OtherSecurities = 'Q',
        Right = 'R',
        SharesOfBeneficialInt = 'S',
        ConvertibleDebenture = 'T',
        Unit = 'U',
        UnitsOrBenifInt = 'V',
        Warrant = 'W',
    };

    // Add order
    enum class Side : char {
        Buy  = 'B',
        Sell = 'S',
    };
}
