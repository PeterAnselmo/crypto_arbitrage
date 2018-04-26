#ifndef CRYPTO_EXCHANGE_H
#define CRYPTO_EXCHANGE_H

#include <iostream>
#include <string>
#include "arb_util.cpp"
#include "trade_pair.cpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace std;

struct trade_seq {
    vector<trade_pair> trades;
    float net_gain = 1;

    void add_pair(trade_pair new_trade_pair){
        trades.push_back(new_trade_pair);
        net_gain = net_gain * new_trade_pair.price;
    }

    vector<string> currencies(){
        vector<string> _currencies;
        for(const trade_pair& tp : trades){
            _currencies.push_back(tp.sell);
        }
        return _currencies;
    }

    void print_seq(){
        cout << "Trade Seq: ";
        for(const trade_pair& tp : trades){
            cout << tp.sell << ">" << tp.buy << "@" << tp.price << ", ";
        }
        cout << "Net Change:" << net_gain << endl;
    }
};

class crypto_exchange {
    string _name;
    string _get_url;
    string _post_url;
    float _market_fee;
    map<string, float> _balances;
    vector<trade_pair> _pairs;
    vector<trade_seq> _seqs;

public:

    crypto_exchange(string new_name){
        _name = new_name;

        if(_name == "gdax"){
            _get_url = "https://api.gdax.com/products";
            _market_fee = 0.0025;
        } else if(_name == "gdax-test"){
            _get_url = "http://anselmo.me/gdax/products.php";
            _market_fee = 0.0025;
        } else if(_name == "poloniex"){
            _get_url = "https://poloniex.com/public";
            _post_url = "https://poloniex.com/tradingApi";
            _market_fee = get_poloniex_taker_fee();
            populate_balances();
        } else if(_name == "poloniex-test"){
            _get_url = "https://anselmo.me/poloniex/public.php";
            _post_url = "http://anselmo.me/poloniex/tradingapi.php";
            _market_fee = get_poloniex_taker_fee();
            populate_balances();
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

    void populate_trade_pairs(){
        if(_name == "poloniex" || _name == "poloniex-test"){
            return populate_poloniex_trade_pairs();
        }else if(_name == "gdax" || _name == "gdax-test"){
            return populate_gdax_trade_pairs();
        }
    }

    void clear_trades_and_pairs(){
        _seqs.clear();
        _pairs.clear();

    }

    void print_trade_pairs(){
        for(const auto& pair : _pairs){
            cout << pair.sell << "-" << pair.buy << " price:" << pair.price << endl;
        }
    }

    int num_trade_pairs(){
        return _pairs.size();
    }

    int populate_trades(){
        int trades_added = 0;

        //Naieve O(n^3) terrible graph traversal
        for(const auto& tp1 : _pairs){
            for(const auto& tp2 : _pairs) {
                if(tp1.buy != tp2.sell){
                    continue;
                }
                for(const auto& tp3 : _pairs){
                    if(tp2.buy != tp3.sell){
                        continue;
                    }
                    //make sure it's a triangle
                    if(tp3.buy != tp1.sell) {
                        continue;
                    }
                    trade_seq ts;
                    ts.add_pair(tp1);
                    ts.add_pair(tp2);
                    ts.add_pair(tp3);
                    if(ARB_DEBUG){
                        ts.print_seq();
                    }
                    if(ts.net_gain > 1.0){
                        _seqs.push_back(ts);
                        trades_added += 1;
                    }
                }
            }
        }
    }

    void print_trade_seqs(){
        for(auto& ts : _seqs){
            ts.print_seq();
        }
    }

    trade_seq* compare_trades() {

        trade_seq *most_profitable = nullptr;
        int count = 0;
        
        vector<trade_seq>::iterator it;
        for(auto& seq : _seqs){
            if(seq.net_gain > 1.0){
                if(most_profitable == nullptr || seq.net_gain > most_profitable->net_gain){
                    most_profitable = &seq;
                }
            }
        }
        
        if(ARB_DEBUG){
            if(most_profitable != nullptr){
                cout << "Profitable Trade Found: " << endl;
                most_profitable->print_seq();
            }
        }
        return most_profitable;
    }

    void execute_trades(trade_seq* trade_seq){

        cout << "Balances Before:";
        for(const auto& currency : trade_seq->currencies()){
            cout << " " << currency << ":" << _balances[currency];
        }
        cout << endl;

        for(const auto& tp : trade_seq->trades){
            cout << "EXECUTING TRADE: " << tp.sell << ">" << tp.buy << "@" << tp.price;
            if(tp.inverted){
                cout << " (inverted)";
            }
            cout << endl;
            /*
            if( !trade.inverted){

                amount = _balanc
                buy(
                */


        }

        populate_balances();
        cout << "Balances After:";
        for(const auto& currency : trade_seq->currencies()){
            cout << " " << currency << ":" << _balances[currency];
        }
        cout << endl;
        

    }

private:

    void populate_gdax_trade_pairs(){

        std::string http_data = curl_get(_get_url);
        rapidjson::Document products;
        products.Parse(http_data.c_str());

        vector<trade_pair> pairs;
        trade_pair tp;
        for(auto& listed_pair : products.GetArray()){
            if(ARB_DEBUG){
                cout << "Fetcing Pair: " << listed_pair["id"].GetString() << "..." << endl;
            }

            char url[127];
            strcpy(url, "https://anselmo.me/gdax/products/");
            strcat(url, listed_pair["id"].GetString());
            strcat(url, "/book.php");

            if(ARB_DEBUG){
                cout << "Product Book URL: " << url << endl;
            }


            try {
                http_data = curl_get(url);
            } catch (int exception){
                continue;
            }
            rapidjson::Document book_data;
            book_data.Parse(http_data.c_str());
            if(!book_data["bids"].Empty() && !book_data["asks"].Empty()){
                /*
                cout << "Importing Pair: " << listed_pair["id"].GetString() 
                     << " bid: " << book_data["bids"][0][0].GetString()
                     << " ask: " << book_data["asks"][0][0].GetString()<< endl;
                     */
                tp.sell = listed_pair["base_currency"].GetString();
                tp.buy = listed_pair["quote_currency"].GetString();
                tp.price = (1.0 - _market_fee) * stof(book_data["bids"][0][0].GetString());
                tp.inverted = false;
                _pairs.push_back(tp);

                tp.sell = listed_pair["quote_currency"].GetString();
                tp.buy = listed_pair["base_currency"].GetString();
                tp.price = (1.0 - _market_fee) / stof(book_data["asks"][0][0].GetString());
                tp.inverted = true;
                _pairs.push_back(tp);
            }

        }
    }

    std::string poloniex_post(std::string post_data) {

        post_data = post_data + "&nonce=" + to_string(time(0));
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

    float get_poloniex_taker_fee(){

        string post_data = "command=returnFeeInfo";
        string http_data = poloniex_post(post_data);

        rapidjson::Document fee_json;
        fee_json.Parse(http_data.c_str());

        return stof(fee_json["takerFee"].GetString());
    }


    void populate_poloniex_balances(){

        string post_data = "command=returnBalances";
        string http_data = poloniex_post(post_data);

        rapidjson::Document balance_json;
        balance_json.Parse(http_data.c_str());

        //convert to hash for fast lookups. Is this necessary? Should the currency be stored as JSON?
        for(auto& currency : balance_json.GetObject()){
            _balances[currency.name.GetString()] = stof(currency.value.GetString());
        }
    }

    void populate_poloniex_trade_pairs(){

        std::string http_data = curl_get(_get_url + "?command=returnTicker");
        rapidjson::Document products;
        products.Parse(http_data.c_str());

        trade_pair tp;
        for(const auto& listed_pair : products.GetObject()){
            if(ARB_DEBUG){
                cout << "Import Pair: " << listed_pair.name.GetString()
                     << " bid: " << listed_pair.value["highestBid"].GetString()
                     << " ask: " << listed_pair.value["lowestAsk"].GetString() << endl;
            }

            std::vector<std::string> pair_names = split(listed_pair.name.GetString(), '_');

            tp.sell = pair_names[1];
            tp.buy = pair_names[0];
            tp.price = (1 - _market_fee) * stof(listed_pair.value["highestBid"].GetString());
            tp.inverted = false;
            _pairs.push_back(tp);

            tp.sell = pair_names[0];
            tp.buy = pair_names[1];
            tp.price = (1 - _market_fee) / stof(listed_pair.value["lowestAsk"].GetString());
            tp.inverted = true;
            _pairs.push_back(tp);
        }
    }

};

#endif
