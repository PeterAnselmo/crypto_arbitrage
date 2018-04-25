#include "../crypto_exchange.cpp"
#include "gtest/gtest.h"

TEST(CryptoExchange, GDAXTradePairsRetrieved){

    crypto_exchange* gdax = new crypto_exchange("gdax-test");

    ASSERT_EQ(0, gdax->num_trade_pairs());
    gdax->populate_trade_pairs();
    ASSERT_EQ(2, gdax->num_trade_pairs());

}
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
TEST(CryptoExchange, PoloniexTradePairsRetrieved){

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");

    ASSERT_EQ(0, poloniex->num_trade_pairs());

    poloniex->populate_trade_pairs();

    ASSERT_EQ(198, poloniex->num_trade_pairs());
}
TEST(CryptoExchange, PoloniexFindsProfitableTrade){

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");

    poloniex->populate_trade_pairs();

    poloniex->populate_trades();
    trade_seq* profitable_trade = poloniex->compare_trades();

    ASSERT_FALSE(profitable_trade == nullptr);
}

