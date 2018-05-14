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

//requires all balances be within this amount of each other
constexpr float balance_utilization = 0.65;

struct trade_seq {
    vector<trade_pair> trades;
    vector<double> amounts;
    double net_gain = 1;
    double next_sell_amount = 0;

    vector<string> currencies() const{
        vector<string> _currencies;
        for(const trade_pair& tp : trades){
            _currencies.push_back(tp.sell);
        }
        return _currencies;
    }
    bool add_pair(trade_pair new_trade_pair, double passed_amount = 0){
        
        //first in a sequence, start with balance
        double amount;
        if(passed_amount != 0){
            amount = passed_amount;
        } else {
            amount = next_sell_amount;
        }
        next_sell_amount = amount * new_trade_pair.net;

        if(amount < 0.001){
            cout << "Trade " << new_trade_pair.sell << ">" << new_trade_pair.buy << " (" << new_trade_pair.action << ") "
                 << amount << "@" << new_trade_pair.quote << " has Insufficient starting balance. Invalidating." << endl;
            return false;
        }
        if(new_trade_pair.action == 'b'){//have sell amount, convert for buy amount
            amount = amount / new_trade_pair.quote;
        }
        double total = amount * new_trade_pair.quote;
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
    array<string, 300> _pair_ids;
    array<bool, 300> _funded_pairs;
    const char* _api_keys[3];
    const char* _api_secrets[3];

public:

    crypto_exchange(string new_name, string new_get_url = "", string new_post_url = "", string new_ws_url = ""){
        _name = new_name;

        _curl_get = curl_easy_init();

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
        _pairs.clear();
        if(_name == "poloniex"){
            return populate_poloniex_trade_pairs();
        }else if(_name == "gdax" || _name == "gdax-test"){
            return populate_gdax_trade_pairs();
        }
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
        throw ARB_ERR_UNKNOWN_PAIR;
    }

    int num_trade_pairs() const{
        return _pairs.size();
    }

    int monitor_trades(){
        web::websockets::client::websocket_client client;
        //client.connect("ws://anselmo.me:8181").wait();
        client.connect(_ws_url).wait();

        string command = "{\"command\":\"subscribe\",\"channel\":1002}";

        web::websockets::client::websocket_outgoing_message out_msg;
        out_msg.set_utf8_message(command);
        client.send(out_msg).wait();

        std::chrono::steady_clock::time_point begin, end;
        begin = std::chrono::steady_clock::now();
        bool data_stale = true;
        int count = 0;
        int elapsed;
        float throughput, max_throughput = 0;
        constexpr int print_interval = 100;
        while(true){
            client.receive().then([](web::websockets::client::websocket_incoming_message in_msg) {
                return in_msg.extract_string();
            }).then([&](string body) {
                if(body == "[1002,1]" || body == "[1010]"){//sometimes it just spits this back for a while
                    cout << "Skipping Bad Data..." << endl;
                    data_stale = true;
                } else {
                    if(data_stale == true){//first time back in the loop
                        cout << "Back to good data, doing fresh pull of all trade pairs..." << endl;
                        populate_trade_pairs();
                        data_stale = false;
                    }
                    process_ticker_update(body);
                    trade_seq* profitable_trade = nullptr;

                    if(find_trade(profitable_trade)){
                        execute_trades(profitable_trade);

                        delete profitable_trade;
                        throw ARB_TRADE_EXECUTED;
                    }
                    if(++count == print_interval){
                        count = 0;
                        end = std::chrono::steady_clock::now();
                        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                        throughput = print_interval * 1000.0 / elapsed;
                        if(throughput > max_throughput){
                            max_throughput = throughput;
                        }

                        cout << "100 Ticker Updates Processed in " << elapsed << " milliseconds (" << throughput << "/s, max: " << max_throughput << "/s)." << endl;
                        begin = std::chrono::steady_clock::now();
                    }
                }
            }).wait();

        }
        return 1;

    }


    bool find_trade(trade_seq*& ts){

        float net;
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
                    cout << "Found Potential Trade: " 
                         << tp1.sell << ">" << tp2.sell << "(" << tp1.action << ") > " 
                         << tp2.sell << ">" << tp3.sell << "(" << tp2.action << ") > " 
                         << tp3.sell << ">" << tp3.buy << "(" << tp3.action << ") > " 
                         << tp3.buy << fixed << setprecision(8) << " net:" << net << ", " << endl;

                    ts = new trade_seq;
                    //this requires all balances to be within 75% absolute value of each other
                    if(ts->add_pair(tp1, _balances[tp1.sell] * balance_utilization)){
                        if(ts->add_pair(tp2)){
                            if(ts->add_pair(tp3)){
                                ts->net_gain = net;
                                cout << "Profitable Trade Found: " << endl;
                                ts->print_seq();
                                return true;
                            }
                        }
                    }
                    delete ts;
                }
            }
        }
        return false;
    }

    bool execute_trades(trade_seq*& trade_seq){
        int num_trades = trade_seq->trades.size();

        CURL* handles[num_trades];
        struct curl_slist *header_slist[num_trades];
        CURLM* multi_handle;
        CURLMsg *msg; /* for picking up messages with the transfer status */ 
        int msgs_left; /* how many messages are left */ 

        int curl_still_running;

        long http_code[num_trades];
        std::unique_ptr<std::string> http_data[num_trades];

        for(int i=0; i<num_trades; ++i){
            handles[i] = curl_easy_init();
            http_data[i] = std::unique_ptr<std::string>(new string());
            header_slist[i] = nullptr;
        }

        int trade_num = 0;
        for(const auto& tp : trade_seq->trades){

            if(_name == "poloniex"){
                poloniex_prepare_trade(handles[trade_num], header_slist[trade_num], trade_num, tp.sell, tp.buy, tp.exchange_id, tp.action, tp.quote, trade_seq->amounts[trade_num]);
            } else {
                cout << "trade requested on unsupported exchange. Throwing Error." << endl;
                throw ARB_ERR_BAD_OPTION;
            }
            ++trade_num;
        }

        multi_handle = curl_multi_init();
        for(int i=0; i<num_trades; ++i){
            curl_easy_setopt(handles[i], CURLOPT_WRITEDATA, http_data[i].get());
            curl_multi_add_handle(multi_handle, handles[i]);
        }

        /* we start some action by calling perform right away */ 
        curl_multi_perform(multi_handle, &curl_still_running);

        do {
            struct timeval timeout;
            int rc; /* select() return code */ 
            CURLMcode mc; /* curl_multi_fdset() return code */ 

            fd_set fdread;
            fd_set fdwrite;
            fd_set fdexcep;
            int maxfd = -1;

            long curl_timeo = 20000;

            FD_ZERO(&fdread);
            FD_ZERO(&fdwrite);
            FD_ZERO(&fdexcep);

            /* set a suitable timeout to play around with */ 
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            curl_multi_timeout(multi_handle, &curl_timeo);
            if(curl_timeo >= 0) {
                timeout.tv_sec = curl_timeo / 1000;
                if(timeout.tv_sec > 1)
                    timeout.tv_sec = 1;
                else
                    timeout.tv_usec = (curl_timeo % 1000) * 1000;
            }

            /* get file descriptors from the transfers */ 
            mc = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

            if(mc != CURLM_OK) {
                fprintf(stderr, "curl_multi_fdset() failed, code %d.\n", mc);
                break;
            }

            /* On success the value of maxfd is guaranteed to be >= -1. We call
               select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
               no fds ready yet so we call select(0, ...) --or Sleep() on Windows--
               to sleep 100ms, which is the minimum suggested value in the
               curl_multi_fdset() doc. */ 

            if(maxfd == -1) {
                // Portable sleep for platforms other than Windows.
                struct timeval wait = { 0, 100 * 1000 }; // 100ms
                rc = select(0, NULL, NULL, NULL, &wait);
            } else {
                // Note that on some platforms 'timeout' may be modified by select().
                //   If you need access to the original value save a copy beforehand.
                rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
            }

            switch(rc) {
                case -1:
                    // select error
                    break;
                case 0: // timeout
                default: // action
                    curl_multi_perform(multi_handle, &curl_still_running);
                    break;
            }
        } while(curl_still_running);

        /* See how the transfers went */ 
        while((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
            if(msg->msg == CURLMSG_DONE) {
                int idx, found = 0;

                /* Find out which handle this message is about */ 
                for(idx = 0; idx<num_trades; ++idx) {
                    found = (msg->easy_handle == handles[idx]);
                    if(found)
                        break;
                }

                //printf("CURL Handle %i finished with status %d\n", idx, msg->data.result);
            }
        }

        bool all_trades_executed = true;
        /* Free the CURL handles */ 
        for(int i = 0; i<num_trades; i++){

            curl_easy_getinfo(handles[i], CURLINFO_RESPONSE_CODE, &http_code[i]);
            std::cout << "Receieved " << http_code[i] << " Response: " << *http_data[i].get() << std::endl;
            if (http_code[i] == 200) {

                rapidjson::Document trade_result;
                trade_result.Parse((*http_data[i]).c_str());
                trade_pair* tp = &trade_seq->trades[i];

                double traded_amount = 0;
                double traded_total = 0;
                if(trade_result["resultingTrades"].Empty()){
                    all_trades_executed = false;
                } else {
                    //{"orderNumber":"144492489990","resultingTrades":[{"amount":"179.69999695","date":"2018-04-26 06:40:57","rate":"0.00008996","total":"0.01616581","tradeID":"21662264","type":"sell"}],"amountUnfilled":"0.00000000"}
                    for(const auto& trade : trade_result["resultingTrades"].GetArray()){
                        traded_amount += stod(trade["amount"].GetString());
                        traded_total += stod(trade["total"].GetString());
                    }
                    if(trade_seq->trades[i].action == 'b'){
                        _balances[tp->buy] += traded_amount * (1 - _market_fee);
                        _balances[tp->sell] -= traded_total;
                    }else if(trade_seq->trades[i].action == 's'){
                        _balances[tp->sell] -= traded_amount;
                        _balances[tp->buy] += traded_total * (1 - _market_fee);
                    }
                    //trade was partially filled
                    if((traded_amount - trade_seq->amounts[i]) > 0.0000001){
                        all_trades_executed = false;
                    }
                }
            } else { //non 200 response
                all_trades_executed = false;
            }

            curl_slist_free_all(header_slist[i]);
            curl_easy_cleanup(handles[i]);
        }
        curl_multi_cleanup(multi_handle);

        return all_trades_executed;
    }

    bool any_open_orders(){

        char post_data[120];
        strcpy(post_data, "command=returnOpenOrders&currencyPair=all");

        CURL* curl = curl_easy_init();
        struct curl_slist *header_slist = nullptr;
		set_curl_post_options(curl, header_slist, post_data);
        string http_data = poloniex_single_post(curl);
        curl_slist_free_all(header_slist);
		curl_easy_cleanup(curl);

        cout << http_data << endl;
        rapidjson::Document open_trades;
        open_trades.Parse(http_data.c_str());

        for(const auto& currency_pair : open_trades.GetObject()){
            if(!currency_pair.value.Empty()){
                return true;
                //cout << "Pair: " << currency_pair.name.GetString() << " val: " << currency_pair.value.GetString() << endl;
            }
        }
        return false;
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
        //    cout << "Processing Message:" << message << endl;
        //[1002,null,[126,"238.99999999","239.25328845","239.07169998","-0.00905593","549272.05317976","2348.39754632",0,"241.63999999","228.00000000"]])

        //begin really funky string parsing. It's two lines of code to turn the whole thing into a json document, but this is a bit faster
        int pair_id = 0;
        double ask = 0;
        double bid = 0;
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

        /*
        rapidjson::Document ticker_update;
        ticker_update.Parse(message.c_str());
        int pair_id = ticker_update[2][0].GetInt();
        double ask = stod(ticker_update[2][2].GetString());
        double bid = stod(ticker_update[2][3].GetString());
        */

        if(!_funded_pairs[pair_id]){
            return;
        }

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

    void set_curl_post_options(CURL* curl, curl_slist*& header_slist, char* post_data, int api_num = 0){
        char nonce[14];
        long int now = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        sprintf(nonce, "%li", now);

        strcat(post_data, "&nonce=");
        strcat(post_data, nonce);

        char api_key_header[40] = "Key:";
        strcat(api_key_header, _api_keys[api_num]);

        string signature = hmac_512_sign(_api_secrets[api_num], post_data);

        printf("Sending Post Data: %s\n", post_data);
        curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, post_data);

        header_slist = curl_slist_append(header_slist, api_key_header);
        string sig_header = "Sign:" + signature;
        header_slist = curl_slist_append(header_slist, sig_header.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_slist);

        if(ARB_DEBUG){
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        }

        // Hook up data handling function.
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        // Set remote URL.
        curl_easy_setopt(curl, CURLOPT_URL, _post_url.c_str());

        // Don't bother trying IPv6, which would increase DNS resolution time.
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

        // Don't wait forever, time out after 15 seconds.
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);


        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, "redshift crypto automated trader");
    }

    string poloniex_single_post(CURL* curl) const {

        // Hook up data container (will be passed as the last parameter to the
        // callback handling function).  Can be any pointer type, since it will
        // internally be passed as a void pointer.
        const std::unique_ptr<std::string> http_data(new std::string());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, http_data.get());

        CURLcode result;
        result = curl_easy_perform(curl);

        long http_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

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

    double get_poloniex_taker_fee() {

        char post_data[120];
        strcpy(post_data, "command=returnFeeInfo");
        
        CURL* curl = curl_easy_init();
        struct curl_slist *header_slist = nullptr;
		set_curl_post_options(curl, header_slist, post_data);
        string http_data = poloniex_single_post(curl);
        curl_slist_free_all(header_slist);
		curl_easy_cleanup(curl);

        rapidjson::Document fee_json;
        fee_json.Parse(http_data.c_str());

        return stod(fee_json["takerFee"].GetString());
    }


    void populate_poloniex_balances(){

        char post_data[120];
        strcpy(post_data, "command=returnCompleteBalances");

        CURL* curl = curl_easy_init();
        struct curl_slist *header_slist = nullptr;
		set_curl_post_options(curl, header_slist, post_data);
        string http_data = poloniex_single_post(curl);
        curl_slist_free_all(header_slist);
		curl_easy_cleanup(curl);

        rapidjson::Document balance_json;
        balance_json.Parse(http_data.c_str());

        double btc_val;
        double btc_min = 1000;
        double btc_max = 0;
        //convert to hash for fast lookups. Is this necessary? Should the currency be stored as JSON?
        for(const auto& currency : balance_json.GetObject()){
            btc_val = stof(currency.value["btcValue"].GetString());
            if(btc_val > 0.0001 && btc_val > btc_max){
                btc_max = btc_val;
            }
            if(btc_val > 0.0001 && btc_val < btc_min){
                btc_min = btc_val;
            }
            _balances[currency.name.GetString()] = stod(currency.value["available"].GetString());
        }
        if(btc_max * balance_utilization > btc_min){
            cout << "Error, Min/Max BTC balance values " << btc_min << "/" << btc_max << " >" << balance_utilization << " balance utilization" << endl;
            throw ARB_ERR_INSUFFICIENT_FUNDS;
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

            if(_balances[curr_1] >= 0.001 && _balances[curr_2] >= 0.001){
                _funded_pairs[listed_pair.value["id"].GetInt()] = true;

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
    }

    //returns balance added to destination currency to use in forward chain
    void poloniex_prepare_trade(CURL* curl, curl_slist*& header_slist, int seq_num, const char* sell, const char* buy, int pair_id, char action, double rate, double amount, bool immediate_only = false){
        cout << "EXECUTING TRADE " << seq_num << ": " << sell << ">" << buy << " quote:" << fixed << setprecision(8) << rate << " amount:" << amount << "(" << action << ")" << endl;
        /*
        //round to improve odds of sale executing
        if(action == 'b'){
            rate += 0.00000001;
            //amount passed is amount to sell, convert for buys
        }else if(action == 's'){
            rate -= 0.00000001;
        }
        */

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

        set_curl_post_options(curl, header_slist, post_data, seq_num);
    }

};

#endif
