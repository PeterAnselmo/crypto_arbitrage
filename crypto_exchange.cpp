#ifndef CRYPTO_EXCHANGE_H
#define CRYPTO_EXCHANGE_H

#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <string>
#include <vector>
#include "arb_util.cpp"
#include "trade_pair.cpp"
#include "order_book.cpp"
#include "trade_seq.cpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <cpprest/ws_client.h>

using namespace std;

#define CHANNEL_TICKER 1002
#define CHANNEL_HEARTBEAT 1010

//requires all balances be within this amount of each other so that
//we can run the trade in any order and have sufficient funds
constexpr int max_trade_pairs = 300;
constexpr float balance_utilization = 0.70;
constexpr int print_interval = 1000;
constexpr int num_trades = 3;


class crypto_exchange {
    string _name;
    string _get_url;
    string _post_url;
    string _ws_url;
    double _market_fee;
    map<string, double> _balances;
    vector<trade_pair> _pairs;
    CURL* _curl_get;
    array<string, max_trade_pairs> _pair_names;
    array<bool, max_trade_pairs> _funded_pairs;
    array<order_book*, max_trade_pairs> _order_books;
    array<unsigned long, max_trade_pairs> _order_sequences;
    const char* _api_keys[3];
    const char* _api_secrets[3];
    std::chrono::steady_clock::time_point _segment_begin, _segment_end;

    CURL* handles[num_trades];
    std::unique_ptr<std::string> http_data[num_trades];
    struct curl_slist *header_slist[num_trades];
    long http_code[num_trades];
    CURLM* multi_handle;

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

            _get_url = (new_get_url != "") ? new_get_url : "https://poloniex.com/public";
            _post_url = (new_post_url != "") ? new_post_url : "https://poloniex.com/tradingApi";
            _ws_url = (new_ws_url != "") ? new_ws_url : "wss://api2.poloniex.com";


            _api_keys[0] = getenv("ARB_API_KEY_0");
            _api_secrets[0] = getenv("ARB_API_SECRET_0");
            _api_keys[1] = getenv("ARB_API_KEY_1");
            _api_secrets[1] = getenv("ARB_API_SECRET_1");
            _api_keys[2] = getenv("ARB_API_KEY_2");
            _api_secrets[2] = getenv("ARB_API_SECRET_2");

            _market_fee = get_poloniex_taker_fee();
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
        return _pair_names[id];
    }

    void print_ids() const{
        int size = _pair_names.size();
        for(int i=0; i<size; ++i){
            cout << "ID:" << i << _pair_names[i] << endl;
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

    trade_pair get_pair(unsigned int id, char action){
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

        //terible locality for this curl code (It's more relevant to execute_trades(),
        //but it should be executed as soon as possible before processing tickers
        for(int i=0; i<num_trades; ++i){
            handles[i] = curl_easy_init();
            http_data[i] = std::unique_ptr<std::string>(new string());
            header_slist[i] = nullptr;
        }
        multi_handle = curl_multi_init();
        for(int i=0; i<num_trades; ++i){
            curl_easy_setopt(handles[i], CURLOPT_WRITEDATA, http_data[i].get());
            curl_multi_add_handle(multi_handle, handles[i]);
        }

        web::websockets::client::websocket_client client;
        client.connect(_ws_url).wait();

        web::websockets::client::websocket_outgoing_message out_msg;
        string command;

/*
        command = "{\"command\":\"subscribe\",\"channel\":1002}";
        out_msg.set_utf8_message(command);
        client.send(out_msg).wait();
*/

        bool any_funded = false;
        for(int pair_id=0; pair_id<max_trade_pairs; ++pair_id){
            if(_funded_pairs[pair_id]){
                any_funded = true;
                command = "{\"command\":\"subscribe\",\"channel\":\"" + _pair_names[pair_id] + "\"}";
                out_msg.set_utf8_message(command);
                client.send(out_msg).wait();
            }
        }
        if(!any_funded){
            cout << "ERROR, Monitor trades called with no funded pairs. Exiting." << endl;
            throw ARB_ERR_UNKNOWN_PAIR;
        }

        std::chrono::steady_clock::time_point begin, end;
        begin = std::chrono::steady_clock::now();
        bool data_stale = true;
        int count = 0;
        int elapsed;
        float throughput, max_throughput = 0;
        long int last_pair_id;
        while(true){
            client.receive().then([](web::websockets::client::websocket_incoming_message in_msg) {
                return in_msg.extract_string();
            }).then([&](string body) {
                print_ts();
                cout << "Message:" << body << endl;
                if(body == "[1010]"){
                    //heartbeat
                }else if(body == "[1002,1]"){//sometimes it just spits this back for a while
                    //cout << "Skipping Bad Data..." << endl;
                    data_stale = true;
                } else {
                    if(data_stale == true){//first time back in the loop
                        cout << "Back to good data, doing fresh pull of all trade pairs..." << endl;
                        populate_trade_pairs();
                        data_stale = false;
                    }
                    _segment_begin = std::chrono::steady_clock::now();
                    last_pair_id = process_ticker_update(body);
                    trade_seq* profitable_trade = nullptr;

                    if(find_trade(last_pair_id, profitable_trade)){
                        try{
                            execute_trades(profitable_trade);
                        } catch (int exception) {
                            print_market_data(profitable_trade);
                            throw exception;
                        }
                        print_market_data(profitable_trade);

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

                        cout << print_interval << " Ticker Updates Processed in " << elapsed << " milliseconds (" << throughput << "/s, max: " << max_throughput << "/s)." << endl;
                        begin = std::chrono::steady_clock::now();
                    }
                }
            }).wait();

        }
        return 1;

    }


    bool find_trade(unsigned int starting_pair_id, trade_seq*& ts){

        float net;
        for(const auto& tp1 : _pairs){
            if(tp1.exchange_id != starting_pair_id){
                continue;
            }
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
                    ts = new trade_seq;
                    //this requires all balances to be within 75% absolute value of each other
                    //do a round robin to ensure that we start with the lowest depth order
                    if(ts->check_pair(tp1, _balances[tp1.sell] * balance_utilization) && ts->check_pair(tp2) && ts->check_pair(tp3)) {
                        ts->trades.push_back(tp1);
                        ts->trades.push_back(tp2);
                        ts->trades.push_back(tp3);
                        return true;
                    } else if (ts->check_pair(tp2, _balances[tp2.sell] * balance_utilization) && ts->check_pair(tp3) && ts->check_pair(tp1)) {
                        ts->trades.push_back(tp2);
                        ts->trades.push_back(tp3);
                        ts->trades.push_back(tp1);
                        return true;
                    } else if (ts->check_pair(tp3, _balances[tp3.sell] * balance_utilization) && ts->check_pair(tp1) && ts->check_pair(tp2)) {
                        ts->trades.push_back(tp3);
                        ts->trades.push_back(tp1);
                        ts->trades.push_back(tp2);
                        return true;
                    } else {
                        cout << "Found Potential, but unworkable trade: " << endl;
                        cout << tp1.sell << ">" << tp1.buy << " " << tp1.action << "@" << fixed << setprecision(8) << tp1.quote << " net:" << tp1.net << ", "
                             << tp2.sell << ">" << tp2.buy << " " << tp2.action << "@" << fixed << setprecision(8) << tp2.quote << " net:" << tp2.net << ", "
                             << tp3.sell << ">" << tp3.buy << " " << tp3.action << "@" << fixed << setprecision(8) << tp3.quote << " net:" << tp3.net << ", ";
                        delete ts;
                    }
                }
            }
        }
        return false;
    }

    inline void set_curl_post_options(CURL* curl, curl_slist*& header_slist, char* post_data, int api_num = 0){
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

        // Don't wait forever, time out after 20 seconds.
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);


        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, "redshift crypto automated trader");
    }


    bool execute_trades(trade_seq*& trade_seq){
        //int num_trades = trade_seq->trades.size();

        CURLMsg *msg; /* for picking up messages with the transfer status */ 
        int msgs_left; /* how many messages are left */ 

        int curl_still_running;

        int trade_num = 0;
        for(const auto& tp : trade_seq->trades){
        if(ARB_DEBUG){
            cout << "Preparing Trade " << trade_num << ": " << tp.sell << ">" << tp.buy
                 << " quote:" << fixed << setprecision(8) << tp.quote << " amount:" << trade_seq->amounts[trade_num] << "(" << tp.action << ")" << endl;
            }
            char rate_s[16];
            sprintf(rate_s, "%.8f", tp.quote);
            char amount_s[16];
            sprintf(amount_s, "%.8f", trade_seq->amounts[trade_num]);
            char post_data[120];

            strcpy(post_data, "command=");
            if(tp.action == 'b'){
                strcat(post_data, "buy&currencyPair=");
            }else if(tp.action == 's'){
                strcat(post_data, "sell&currencyPair=");
            }
            //strcat(post_data, "&currencyPair=");
            strcat(post_data, _pair_names[tp.exchange_id].c_str());
            strcat(post_data, "&rate=");
            strcat(post_data, rate_s);
            strcat(post_data, "&amount=");
            strcat(post_data, amount_s);
            /*
            if(immediate_only){
                strcat(post_data, "&immediateOrCancel=1");
            }
            */

            set_curl_post_options(handles[trade_num], header_slist[trade_num], post_data, trade_num);

            ++trade_num;
        }

        /* we start some action by calling perform right away */ 
        curl_multi_perform(multi_handle, &curl_still_running);

        _segment_end = std::chrono::steady_clock::now();
        cout << "Curl Performed: " << std::chrono::duration_cast<std::chrono::microseconds>(_segment_end - _segment_begin).count() << " microseconds after ticker." << endl;

        do {
            struct timeval timeout;
            int rc; /* select() return code */ 
            CURLMcode mc; /* curl_multi_fdset() return code */ 

            fd_set fdread;
            fd_set fdwrite;
            fd_set fdexcep;
            int maxfd = -1;

            long curl_timeo = 25000;

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
                throw ARB_ERR_TRADE_NOT_EX;
            }

            curl_slist_free_all(header_slist[i]);
            curl_easy_cleanup(handles[i]);
        }
        curl_multi_cleanup(multi_handle);

        return all_trades_executed;
    }

    void print_market_data(trade_seq*& trade_seq) const{
        for(const auto& tp : trade_seq->trades){
            cout << "Pair Summary: ID:" << tp.exchange_id << ", " << tp.sell << ">" << tp.buy << " " << tp.action << "@" << fixed << setprecision(8) << tp.quote << " net:" << tp.net << endl;
            _order_books[tp.exchange_id]->print_book();
        }

        sleep(3); //wait for our order to go through

        for(const auto& tp : trade_seq->trades){
            print_trade_history(tp.exchange_id, 10, tp.quote);
        }
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

    inline void process_ticker_ticker(unsigned int pair_id, const rapidjson::Document& ticker_update){

        double ask = stod(ticker_update[2][2].GetString());
        double bid = stod(ticker_update[2][3].GetString());

        //cout << "Ticker Update. Pair:" << pair_id << " ask:" << ask << " bid:" << bid << endl;
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


    unsigned int process_ticker_update(const string& message){

        rapidjson::Document ticker_update;
        ticker_update.Parse(message.c_str());

        int channel_id = ticker_update[0].GetInt();
        unsigned int pair_id;
        switch(channel_id){

            //[1002,null,[126,"238.99999999","239.25328845","239.07169998","-0.00905593","549272.05317976","2348.39754632",0,"241.63999999","228.00000000"]])
            case CHANNEL_TICKER:{
                pair_id = ticker_update[2][0].GetInt();
                if(_funded_pairs[pair_id]){
                    process_ticker_ticker(pair_id, ticker_update);
                }
            } break;

            //[148,531912534,[["o",0,"0.08534593","0.00000000"],["o",0,"0.08519999","49.35309829"]]]
            default: {
                pair_id = ticker_update[0].GetInt();
                unsigned int sequence_id = ticker_update[1].GetInt();
                //cout << "Order Update. Pair:" << pair_id << " Sequence:" << sequence_id << endl;
                if(sequence_id != ++_order_sequences[pair_id]){
                    cout << "SEQUENCE MISMATCH!! ORDERS MAY BE MISSING!!!" << endl;
                    _order_sequences[pair_id] = sequence_id;
                }
                bool any_buy_updates = false;
                bool any_sell_updates = false;
                for(const auto& order : ticker_update[2].GetArray()){
                    if(order[0] == "o"){
                        //cout << "Modify Order. " << ((order[1] == 1) ? "bid: " : "ask: ") << order[2].GetString() << " amount:" << order[3].GetString() << endl;
                        if(order[1] == 1){
                            //returns true if market price was updated
                            if(_order_books[pair_id]->record_buy(stod(order[2].GetString()), stod(order[3].GetString()))){
                                any_buy_updates = true;
                            }
                        } else {
                            if(_order_books[pair_id]->record_sell(stod(order[2].GetString()), stod(order[3].GetString()))){
                                any_sell_updates = true;
                            }
                        }
                    //["t","43464542",1,"0.08514500","0.12873209",1526828523]
                    }else if(order[0] == "t"){
                        /*
                        cout << "Trade. ID:" << order[1].GetString()
                             << ((order[2] == 1) ? " buy: " : " sell: " ) << order[3].GetString()
                             << " amount:" << order[4].GetString() << endl;
                             */

                        //_order_books[pair_id]->record_trade(stod(order[3].GetString()), stod(order[4].GetString()), ((order[2] == 1) ? 'b' : 's' ));
                    }else if(order[0] == "i"){
                        for(const auto& ask_order : order[1]["orderBook"][0].GetObject()){
                            _order_books[pair_id]->record_sell(stod(ask_order.name.GetString()), stod(ask_order.value.GetString()));
                        }
                        any_sell_updates = true;
                        for(const auto& bid_order : order[1]["orderBook"][1].GetObject()){
                            _order_books[pair_id]->record_buy(stod(bid_order.name.GetString()), stod(bid_order.value.GetString()));
                        }
                        any_buy_updates = true;
                    }else {
                        cout << "ERROR, Unknown opration in ticker update" << endl;
                        throw ARB_ERR_BAD_OPTION;
                    }
                }
                if(any_buy_updates){
                    for(auto& tp : _pairs){
                        if(pair_id == tp.exchange_id && 's' == tp.action){
                            auto bid_order = _order_books[pair_id]->highest_bid();
                            tp.quote = bid_order.price;
                            tp.depth = bid_order.amount;
                            tp.net = (1 - _market_fee) * bid_order.price;
                            break;
                        }
                    }
                }
                if(any_sell_updates){
                    for(auto& tp : _pairs){
                        if(pair_id == tp.exchange_id && 'b' == tp.action){
                            auto ask_order = _order_books[pair_id]->lowest_ask();
                            tp.quote = ask_order.price;
                            //tp.sell_amount = ask_order.amount * ask_order.price;
                            tp.depth = ask_order.amount;
                            tp.net = (1 - _market_fee) / ask_order.price;
                            break;
                        }
                    }
                }
             } break;
        }
        return pair_id;

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

    void print_trade_history(unsigned int pair_id, unsigned int num_trades=10, double highlight_rate = 0) const{

        string url = _get_url + "?command=returnTradeHistory&currencyPair=" + _pair_names[pair_id];
        set_curl_get_options(_curl_get, url);

        std::string http_data = curl_get(_curl_get);
        rapidjson::Document trades;
        trades.Parse(http_data.c_str());

        unsigned int count = 0;
        unsigned long trade_id;
        string date, type, rate, amount, total;

        cout << "===Trade History for Pair:" << pair_id << ", " << _pair_names[pair_id] << " ===" << endl;
        for(const auto& trade : trades.GetArray()){
            trade_id = trade["tradeID"].GetInt();
            date = trade["date"].GetString();
            type = trade["type"].GetString();
            rate = trade["rate"].GetString();
            amount = trade["amount"].GetString();
            total = trade["total"].GetString();
            cout << "ID:" << trade_id << " : date:" << date << " : type:" << setw(4) << left << type
                 << " : rate:" << rate << " : amount:" << setw(12) << right << amount << " : total:" << setw(12) << total;
            if(highlight_rate != 0){
                if(abs(stod(rate) - highlight_rate) < 0.000000001){
                    cout << " <---";
                }
            }
            cout << endl;
            if(++count > num_trades){
                break;
            }
        }

    }


    void populate_poloniex_trade_pairs(){

        set_curl_get_options(_curl_get, _get_url + "?command=returnTicker");

        std::string http_data = curl_get(_curl_get);
        rapidjson::Document products;
        products.Parse(http_data.c_str());

        trade_pair tp;
        char curr_1[8];
        char curr_2[8];
        const char* market;
        const char* it;
        const char* sep_it;
        unsigned int pair_id;
        for(const auto& listed_pair : products.GetObject()){
            if(ARB_DEBUG){
                cout << "Import Pair: " << listed_pair.name.GetString()
                     << " bid: " << listed_pair.value["highestBid"].GetString()
                     << " ask: " << listed_pair.value["lowestAsk"].GetString() << endl;
            }

            pair_id = listed_pair.value["id"].GetInt();
            _pair_names[pair_id] = listed_pair.name.GetString();
            _order_books[pair_id] = new order_book;

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
                _funded_pairs[pair_id] = true;

                strcpy(tp.sell, curr_2);
                strcpy(tp.buy, curr_1);
                tp.exchange_id = pair_id;
                tp.quote = stod(listed_pair.value["highestBid"].GetString());
                tp.net = (1 - _market_fee) * tp.quote;
                tp.action = 's';
                _pairs.push_back(tp);

                strcpy(tp.sell, curr_1);
                strcpy(tp.buy, curr_2);
                tp.exchange_id = pair_id;
                tp.quote = stod(listed_pair.value["lowestAsk"].GetString());
                tp.net = (1 - _market_fee) / tp.quote;
                tp.action = 'b';
                _pairs.push_back(tp);

            }
        }
    }


};

#endif
