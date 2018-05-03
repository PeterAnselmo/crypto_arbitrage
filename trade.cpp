
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
    trade_seq ts;

    trade_pair tp;
    strcpy(tp.sell, "BTC");
    strcpy(tp.buy, "CVC");
    tp.exchange_id = 194;
    tp.quote = 0.00004995;
    tp.action = 'b';
    ts.add_pair(tp, .00300);

    trade_pair tp1;
    strcpy(tp1.sell, "BTC");
    strcpy(tp1.buy, "GNO");
    tp1.exchange_id = 187;
    tp1.quote = 0.01250871;
    tp1.action = 'b';
    ts.add_pair(tp1, .00300);


    trade_pair tp3;
    strcpy(tp3.sell, "BTC");
    strcpy(tp3.buy, "NXT");
    tp3.exchange_id = 69;
    tp3.quote = 0.00002636;
    tp3.action = 'b';
    ts.add_pair(tp3, .00300);

    poloniex->execute_trades(&ts);

    return 0;
}
