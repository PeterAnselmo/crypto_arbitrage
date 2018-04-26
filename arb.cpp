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
    bool trade_found = false;
    int iterations = 0;

    while(!trade_found){
        cout << "Data has been checked " << ++iterations << " times." << endl;
        chrono::seconds duration(1);
        this_thread::sleep_for( duration );

        poloniex->populate_trade_pairs();
        poloniex->populate_trades();

        trade_seq* profitable_trade = poloniex->compare_trades();

        if(profitable_trade != nullptr){
            trade_found = true;
            poloniex->execute_trades(profitable_trade);
        }

        poloniex->clear_trades_and_pairs();

   }

    return 0;
}
