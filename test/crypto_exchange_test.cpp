#include "../crypto_exchange.cpp"
#include "gtest/gtest.h"

/*
TEST(CryptoExchange, GDAXTradePairsRetrieved){

    crypto_exchange* gdax = new crypto_exchange("gdax-test");

    vector<trade_pair> pairs = gdax->get_trade_pairs();

    //"BTC_LTC":{"last":"0.0251","lowestAsk":"0.02589999","highestBid":"0.0251","percentChange":"0.02390438","baseVolume":"6.16485315","quoteVolume":"245.82513926"}

    //TODO: Create = comparison for trade_pairs
    bool has_btc_ltc = false;
    for(const auto& pair : pairs){
        if(pair.quote == "BTC" && pair.base == "LTC"){
            has_btc_ltc = true;
        }
    }
    ASSERT_EQ(true, has_btc_ltc);
}
*/
TEST(CryptoExchange, PoloniexFeeIsPopulated){

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");
    ASSERT_FLOAT_EQ(0.002400, poloniex->market_fee());
}

TEST(CryptoExchange, PoloniexBalancesArePopulated){

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");

    ASSERT_FLOAT_EQ(0.0, poloniex->balance("BTC"));
    ASSERT_FLOAT_EQ(0.0, poloniex->balance("XMR"));

    poloniex->populate_balances();

    ASSERT_FLOAT_EQ(1.000, poloniex->balance("BTC"));
    ASSERT_FLOAT_EQ(2.500, poloniex->balance("BCH"));
    ASSERT_FLOAT_EQ(3.250, poloniex->balance("LTC"));
    ASSERT_FLOAT_EQ(0.000, poloniex->balance("ETH"));
    ASSERT_FLOAT_EQ(0.000, poloniex->balance("XMR"));
}

/*
TEST(CryptoExchange, PoloniexTradePairsRetrieved){

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");

    vector<trade_pair> pairs = poloniex->get_trade_pairs();

    //"BTC_LTC":{"last":"0.0251","lowestAsk":"0.02589999","highestBid":"0.0251","percentChange":"0.02390438","baseVolume":"6.16485315","quoteVolume":"245.82513926"}

    //TODO: Create = comparison for trade_pairs
    bool has_btc_ltc = false;
    for(const auto& pair : pairs){
        if(pair.quote == "BTC" && pair.base == "LTC"){
            has_btc_ltc = true;
        }
    }
    ASSERT_EQ(true, has_btc_ltc);
}
*/

