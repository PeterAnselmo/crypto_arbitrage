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

    /*
    long int elapsed;
    vector<long int> times;
    double median;
    */

        //std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        poloniex->populate_trade_pairs();

        poloniex->monitor_trades();

        /*
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
        times.push_back(elapsed);

        std::cout << "Computation Time: = " << elapsed <<std::endl;
        std::sort(times.begin(), times.end());
        median = times[times.size() / 2];
        std::cout << "Median Time: = " << median <<std::endl;
        */

                /*
        if(profitable_trade != nullptr){
            trade_found = true;
            poloniex->execute_trades(profitable_trade);
        }

        poloniex->clear_trades_and_pairs();
        */


    return 0;
}
