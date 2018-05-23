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
    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi.php",
                                                    "ws://anselmo.me:8181");

    poloniex->populate_balances();
    poloniex->populate_trade_pairs();
    ASSERT_EQ("BTC_DASH", poloniex->pair_string(24));
    ASSERT_EQ("XMR_LTC", poloniex->pair_string(137));
    ASSERT_EQ("USDT_BCH", poloniex->pair_string(191));
}
TEST(CryptoExchange, PoloniexFeeIsPopulated){

    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi.php",
                                                    "ws://anselmo.me:8181");
    ASSERT_FLOAT_EQ(0.002500, poloniex->market_fee());
}
TEST(CryptoExchange, PoloniexBalancesArePopulated){

    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi.php",
                                                    "ws://anselmo.me:8181");

    /* can't check before, added to constructor
    ASSERT_FLOAT_EQ(0.0, poloniex->balance("BTC"));
    ASSERT_FLOAT_EQ(0.0, poloniex->balance("XMR"));
    */

    poloniex->populate_balances();


    ASSERT_FLOAT_EQ(1.000, poloniex->balance("BTC"));
    ASSERT_FLOAT_EQ(2.500, poloniex->balance("BCH"));
    ASSERT_FLOAT_EQ(3.250, poloniex->balance("LTC"));
    ASSERT_FLOAT_EQ(0.000, poloniex->balance("XMR"));
}
TEST(CryptoExchange, PoloniexTradePairsRetrieved){

    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi.php",
                                                    "ws://anselmo.me:8181");

    ASSERT_EQ(0, poloniex->num_trade_pairs());

    poloniex->populate_balances();
    poloniex->populate_trade_pairs();

    //198 total, 20 are funded
    //ASSERT_EQ(198, poloniex->num_trade_pairs());
    ASSERT_EQ(20, poloniex->num_trade_pairs());
}
TEST(CryptoExchange, PoloniexTradePairsRetrieved2){
    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public2.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi2.php",
                                                    "ws://anselmo.me:8282");

    poloniex->populate_balances();
    poloniex->populate_trade_pairs();
    trade_pair tp;

    //"XMR_MAID":{"id":138,"last":"0.00159597","lowestAsk":"0.00163977","highestBid":"0.00159599"...
    tp = poloniex->get_pair(138, 'b');
    ASSERT_FLOAT_EQ(0.00163977, tp.quote);
    tp = poloniex->get_pair(138, 's');
    ASSERT_FLOAT_EQ(0.00159599, tp.quote);

    //"BTC_GAS":{"id":198,"last":"0.00322453","lowestAsk":"0.00324822","highestBid":"0.00321199"...
    tp = poloniex->get_pair(198, 'b');
    ASSERT_FLOAT_EQ(0.00324822, tp.quote);
    tp = poloniex->get_pair(198, 's');
    ASSERT_FLOAT_EQ(0.00321199, tp.quote);
}
TEST(CryptoExchange, PoloniexWebSockerPairIsRetrieved){

    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi.php",
                                                    "ws://anselmo.me:8181");

    poloniex->populate_balances();
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
/* broken due to now requiring known depth
TEST(CryptoExchange, PoloniexFindsProfitableTrade){

    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi.php",
                                                    "ws://anselmo.me:8181");

    poloniex->populate_balances();
    poloniex->populate_trade_pairs();

    ASSERT_FLOAT_EQ(2.1, poloniex->balance("ETH"));

    trade_seq* profitable_trade = nullptr;
    ASSERT_TRUE(poloniex->find_trade(profitable_trade));

}
TEST(CryptoExchange, PoloniexFailsUnderfundedSell){

    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi.php",
                                                    "ws://anselmo.me:8181");
    poloniex->populate_balances();

    trade_pair tp;
    strcpy(tp.sell, "XMR");
    strcpy(tp.buy, "BTC");
    tp.quote = 0.030;
    tp.action = 's';

    float amount = poloniex->balance("XMR") /tp.quote;

    trade_seq ts;

    ASSERT_FALSE(ts.add_pair(tp, amount));
}
TEST(CryptoExchange, PoloniexFailsMispricedSell){

    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi.php",
                                                    "ws://anselmo.me:8181");

    poloniex->populate_balances();
	poloniex->populate_trade_pairs();
    trade_pair tp;
    strcpy(tp.sell, "XRP");
    strcpy(tp.buy, "BTC");
    tp.exchange_id = 117;
    tp.quote = 0.00010000;
    tp.action = 's';

    trade_seq* ts = new trade_seq;
    ts->add_pair(tp, poloniex->balance("XRP"));

    ASSERT_FALSE(poloniex->execute_trades(ts));
}
TEST(CryptoExchange, PoloniexSellSucceeds){

    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi.php",
                                                    "ws://anselmo.me:8181");

    poloniex->populate_balances();
    poloniex->populate_trade_pairs();
    trade_pair tp;
    strcpy(tp.sell, "XRP");
    strcpy(tp.buy, "BTC");
    tp.exchange_id = 117;
    tp.quote = 0.00008996;
    tp.action = 's';

    trade_seq* ts = new trade_seq;
    ts->add_pair(tp, poloniex->balance("XRP"));

    ASSERT_FLOAT_EQ(50.0, poloniex->balance("XRP"));
    ASSERT_FLOAT_EQ(1.0, poloniex->balance("BTC"));

    //TODO: Make this exception expectation more specific
    ASSERT_TRUE(poloniex->execute_trades(ts));

    ASSERT_FLOAT_EQ(0.0, poloniex->balance("XRP"));
    ASSERT_FLOAT_EQ(1.00448676, poloniex->balance("BTC"));
}
TEST(CryptoExchange, PoloniexFailsMispricedBuy){

    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi.php",
                                                    "ws://anselmo.me:8181");

    poloniex->populate_balances();
    poloniex->populate_trade_pairs();
    trade_pair tp;
    strcpy(tp.sell, "BTC");
    strcpy(tp.buy, "ZEC");
    tp.exchange_id=178;
    tp.quote = 0.0300;
    tp.action = 'b';

    trade_seq* ts = new trade_seq;
    ts->add_pair(tp, 1.0);

    ASSERT_FLOAT_EQ(1.0, poloniex->balance("BTC"));
    ASSERT_FLOAT_EQ(0, poloniex->balance("ZEC"));

    //TODO: Make this exception expectation more specific
    ASSERT_FALSE(poloniex->execute_trades(ts));

    ASSERT_FLOAT_EQ(1.0, poloniex->balance("BTC"));
    ASSERT_FLOAT_EQ(0, poloniex->balance("ZEC"));
}
TEST(CryptoExchange, PoloniexBuySucceeds){

    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi.php",
                                                    "ws://anselmo.me:8181");

    poloniex->populate_balances();
    poloniex->populate_trade_pairs();
    trade_pair tp;
    strcpy(tp.sell, "BTC");
    strcpy(tp.buy, "XMR");
    tp.exchange_id = 114;
    tp.quote = 0.0299;
    tp.action = 'b';

    trade_seq* ts = new trade_seq;
    ts->add_pair(tp, 1.0);

    ASSERT_FLOAT_EQ(1.0, poloniex->balance("BTC"));
    ASSERT_FLOAT_EQ(0, poloniex->balance("XMR"));

    //TODO: Make this exception expectation more specific
    ASSERT_NO_THROW(poloniex->execute_trades(ts));

    //Have to account for some dust lost due to conversion rounding
    ASSERT_NEAR(0.0, poloniex->balance("BTC"), .001);
    ASSERT_NEAR(33.377959973, poloniex->balance("XMR"), .02);
}
TEST(CryptoExchange, EntireSequenceExcecuted){
    //Trade Seq: ETH>BTC sell@0.07401501 net:0.07382997, BTC>BCH buy@0.15167712 net:6.57646990, BCH>ETH sell@2.25217676 net:2.24654627, Net Change:1.09078932

    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi.php",
                                                    "ws://anselmo.me:8181");


    ASSERT_NO_THROW({
        poloniex->populate_balances();
        poloniex->populate_trade_pairs();

        trade_seq* profitable_trade = nullptr;
        ASSERT_TRUE(poloniex->find_trade(profitable_trade));
        ASSERT_TRUE(poloniex->execute_trades(profitable_trade));
    });

}
TEST(CryptoExchange, EntireWSequenceExecuted){

    //Trade Seq: BTC>ETH b@0.07410000 net:13.46153846, ETH>GAS b@0.04385234 net:22.74679071, GAS>BTC s@0.00421197 net:0.00420144, Net Change:1.28650951
    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public2.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi2.php",
                                                    "ws://anselmo.me:8282");

    poloniex->populate_balances();
    double starting_btc = poloniex->balance("BTC");
    double utilized_amount = starting_btc * balance_utilization;
    double net_profit = 0.28650951 * utilized_amount;

    poloniex->populate_trade_pairs();
    trade_pair tp;

    //"USDT_XMR":{"id":126,"last":"239.25328845","lowestAsk":"239.73639657","highestBid":"239.49254174"...
    trade_pair tp1 = poloniex->get_pair(126, 'b');
    ASSERT_FLOAT_EQ(239.73639657, tp1.quote);//lowest ask from initial http data
    trade_pair tp2 = poloniex->get_pair(126, 's');
    ASSERT_FLOAT_EQ(239.49254174, tp2.quote);//highest bid from initial http data

    //"BTC_GAS":{"id":198,"last":"0.00322453","lowestAsk":"0.00324822","highestBid":"0.00321199"
    tp = poloniex->get_pair(198, 'b');
    ASSERT_FLOAT_EQ(0.00324822, tp.quote);
    tp = poloniex->get_pair(198, 's');
    ASSERT_FLOAT_EQ(0.00321199, tp.quote);

    //test server will disconnect causing exception (but after data is populated)
    ASSERT_ANY_THROW(poloniex->monitor_trades());

    //[1002,null,[126,"238.99999999","239.25328845","239.07169998","-0.00905593","549272.05317976","2348.39754632",0,"241.63999999","228.00000000"]])
    trade_pair tp3 = poloniex->get_pair(126, 'b');
    ASSERT_FLOAT_EQ(239.25328845, tp3.quote);//lowest ask from initial http data
    trade_pair tp4 = poloniex->get_pair(126, 's');
    ASSERT_FLOAT_EQ(239.07169998, tp4.quote);//highest bid from initial http data

    //[1002,null,[198,"0.00322453","0.00424823","0.00421197","0.00586136","12.05556792","3795.09494089",0,"0.00325361","0.00307254"]]
    tp = poloniex->get_pair(198, 'b');
    ASSERT_FLOAT_EQ(0.00424823, tp.quote);
    tp = poloniex->get_pair(198, 's');
    ASSERT_FLOAT_EQ(0.00421197, tp.quote);

    ASSERT_FLOAT_EQ(starting_btc + net_profit, poloniex->balance("BTC"));
}
TEST(CryptoExchange, BuyAmountCorrectlyCalculated){

    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public3.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi3.php",
                                                    "ws://anselmo.me:8282");

    poloniex->populate_balances();
    poloniex->populate_trade_pairs();
    poloniex->print_trade_pairs();

    trade_seq* profitable_trade = nullptr;
    ASSERT_TRUE(poloniex->find_trade(profitable_trade));
    ASSERT_TRUE(poloniex->execute_trades(profitable_trade));
}
*/
TEST(CryptoExchange, LoadsBookFromFeed){
    crypto_exchange* poloniex = new crypto_exchange("poloniex",
                                                    "https://anselmo.me/poloniex/public4.php?command=returnTicker",
                                                    "http://anselmo.me/poloniex/tradingapi3.php",
                                                    "ws://anselmo.me:8282");

    poloniex->populate_balances();
    poloniex->populate_trade_pairs();
    ASSERT_ANY_THROW(poloniex->monitor_trades());

}
