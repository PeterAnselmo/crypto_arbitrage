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
#include <cpprest/ws_client.h>

using namespace std;
using namespace web::websockets::client;

struct trade_seq {
    vector<trade_pair> trades;
    vector<float> amounts;
    double net_gain = 1;

    vector<string> currencies() const{
        vector<string> _currencies;
        for(const trade_pair& tp : trades){
            _currencies.push_back(tp.sell);
        }
        return _currencies;
    }
    bool add_pair(trade_pair new_trade_pair, float amount){
        
        if(amount < 0.001){
            cout << "Trade " << new_trade_pair.sell << ">" << new_trade_pair.buy << " (" << new_trade_pair.action << ") "
                 << amount << "@" << new_trade_pair.quote << " has Insufficient starting balance. Invalidating." << endl;
            return false;
        }
        if(new_trade_pair.action == 'b'){//have sell amount, convert for buy amount
            amount = amount / new_trade_pair.quote;
        }
        float total = amount * new_trade_pair.quote;
        if(total < 0.0001){
            cout << "Trade " << new_trade_pair.sell << ">" << new_trade_pair.buy << " (" << new_trade_pair.action << ") "
                 << amount << "@" << new_trade_pair.quote << " would have total < 0.0001. Invalidating." << endl;
            return false;
        }

        trades.push_back(new_trade_pair);
        amounts.push_back(amount);
        return true;
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
    string _ws_url;
    double _market_fee;
    map<string, double> _balances;
    vector<trade_pair> _pairs;
    CURL* _curl_get;
    CURL* _curl_post;
    array<string, 300> _pair_ids;
    const char* _api_keys[3];
    const char* _api_secrets[3];

public:

    crypto_exchange(string new_name, string new_get_url = "", string new_post_url = "", string new_ws_url = ""){
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

            _get_url = (new_get_url != "") ? new_get_url : "https://poloniex.com/public?command=returnTicker";
            _post_url = (new_post_url != "") ? new_post_url : "https://poloniex.com/tradingApi";
            _ws_url = (new_ws_url != "") ? new_ws_url : "wss://api2.poloniex.com";

            set_curl_get_options(_curl_get, _get_url);
            set_curl_post_options(_curl_post);

            _api_keys[0] = getenv("ARB_API_KEY_0");
            _api_secrets[0] = getenv("ARB_API_SECRET_0");
            _api_keys[1] = getenv("ARB_API_KEY_1");
            _api_secrets[1] = getenv("ARB_API_SECRET_1");
            _api_keys[2] = getenv("ARB_API_KEY_2");
            _api_secrets[2] = getenv("ARB_API_SECRET_2");


            _market_fee = get_poloniex_taker_fee();
            populate_balances();
        } else {
            cout << "Unknown Exchange" << endl;
        }

        //post connections can't be left open during all the ticker polling
        curl_easy_cleanup(_curl_post);

    }
    ~crypto_exchange(){
        curl_easy_cleanup(_curl_get);
    }


    string name() const {
        return _name;
    }
    double market_fee() const {
        return _market_fee;
    }

    string pair_string(int id) const{
        return _pair_ids[id];
    }

    void print_ids() const{
        int size = _pair_ids.size();
        for(int i=0; i<size; ++i){
            cout << "ID:" << i << _pair_ids[i] << endl;
        }
    }

    double balance(const string& currency) const{
        return _balances.at(currency);
    }

    void populate_balances(){
        if(_name == "poloniex"){
            populate_poloniex_balances();
        }
    }

    void print_balances() const{
        for(const auto& currency : _balances){
            cout << currency.first << ": " << currency.second << endl;
        }
    }

    void populate_trade_pairs(){
        if(_name == "poloniex"){
            return populate_poloniex_trade_pairs();
        }else if(_name == "gdax" || _name == "gdax-test"){
            return populate_gdax_trade_pairs();
        }
    }

    void clear_trades_and_pairs(){
        _pairs.clear();

    }

    void print_trade_pairs() const{
        for(const auto& pair : _pairs){
            cout << pair.sell << "-" << pair.buy << " net:" << pair.net << endl;
        }
    }

    trade_pair get_pair(int id, char action){
        for(auto& tp : _pairs){
            if(id == tp.exchange_id && action == tp.action){
                return tp;
            }
        }
    }

    int num_trade_pairs() const{
        return _pairs.size();
    }

    int monitor_trades(){
        websocket_client client;
        //client.connect("ws://anselmo.me:8181").wait();
        client.connect(_ws_url).wait();

        string command = "{\"command\":\"subscribe\",\"channel\":1002}";

        websocket_outgoing_message out_msg;
        out_msg.set_utf8_message(command);
        client.send(out_msg).wait();

        bool data_stale = true;
        while(true){
            client.receive().then([](websocket_incoming_message in_msg) {
                return in_msg.extract_string();
            }).then([&](string body) {
                cout << body << endl;
                if(body == "[1002,1]" || body == "[1010]"){//sometimes it just spits this back for a while
                    cout << "Skipping Bad Data..." << endl;
                    data_stale = true;
                } else {
                    if(data_stale == true){//first time back in the loop
                        populate_trade_pairs();
                        data_stale = false;
                    }
                    process_ticker_update(body);
                    trade_seq* profitable_trade = find_trade();

                    if(profitable_trade != nullptr){
                        execute_trades(profitable_trade);

                        throw 255;
                    }
                }
            }).wait();

        }
        return 1;

    }


    trade_seq* find_trade(){
        int trades_added = 0;

        trade_seq* ts = nullptr;
        double net;
        for(const auto& tp1 : _pairs){
            for(const auto& tp2 : _pairs) {
                if(strcmp(tp1.buy, tp2.sell) != 0){
                    continue;
                }
                for(const auto& tp3 : _pairs){
                    if(strcmp(tp2.buy, tp3.sell) != 0){
                        continue;
                    }
                    //make sure it's a triangle
                    if(strcmp(tp3.buy, tp1.sell) != 0){
                        continue;
                    }

                    net = tp1.net * tp2.net * tp3.net;
                    if(net < 1.0020){
                        continue;
                    }
                    cout << "Found Potential Trade: " << tp1.sell << ">" << tp2.sell << ">" << tp3.sell << ">" << tp3.buy << fixed << setprecision(8) << " net:" << net << ", " << endl;

                    ts = new trade_seq;
                    if(ts->add_pair(tp1, _balances[tp1.sell])){
                        if(ts->add_pair(tp2, _balances[tp2.sell])){
                            if(ts->add_pair(tp3, _balances[tp3.sell])){
                                ts->net_gain = net;
                                cout << "Profitable Trade Found: " << endl;
                                ts->print_seq();
                                return ts;
                            }
                        }
                    }
                    delete ts;
                }
            }
        }
        return nullptr;
    }

    void execute_trades(trade_seq* trade_seq){

        _curl_post = curl_easy_init();
        set_curl_post_options(_curl_post);

        int num_trades = trade_seq->trades.size();
        int trade_num = 0;
        for(const auto& tp : trade_seq->trades){

            if(_name == "poloniex"){
                poloniex_trade(trade_num, tp.sell, tp.buy, tp.exchange_id, tp.action, tp.quote, trade_seq->amounts[trade_num]);
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

        curl_easy_cleanup(_curl_post);
        ++trade_num;

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
                strcpy(tp.sell, listed_pair["base_currency"].GetString());
                strcpy(tp.buy, listed_pair["quote_currency"].GetString());
                tp.quote = stod(book_data["bids"][0][0].GetString());
                tp.net = (1.0 - _market_fee) * tp.quote;
                tp.action = 'b';
                _pairs.push_back(tp);

                strcpy(tp.sell, listed_pair["quote_currency"].GetString());
                strcpy(tp.buy, listed_pair["base_currency"].GetString());
                tp.quote = stod(book_data["asks"][0][0].GetString());
                tp.net = (1.0 - _market_fee) / tp.quote;
                tp.action = 's';
                _pairs.push_back(tp);
            }

        }
    }

    void process_ticker_update(string message){
        cout << "Processing Message:" << message << endl;
        //[1002,null,[126,"238.99999999","239.25328845","239.07169998","-0.00905593","549272.05317976","2348.39754632",0,"241.63999999","228.00000000"]])

        /*
        //begin really funky string parsing. It's two lines of code to turn the whole thing into a json document, but this is a bit faster
        int pair_id;
        double ask;
        double bid;
        //check we're working on expected array before doing brittle operations
        if(!(message[0] == '[' && message[11] == '[')){
            throw ARB_ERR_UNEXPECTED_STR;
        }
        int start=12; //location of trade pair id
        int end = message.size(); //
        int quote_num = 0;
        for(int i=12; i<end; ++i){
            if(quote_num == 0 && message[i] == ','){
                //cout << "Parsed ID:" << message.substr(start, i-start) << endl;
                pair_id = stoi(message.substr(start, i-start));
            }
            if(message[i] == '"'){
                ++quote_num;
                if(quote_num == 3){ //lowestAsk
                    start = i+1;
                }else if(quote_num == 4){//end of lowestAsk
                 //   cout << "Parsed ask:" << message.substr(start, i-start) << endl;
                    ask = stod(message.substr(start, i-start));
                } else if(quote_num == 5){ //highestBid
                    start = i+1;
                } else if(quote_num == 6){//end of highestBid
                  //  cout << "Parsed bid:" << message.substr(start, i-start) << endl;
                    bid = stod(message.substr(start, i-start));
                    break;
                }
            }
        }
        */
        rapidjson::Document ticker_update;
        ticker_update.Parse(message.c_str());
        int pair_id = ticker_update[2][0].GetInt();
        double ask = stod(ticker_update[2][2].GetString());
        double bid = stod(ticker_update[2][3].GetString());

        int trades_seen = 0;
        for(vector<trade_pair>::iterator it = _pairs.begin(); it != _pairs.end(); ++it){
            if(it->exchange_id != pair_id){
                continue;
            }
            if(it->action == 's'){
                it->quote = bid;
                it->net = (1 - _market_fee) * bid;
            } else if(it->action == 'b'){
                it->quote = ask;
                it->net = (1 - _market_fee) / ask;
            }
            //should only be a single buy & sell for each market
            if(++trades_seen == 2){
                return;
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

    std::string poloniex_post(char* post_data, int seq_num = 0) const {

        char nonce[14];
        long int now = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        sprintf(nonce, "%li", now);

        strcat(post_data, "&nonce=");
        strcat(post_data, nonce);

        char api_key_header[40] = "Key:";
        strcat(api_key_header, _api_keys[seq_num]);

        string signature = hmac_512_sign(_api_secrets[seq_num], post_data);

        CURLcode result;

        printf("Sending Post Data: %s\n", post_data);
        curl_easy_setopt(_curl_post, CURLOPT_POSTFIELDS, post_data);

        struct curl_slist *chunk = NULL;
        chunk = curl_slist_append(chunk, api_key_header);
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
            std::cout << "HTTP Response: " << *http_data.get() << std::endl;
            throw 30;
        }

        return *http_data;
    }

    double get_poloniex_taker_fee() const{

        char post_data[120];
        strcpy(post_data, "command=returnFeeInfo");
        string http_data = poloniex_post(post_data);

        rapidjson::Document fee_json;
        fee_json.Parse(http_data.c_str());

        return stod(fee_json["takerFee"].GetString());
    }


    void populate_poloniex_balances(){

        char post_data[120];
        strcpy(post_data, "command=returnBalances");
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
        char curr_1[8];
        char curr_2[8];
        const char* market;
        const char* it;
        const char* sep_it;
        for(const auto& listed_pair : products.GetObject()){
            if(ARB_DEBUG){
                cout << "Import Pair: " << listed_pair.name.GetString()
                     << " bid: " << listed_pair.value["highestBid"].GetString()
                     << " ask: " << listed_pair.value["lowestAsk"].GetString() << endl;
            }

	    _pair_ids[listed_pair.value["id"].GetInt()] = listed_pair.name.GetString();

            //brittle, dangerous, fast way to split on known-delimiter market string
            market = listed_pair.name.GetString();
            it = sep_it = market;
            while(it && *it){
                if(*it == '_'){
                    memcpy(curr_1, market, it-market);
                    curr_1[it-market] = '\0';
                    sep_it = it;
                }
                ++it;
            }
            memcpy(curr_2, sep_it+1, it - sep_it);

            strcpy(tp.sell, curr_2);
            strcpy(tp.buy, curr_1);
            tp.exchange_id = listed_pair.value["id"].GetInt();
            tp.quote = stod(listed_pair.value["highestBid"].GetString());
            tp.net = (1 - _market_fee) * tp.quote;
            tp.action = 's';
            _pairs.push_back(tp);

            strcpy(tp.sell, curr_1);
            strcpy(tp.buy, curr_2);
            tp.exchange_id = listed_pair.value["id"].GetInt();
            tp.quote = stod(listed_pair.value["lowestAsk"].GetString());
            tp.net = (1 - _market_fee) / tp.quote;
            tp.action = 'b';
            _pairs.push_back(tp);

        }
    }

    //returns balance added to destination currency to use in forward chain
    void poloniex_trade(int seq_num, const char* sell, const char* buy, int pair_id, char action, double rate, double amount, bool immediate_only = false){
        cout << "EXECUTING TRADE " << seq_num << ": " << sell << ">" << buy << " quote:" << fixed << setprecision(8) << rate << " amount:" << amount << "(" << action << ")" << endl;
        //round to improve odds of sale executing
        if(action == 'b'){
            rate += 0.00000001;
            //amount passed is amount to sell, convert for buys
        }else if(action == 's'){
            rate -= 0.00000001;
        }


        char rate_s[16];
        sprintf(rate_s, "%.8f", rate);
        char amount_s[16];
        sprintf(amount_s, "%.8f", amount);
        char post_data[120];

        strcpy(post_data, "command=");
        if(action == 'b'){
            strcat(post_data, "buy");
        }else if(action == 's'){
            strcat(post_data, "sell");
        }
        strcat(post_data, "&currencyPair=");
        strcat(post_data, _pair_ids[pair_id].c_str());
        strcat(post_data, "&rate=");
        strcat(post_data, rate_s);
        strcat(post_data, "&amount=");
        strcat(post_data, amount_s);
        if(immediate_only){
            strcat(post_data, "&immediateOrCancel=1");
        }

        string http_data = poloniex_post(post_data, seq_num);

        //{"orderNumber":"144484516971","resultingTrades":[],"amountUnfilled":"179.69999695"}
        rapidjson::Document trade_result;
        trade_result.Parse(http_data.c_str());

        double traded_amount = 0;
        double traded_total = 0;
        if(trade_result["resultingTrades"].Empty()){
            cout << "No Resulting Trades executed. Throwing Error." << endl;
            throw ARB_ERR_TRADE_NOT_EX;
        } else {
            //{"orderNumber":"144492489990","resultingTrades":[{"amount":"179.69999695","date":"2018-04-26 06:40:57","rate":"0.00008996","total":"0.01616581","tradeID":"21662264","type":"sell"}],"amountUnfilled":"0.00000000"}
            for(const auto& trade : trade_result["resultingTrades"].GetArray()){
                traded_amount += stod(trade["amount"].GetString());
                traded_total += stod(trade["total"].GetString());
            }
            if(action == 'b'){
                _balances[buy] += traded_amount * (1 - _market_fee);
                _balances[sell] -= traded_total;
            }else if(action == 's'){
                _balances[sell] -= traded_amount;
                _balances[buy] += traded_total * (1 - _market_fee);
            }
        }
    }

};

#endif
