#ifndef CRYPTO_EXCHANGE_H
#define CRYPTO_EXCHANGE_H

#include <iostream>
#include <string>

using namespace std;

class crypto_exchange {
    string _name;
    float _market_fee;

    public:

    crypto_exchange(string new_name){
        _name = new_name;

        if(_name == "coinbase"){
            _market_fee = 0.0025;
        } else if(_name == "foobase"){
            _market_fee = 0.003;
        } else {
            cout << "Unknown Exchange" << endl;
        }
    }

    string name(){
        return _name;
    }
    float market_fee() {
        return _market_fee;
    }


};

#endif
