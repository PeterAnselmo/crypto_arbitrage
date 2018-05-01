#include "gtest/gtest.h"
#include "../crypto_exchange.cpp"

/*
TEST(CryptoExchange, GDAXTradePairsRetrieved){

    crypto_exchange* gdax = new crypto_exchange("gdax-test");

    ASSERT_EQ(0, gdax->num_trade_pairs());
    gdax->populate_trade_pairs();
    ASSERT_EQ(2, gdax->num_trade_pairs());

}
*/
TEST(CryptoExchange, PoloniexIDsArePopulated){
    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");

    poloniex->populate_trade_pairs();
    ASSERT_EQ("BTC_DASH", poloniex->pair_string(24));
    ASSERT_EQ("XMR_LTC", poloniex->pair_string(137));
    ASSERT_EQ("USDT_BCH", poloniex->pair_string(191));
}
TEST(CryptoExchange, PoloniexFeeIsPopulated){

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");
    ASSERT_FLOAT_EQ(0.002500, poloniex->market_fee());
}

TEST(CryptoExchange, PoloniexBalancesArePopulated){

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");

    /* can't check before, added to constructor
    ASSERT_FLOAT_EQ(0.0, poloniex->balance("BTC"));
    ASSERT_FLOAT_EQ(0.0, poloniex->balance("XMR"));

    poloniex->populate_balances();
    */

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
TEST(CryptoExchange, PoloniexWebSockerPairIsRetrieved){

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");

    poloniex->populate_trade_pairs();

    trade_pair tp1 = poloniex->get_pair(149, 'b');
    ASSERT_FLOAT_EQ(706.76340485, tp1.quote);//lowest ask from initial http data
    trade_pair tp2 = poloniex->get_pair(149, 's');
    ASSERT_FLOAT_EQ(704.53848927, tp2.quote);//highest bid from initial http data

    //test server will disconnect causing exception (but after data is populated)
    ASSERT_ANY_THROW(poloniex->monitor_trades());

    //[149,"655.85494783","656.20257851","653.00808262","-0.04198809","4070403.44163582","6117.26972201",0,"693.99999999","628.17135284"]
    trade_pair tp3 = poloniex->get_pair(149, 'b');
    ASSERT_FLOAT_EQ(656.20257851, tp3.quote);//lowest ask from initial http data
    trade_pair tp4 = poloniex->get_pair(149, 's');
    ASSERT_FLOAT_EQ(653.00808262, tp4.quote);//highest bid from initial http data
        
}
TEST(CryptoExchange, PoloniexFindsProfitableTrade){

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");

    poloniex->populate_trade_pairs();

    poloniex->populate_trades();
    trade_seq* profitable_trade = poloniex->compare_trades();

    ASSERT_FALSE(profitable_trade == nullptr);
}
TEST(CryptoExchange, PoloniexFailsUnderfundedSell){

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");

    trade_pair tp;
    strcpy(tp.sell, "XMR");
    strcpy(tp.buy, "BTC");
    tp.quote = 0.030;
    tp.action = 's';

    trade_seq ts;
    ts.trades.push_back(tp);

    //TODO: Make this exception expectation more specific
    ASSERT_ANY_THROW(poloniex->execute_trades(&ts));
}
TEST(CryptoExchange, PoloniexFailsMispricedSell){

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");

    trade_pair tp;
    strcpy(tp.sell, "XRP");
    strcpy(tp.buy, "BTC");
    tp.quote = 0.00010000;
    tp.action = 's';

    trade_seq ts;
    ts.trades.push_back(tp);

    //TODO: Make this exception expectation more specific
    ASSERT_ANY_THROW(poloniex->execute_trades(&ts));
}
TEST(CryptoExchange, PoloniexSellSucceeds){

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");

    trade_pair tp;
    strcpy(tp.sell, "XRP");
    strcpy(tp.buy, "BTC");
    tp.quote = 0.00008996;
    tp.action = 's';

    trade_seq ts;
    ts.trades.push_back(tp);

    ASSERT_FLOAT_EQ(50.0, poloniex->balance("XRP"));
    ASSERT_FLOAT_EQ(1.0, poloniex->balance("BTC"));

    //TODO: Make this exception expectation more specific
    ASSERT_NO_THROW(poloniex->execute_trades(&ts));

    ASSERT_FLOAT_EQ(0.0, poloniex->balance("XRP"));
    ASSERT_FLOAT_EQ(1.00448676, poloniex->balance("BTC"));
}
TEST(CryptoExchange, PoloniexFailsMispricedBuy){

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");

    trade_pair tp;
    strcpy(tp.sell, "BTC");
    strcpy(tp.buy, "ZEC");
    tp.quote = 0.0300;
    tp.action = 'b';

    trade_seq ts;
    ts.trades.push_back(tp);

    ASSERT_FLOAT_EQ(1.0, poloniex->balance("BTC"));
    ASSERT_FLOAT_EQ(0, poloniex->balance("ZEC"));

    //TODO: Make this exception expectation more specific
    ASSERT_ANY_THROW(poloniex->execute_trades(&ts));

    ASSERT_FLOAT_EQ(1.0, poloniex->balance("BTC"));
    ASSERT_FLOAT_EQ(0, poloniex->balance("ZEC"));
}
TEST(CryptoExchange, PoloniexBuySucceeds){

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");

    trade_pair tp;
    strcpy(tp.sell, "BTC");
    strcpy(tp.buy, "XMR");
    tp.quote = 0.0299;
    tp.action = 'b';

    trade_seq ts;
    ts.trades.push_back(tp);

    ASSERT_FLOAT_EQ(1.0, poloniex->balance("BTC"));
    ASSERT_FLOAT_EQ(0, poloniex->balance("XMR"));

    //TODO: Make this exception expectation more specific
    ASSERT_NO_THROW(poloniex->execute_trades(&ts));

    //Have to account for some dust lost due to conversion rounding
    ASSERT_NEAR(0.0, poloniex->balance("BTC"), .001);
    ASSERT_NEAR(33.377959973, poloniex->balance("XMR"), .02);
}
TEST(CryptoExchange, EntireSequenceExcecuted){
    //Trade Seq: ETH>BTC sell@0.07401501 net:0.07382997, BTC>BCH buy@0.15167712 net:6.57646990, BCH>ETH sell@2.25217676 net:2.24654627, Net Change:1.09078932

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");

    float starting_balance = poloniex->balance("BTC");
    float target_balance = starting_balance * 1.09078932;

    ASSERT_NO_THROW({
        poloniex->populate_trade_pairs();
        poloniex->populate_trades();

        trade_seq* profitable_trade = poloniex->compare_trades();
        poloniex->execute_trades(profitable_trade);
    });

    ASSERT_FLOAT_EQ(target_balance, poloniex->balance("BTC"));
}
