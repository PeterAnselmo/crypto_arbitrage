
#include <iostream>
#include "arb_util.cpp"
#include "crypto_exchange.cpp"

using namespace std;
constexpr int open_order_wait = 600; //10 minutes

int main(int argc, char* argv[]){


    auto begin = std::chrono::steady_clock::now();
    crypto_exchange* poloniex = new crypto_exchange("poloniex");

    poloniex->populate_balances();

    auto end = std::chrono::steady_clock::now();
    cout << "Total Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " milliseconds" << endl;

    delete poloniex;

    return 0;
}
