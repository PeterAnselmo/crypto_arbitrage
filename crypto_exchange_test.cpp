#include <iostream>
#include <string>
#include <memory>
#include <vector>

bool ARB_DEBUG = false;

#include "arb_util.cpp"
#include "crypto_exchange.cpp"

int main(){ 
    crypto_exchange* poloniex = new crypto_exchange("poloniex");

    poloniex->populate_balances();
    poloniex->print_balances();

}
