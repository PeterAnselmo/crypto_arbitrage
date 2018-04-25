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

    //bool trade_found = false;
    //int trades_checked = 0;
//    while(!trade_found){

        crypto_exchange* poloniex = new crypto_exchange("poloniex");
        poloniex->populate_trade_pairs();
        //poloniex->print_trade_pairs();
        poloniex->populate_trades();
        poloniex->print_trade_seqs();

        trade_seq* profitable_trade = poloniex->compare_trades();
        if(profitable_trade != nullptr){
            poloniex->execute_trades(profitable_trade);
        }

 //       chrono::seconds duration(10);
 //       this_thread::sleep_for( duration );
 //   }

    return 0;
}
