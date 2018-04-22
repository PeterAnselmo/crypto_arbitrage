#ifndef CRYPTO_EXCHANGE_H
#define CRYPTO_EXCHANGE_H

#include <iostream>
#include <string>
#include "lib/t-hmac.c"

using namespace std;

class crypto_exchange {
    string _name;
    float _market_fee;

    public:

    crypto_exchange(string new_name){
        _name = new_name;

        if(_name == "coinbase"){
            _market_fee = 0.0025;
        } else if(_name == "poloniex"){
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


    void get_poloniex_balances(){
        std::string api_secret = "4f3698099cddc69418e0b1918f78b3afc887f4b6baea6f17aab9d453e7eec8e8a30fa52057391371b4f4d5c0e48aea6bfb051d534fe0c38df06fc37ce3d49f54";
        std::string api_key = "QTMALQ04-HWB2QMKQ-8TZ7VG2X-3MA8AD8R";


        const byte headers = "Now is the time for all good men to come to the aide of their country";

        OpenSSL_add_all_algorithms();
        /* Sign and Verify HMAC keys */
        EVP_PKEY *skey = NULL, *vkey = NULL;

        int rc = make_keys(&skey, &vkey);
        assert(rc == 0);
        if(rc != 0)
            exit(1);
        
        assert(skey != NULL);
        if(skey == NULL)
            exit(1);
        
        assert(vkey != NULL);
        if(vkey == NULL)
            exit(1);
        
        byte* sig = NULL;
        size_t slen = 0;
        rc = sign_it(headers, sizeof(headers), &sig, &slen, skey);
        assert(rc == 0);
        if(rc == 0) {
            printf("Created signature\n");
        } else {
            printf("Failed to create signature, return code %d\n", rc);
            exit(1); /* Should cleanup here */
        }
        
        print_it("Signature", sig, slen);

    }


};

#endif
