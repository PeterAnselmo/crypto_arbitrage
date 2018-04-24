#ifndef CRYPTO_EXCHANGE_H
#define CRYPTO_EXCHANGE_H

#include <iostream>
#include <string>
#include "arb_util.cpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
class crypto_exchange {
    string _name;
    string _post_url;
    float _market_fee;
    map<string, float> _balances;

public:

    crypto_exchange(string new_name){
        _name = new_name;

        if(_name == "coinbase"){
            _market_fee = 0.0025;
        } else if(_name == "poloniex"){
            _market_fee = 0.0025;
            _post_url = "https://poloniex.com/tradingApi";
        } else if(_name == "poloniex-test"){
            _market_fee = 0.0025;
            _post_url = "https://anselmo.me/poloniex/tradingapi.php";
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
    double balance(string currency){
        return _balances[currency];
    }

    void populate_balances(){
        if(_name == "poloniex" || _name == "poloniex-test"){
            populate_poloniex_balances();
        }
    }

    void print_balances(){
        for(const auto& currency : _balances){
            cout << currency.first << ": " << currency.second << endl;
        }
    }


private:

    std::string poloniex_post(const std::string post_data) {

        string api_secret = getenv("ARB_API_SECRET");
        string api_key_header = getenv("ARB_API_KEY");
        string signature = hmac_512_sign(api_secret, post_data);

        CURLcode result;
        CURL* curl = curl_easy_init();

        // Set remote URL.
        curl_easy_setopt(curl, CURLOPT_URL, _post_url.c_str());

        //const char* c_post_data = post_data.c_str();
        //cout << "Post Data: " << c_post_data << endl;
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());

        struct curl_slist *chunk = NULL;
        chunk = curl_slist_append(chunk, api_key_header.c_str());
        string sig_header = "Sign:" + signature;
        chunk = curl_slist_append(chunk, sig_header.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        if(ARB_DEBUG){
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        }

        // Don't bother trying IPv6, which would increase DNS resolution time.
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

        // Don't wait forever, time out after 10 seconds.
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);


        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, "redshift crypto automated trader");

        // Hook up data handling function.
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

        // Hook up data container (will be passed as the last parameter to the
        // callback handling function).  Can be any pointer type, since it will
        // internally be passed as a void pointer.
        const std::unique_ptr<std::string> http_data(new std::string());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, http_data.get());

        result = curl_easy_perform(curl);

        long http_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_easy_cleanup(curl);

        if(result != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
            throw 20;
        }
        if (http_code == 200) {
            if(ARB_DEBUG){
                std::cout << "\nGot successful response from " << _post_url << std::endl;
                std::cout << "HTTP data was:\n" << *http_data.get() << std::endl;
            }
        } else {
            std::cout << "Received " << http_code << " response code from " << _post_url << endl;
            throw 30;
        }

        return *http_data;
    }

    void populate_poloniex_balances(){

        string post_data = "command=returnBalances&nonce=" + to_string(time(0));

        string http_data = poloniex_post(post_data);

        rapidjson::Document balance_json;
        balance_json.Parse(http_data.c_str());

        //convert to hash for fast lookups. Is this necessary? Should the currency be stored as JSON?
        for(auto& currency : balance_json.GetObject()){
            _balances[currency.name.GetString()] = stof(currency.value.GetString());
        }
    }

};

#endif
