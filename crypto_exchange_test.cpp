#include <iostream>
#include <string>
#include <memory>
#include <vector>

bool ARB_DEBUG = true;

#include "arb_util.cpp"
#include "crypto_exchange.cpp"

int main(){ 
    crypto_exchange poloniex = new crypto_exchange("poloniex");


    poloniex.get_poloniex_balances();

}
