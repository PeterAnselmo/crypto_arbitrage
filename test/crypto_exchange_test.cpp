
#include "../crypto_exchange.cpp"
#include "gtest/gtest.h"

TEST(CryptoExchangeTest, BalancesArePopulated){

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

int main(int argc, char **argv){

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
    
