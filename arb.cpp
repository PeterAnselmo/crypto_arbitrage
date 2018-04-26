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

    crypto_exchange* poloniex = new crypto_exchange("poloniex-test");
    bool trade_found = false;
    int iterations = 0;
    //int trades_checked = 0;
    while(!trade_found){

        poloniex->populate_trade_pairs();
        //poloniex->print_trade_pairs();
        poloniex->populate_trades();
        poloniex->print_trade_seqs();

        trade_seq* profitable_trade = poloniex->compare_trades();
        if(profitable_trade != nullptr){
            if(profitable_trade->net_gain > 1.01){
                trade_found = true;
            }

            poloniex->execute_trades(profitable_trade);
        }
        poloniex->clear_trades_and_pairs();
        cout << "Data has been checked " << ++iterations << " times." << endl;

       chrono::seconds duration(2);
       this_thread::sleep_for( duration );
   }

    return 0;
}
