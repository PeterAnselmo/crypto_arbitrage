
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>

#include "arb_util.cpp"
#include "crypto_exchange.cpp"

using namespace std;

int main(int argc, char* argv[]){

    crypto_exchange* poloniex = new crypto_exchange("poloniex");

    poloniex->populate_trade_pairs();
    trade_pair tp;
    strcpy(tp.sell, "BTC");
    strcpy(tp.buy, "BLK");
    tp.exchange_id = 10;
    tp.quote = 0.00003590;
    tp.action = 'b';


    /*

    trade_pair tp;
    tp.sell = "BTC";
    tp.buy = "XMR";
    tp.quote = 0.02989500;
    tp.action = "buy";
    */

    trade_seq ts;
    ts.add_pair(tp, 0.003);

    poloniex->execute_trades(&ts);


    return 0;
}
