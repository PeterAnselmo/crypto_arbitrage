
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

    /*
    trade_pair tp;
    tp.sell = "XRP";
    tp.buy = "BTC";
    tp.quote = 0.00008990;
    tp.action = "sell";

    trade_pair tp;
    tp.sell = "BTC";
    tp.buy = "XMR";
    tp.quote = 0.02989500;
    tp.action = "buy";
    */

    trade_seq ts;
    ts.add_pair(tp);

    poloniex->execute_trades(&ts);


    return 0;
}
