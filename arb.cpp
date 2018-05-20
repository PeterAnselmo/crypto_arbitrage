#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>

#include "arb_util.cpp"
#include "crypto_exchange.cpp"

using namespace std;
constexpr int open_order_wait = 600; //10 minutes

int main(int argc, char* argv[]){

    crypto_exchange* poloniex = new crypto_exchange("poloniex");

    while(true){

        if(poloniex->any_open_orders()){
            cout << "Open Orders Found. Sleeping for " << open_order_wait << " seconds..." << endl;
            sleep(open_order_wait);
        } else {
            poloniex->populate_balances();
            poloniex->populate_trade_pairs();

            try{
                poloniex->monitor_trades();
            } catch (int exception){
                if(exception == ARB_TRADE_EXECUTED) {
                    continue;
                } else {
                    return exception;
                }
            }
        }
    }

    delete poloniex;

    return 0;
}
