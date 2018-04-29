#ifndef CRYPTO_EXCHANGE_H
#define CRYPTO_EXCHANGE_H

#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <string>
#include "arb_util.cpp"
#include "trade_pair.cpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace std;

struct trade_seq {
    vector<trade_pair> trades;
    double net_gain = 1;

    vector<string> currencies() const{
        vector<string> _currencies;
        for(const trade_pair& tp : trades){
            _currencies.push_back(tp.sell);
        }
        return _currencies;
    }

    void print_seq() const {
        cout << "Trade Seq: ";
        for(const trade_pair& tp : trades){
            cout << tp.sell << ">" << tp.buy << " " << tp.action << "@" << fixed << setprecision(8) << tp.quote << " net:" << tp.net << ", ";
        }
        cout << "Net Change:" << fixed << setprecision(8) << net_gain << endl;
    }
};

class crypto_exchange {
    string _name;
    string _get_url;
    string _post_url;
    double _market_fee;
    map<string, double> _balances;
    vector<trade_pair> _pairs;
    vector<trade_seq> _seqs;
    CURL* _curl_get;
    CURL* _curl_post;

public:

    crypto_exchange(string new_name){
        _name = new_name;

        _curl_get = curl_easy_init();
        _curl_post = curl_easy_init();

        if(_name == "gdax"){
            _get_url = "https://api.gdax.com/products";
            _market_fee = 0.0025;
        } else if(_name == "gdax-test"){
            _get_url = "http://anselmo.me/gdax/products.php";
            _market_fee = 0.0025;
        } else if(_name == "poloniex"){
            _get_url = "https://poloniex.com/public?command=returnTicker";
            _post_url = "https://poloniex.com/tradingApi";

            set_curl_get_options(_curl_get, _get_url);
            set_curl_post_options(_curl_post);

            _market_fee = get_poloniex_taker_fee();
            populate_balances();
        } else if(_name == "poloniex-test"){
            _get_url = "https://anselmo.me/poloniex/public.php?command=returnTicker";
            _post_url = "http://anselmo.me/poloniex/tradingapi.php";

            set_curl_get_options(_curl_get, _get_url);
            set_curl_post_options(_curl_post);

            _market_fee = get_poloniex_taker_fee();
            populate_balances();
        } else if(_name == "foobase"){
            _market_fee = 0.003;
        } else {
            cout << "Unknown Exchange" << endl;
        }


    }
    ~crypto_exchange(){
        curl_easy_cleanup(_curl_get);
        curl_easy_cleanup(_curl_post);
    }


    string name() const {
        return _name;
    }
    double market_fee() const {
        return _market_fee;
    }
    double balance(const string& currency) const{
        return _balances.at(currency);
    }

    void populate_balances(){
        if(_name == "poloniex" || _name == "poloniex-test"){
            populate_poloniex_balances();
        }
    }

    void print_balances() const{
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

    void print_trade_pairs() const{
        for(const auto& pair : _pairs){
            cout << pair.sell << "-" << pair.buy << " net:" << pair.net << endl;
        }
    }

    int num_trade_pairs() const{
        return _pairs.size();
    }

    int populate_trades(){
        int trades_added = 0;

        double net;
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

                    //optimization: don't copy the pair objects unless it's worth it
                    net = tp1.net * tp2.net * tp3.net;
                    if(net < 1.0010){
                        continue;
                    }

                    trade_seq ts;
                    ts.trades.push_back(tp1);
                    ts.trades.push_back(tp2);
                    ts.trades.push_back(tp3);
                    ts.net_gain = net;
                    if(ARB_DEBUG){
                        ts.print_seq();
                    }
                    _seqs.push_back(ts);
                    trades_added += 1;
                }
            }
        }
    }

    void print_trade_seqs() const{
        for(const auto& ts : _seqs){
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
                    if(_balances.at(seq.trades.front().sell) < 0.000001){
                        cout << "Found profitable trade, but skipping due to no balance:" << endl;
                        seq.print_seq();
                        continue;
                    }
                    most_profitable = &seq;
                }
            }
        }
        
        if(most_profitable != nullptr){
            cout << "Profitable Trade Found: " << endl;
            most_profitable->print_seq();
        }
        return most_profitable;
    }

    void execute_trades(trade_seq* trade_seq){

        cout << "Balances Before:";
        for(const auto& currency : trade_seq->currencies()){
            cout << " " << currency << ":" << _balances[currency];
        }
        cout << endl;

        double amount = _balances[trade_seq->trades.front().sell];
        int trade_num = 0;
        for(const auto& tp : trade_seq->trades){
            cout << "EXECUTING TRADE " << ++trade_num << ": " << tp.sell << ">" << tp.buy << " quote:" << fixed << setprecision(8) << tp.quote << " net:" << tp.net << "(" << tp.action << ")" << endl;
            if(_balances[tp.sell] < 0.000001){
                cout << "Encountered near-zero balance for " << tp.sell << ". Throwing Error." << endl;
                throw ARB_ERR_INSUFFICIENT_FUNDS;
            }

            bool immediate_only = true;
            if(trade_num == 3){
                immediate_only = false;
            }

            if(_name == "poloniex" || _name == "poloniex-test"){
                if(tp.action == "sell"){
                    amount = poloniex_sell(tp.sell, tp.buy, tp.quote, amount, immediate_only);
                }else if(tp.action == "buy"){
                    amount = poloniex_buy(tp.sell, tp.buy, tp.quote, amount, immediate_only);
                } else {
                    cout << "Unknown action of trade pair encountered. Throwing Error." << endl;
                    throw ARB_ERR_BAD_OPTION;
                }
            } else {
                cout << "trade requested on unsupported exchange. Throwing Error." << endl;
                throw ARB_ERR_BAD_OPTION;
            }
        }

        cout << "Balances After:";
        for(const auto& currency : trade_seq->currencies()){
            cout << " " << currency << ":" << _balances[currency];
        }
        cout << endl;
        

    }

private:

    void populate_gdax_trade_pairs(){

        std::string http_data = curl_get(_curl_get);
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
                tp.quote = stod(book_data["bids"][0][0].GetString());
                tp.net = (1.0 - _market_fee) * tp.quote;
                tp.action = "buy";
                _pairs.push_back(tp);

                tp.sell = listed_pair["quote_currency"].GetString();
                tp.buy = listed_pair["base_currency"].GetString();
                tp.quote = stod(book_data["asks"][0][0].GetString());
                tp.net = (1.0 - _market_fee) / tp.quote;
                tp.action = "sell";
                _pairs.push_back(tp);
            }

        }
    }

    void set_curl_post_options(CURL* curl){
        // Set remote URL.
        curl_easy_setopt(_curl_post, CURLOPT_URL, _post_url.c_str());

        // Don't bother trying IPv6, which would increase DNS resolution time.
        curl_easy_setopt(_curl_post, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

        // Don't wait forever, time out after 15 seconds.
        curl_easy_setopt(_curl_post, CURLOPT_TIMEOUT, 15L);


        curl_easy_setopt(_curl_post, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(_curl_post, CURLOPT_SSL_VERIFYHOST, 0L);

        curl_easy_setopt(_curl_post, CURLOPT_USERAGENT, "redshift crypto automated trader");
    }

    std::string poloniex_post(std::string post_data) const {
        long int now = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        post_data = post_data + "&nonce=" + to_string(now);
        string api_secret = getenv("ARB_API_SECRET");
        string api_key_header = getenv("ARB_API_KEY");
        string signature = hmac_512_sign(api_secret, post_data);

        CURLcode result;

        //const char* c_post_data = post_data.c_str();
        cout << "Post Data: " << post_data << endl;
        curl_easy_setopt(_curl_post, CURLOPT_POSTFIELDS, post_data.c_str());
        if(ARB_DEBUG){
            cout << "Sending Post Data: " << post_data << endl;
        }

        struct curl_slist *chunk = NULL;
        chunk = curl_slist_append(chunk, api_key_header.c_str());
        string sig_header = "Sign:" + signature;
        chunk = curl_slist_append(chunk, sig_header.c_str());
        curl_easy_setopt(_curl_post, CURLOPT_HTTPHEADER, chunk);

        if(ARB_DEBUG){
            curl_easy_setopt(_curl_post, CURLOPT_VERBOSE, 1);
        }

        // Hook up data handling function.
        curl_easy_setopt(_curl_post, CURLOPT_WRITEFUNCTION, callback);

        // Hook up data container (will be passed as the last parameter to the
        // callback handling function).  Can be any pointer type, since it will
        // internally be passed as a void pointer.
        const std::unique_ptr<std::string> http_data(new std::string());
        curl_easy_setopt(_curl_post, CURLOPT_WRITEDATA, http_data.get());

        result = curl_easy_perform(_curl_post);

        long http_code;
        curl_easy_getinfo(_curl_post, CURLINFO_RESPONSE_CODE, &http_code);

        if(result != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
            throw 20;
        }
        if (http_code == 200) {
            std::cout << "HTTP Response: " << *http_data.get() << std::endl;
        } else {
            std::cout << "Received " << http_code << " response code from " << _post_url << endl;
            throw 30;
        }

        return *http_data;
    }

    double get_poloniex_taker_fee() const{

        string post_data = "command=returnFeeInfo";
        string http_data = poloniex_post(post_data);

        rapidjson::Document fee_json;
        fee_json.Parse(http_data.c_str());

        return stod(fee_json["takerFee"].GetString());
    }


    void populate_poloniex_balances(){

        string post_data = "command=returnBalances";
        string http_data = poloniex_post(post_data);

        rapidjson::Document balance_json;
        balance_json.Parse(http_data.c_str());

        //convert to hash for fast lookups. Is this necessary? Should the currency be stored as JSON?
        for(const auto& currency : balance_json.GetObject()){
            _balances[currency.name.GetString()] = stod(currency.value.GetString());
        }
    }

    void populate_poloniex_trade_pairs(){

        std::string http_data = curl_get(_curl_get);
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
            tp.quote = stod(listed_pair.value["highestBid"].GetString());
            tp.net = (1 - _market_fee) * tp.quote;
            tp.action = "sell";
            _pairs.push_back(tp);

            tp.sell = pair_names[0];
            tp.buy = pair_names[1];
            tp.quote = stod(listed_pair.value["lowestAsk"].GetString());
            tp.net = (1 - _market_fee) / tp.quote;
            tp.action = "buy";
            _pairs.push_back(tp);
        }
    }

    //returns balance added to destination currency to use in forward chain
    double poloniex_sell(string sell, string buy, double rate, double sell_amount, bool immediate_only){
        rate -= 0.00000001; //round down to ensure sale executes
        string post_data = "command=sell&currencyPair=" + buy + "_" + sell;
        stringstream ss_rate, ss_amount;
        ss_rate << fixed << setprecision(8) << rate;
        post_data += "&rate=" + ss_rate.str();
        ss_amount << fixed << setprecision(8) << sell_amount;
        post_data += "&amount=" + ss_amount.str();
        if(immediate_only){
            post_data += "&immediateOrCancel=1";
        }

        string http_data = poloniex_post(post_data);

        //{"orderNumber":"144484516971","resultingTrades":[],"amountUnfilled":"179.69999695"}
        rapidjson::Document trade_result;
        trade_result.Parse(http_data.c_str());

        double traded_amount = 0;
        if(trade_result["resultingTrades"].Empty()){
            cout << "No Resulting Trades executed. Throwing Error." << endl;
            throw ARB_ERR_TRADE_NOT_EX;
        } else {
            //{"orderNumber":"144492489990","resultingTrades":[{"amount":"179.69999695","date":"2018-04-26 06:40:57","rate":"0.00008996","total":"0.01616581","tradeID":"21662264","type":"sell"}],"amountUnfilled":"0.00000000"}
            double unfilled = stod(trade_result["amountUnfilled"].GetString());
            _balances[sell] -= (sell_amount - unfilled);

            for(const auto& trade : trade_result["resultingTrades"].GetArray()){
                traded_amount += stod(trade["total"].GetString()) * (1 - _market_fee);
            }
            _balances[buy] += traded_amount;
        }
        return traded_amount;

    }

    double poloniex_buy(string sell, string buy, double rate, double sell_amount, bool immediate_only){
        rate += 0.00000001; //round up to ensure sale executes
        double buy_amount = sell_amount / rate;
        string post_data = "command=buy&currencyPair=" + sell + "_" + buy;
        stringstream ss_rate, ss_amount;
        ss_rate << fixed << setprecision(8) << rate;
        post_data += "&rate=" + ss_rate.str();
        ss_amount << fixed << setprecision(8) << buy_amount;
        post_data += "&amount=" + ss_amount.str();
        if(immediate_only){
            post_data += "&immediateOrCancel=1";
        }

        string http_data = poloniex_post(post_data);

        //{"orderNumber":"144484516971","resultingTrades":[],"amountUnfilled":"179.69999695"}
        rapidjson::Document trade_result;
        trade_result.Parse(http_data.c_str());

        double bought_amount = 0;
        double sold_amount = 0;
        if(trade_result["resultingTrades"].Empty()){
            cout << "No Resulting Trades executed. Throwing Error." << endl;
            throw ARB_ERR_TRADE_NOT_EX;
        } else {
            //{"orderNumber":"270275494074","resultingTrades":[{"amount":"0.31281719","date":"2018-04-26 19:52:58","rate":"0.02989499","total":"0.00935166","tradeID":"16843227","type":"buy"}],"amountUnfilled":"0.00000000"}
            //double unfilled = stod(trade_result["amountUnfilled"].GetString());

            for(const auto& trade : trade_result["resultingTrades"].GetArray()){
                bought_amount += stod(trade["amount"].GetString()) * (1 - _market_fee);
                sold_amount += stod(trade["total"].GetString());
            }
            _balances[buy] += bought_amount;
            _balances[sell] -= sold_amount;
        }
        return bought_amount;
    }

};

#endif
