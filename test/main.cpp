#include "crypto_exchange_test.cpp"

int main(int argc, char **argv){

    /*
    crypto_market market = new crypto_market;
    crypto_exchange* gdax = new crypto_exchange("gdax-test");
    //gdax->get_trade_pairs();
    gdax.import_pairs(gdax);
    market.add_pairs(gdax);

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");
    cout << "Market Fee: " <<  poloniex->market_fee() << endl;
    poloniex->populate_trade_pairs();
    poloniex->print_trade_pairs();
    poloniex->populate_trades();
    poloniex->execute_trades();

    */
    ::testing::InitGoogleTest(&argc, argv);

    int retval;
    retval = RUN_ALL_TESTS();


    return retval;
}
    
