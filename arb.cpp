#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>

bool ARB_DEBUG = true;

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "arb_util.cpp"
#include "crypto_exchange.cpp"
#include "crypto_market.cpp"

using namespace std;



void build_coinbase_graph(crypto_market &market){

    cout << "Building Coinbase Graph..." << endl;
    crypto_exchange* coinbase = new crypto_exchange("coinbase");

    const std::string url("https://api.gdax.com/products");
    std::string http_data = curl_get(url);
    rapidjson::Document products;
    products.Parse(http_data.c_str());

    trade_pair tp;
    tp.exchange = coinbase;
    for(auto& listed_pair : products.GetArray()){
        if(ARB_DEBUG){
            cout << "Fetcing Pair: " << listed_pair["id"].GetString() << "..." << endl;
        }

        char url[127];
        strcpy(url, "https://api.gdax.com/products/");
        strcat(url, listed_pair["id"].GetString());
        strcat(url, "/book");

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
            cout << "Importing Pair: " << listed_pair["id"].GetString() 
                 << " bid: " << book_data["bids"][0][0].GetString()
                 << " ask: " << book_data["asks"][0][0].GetString()<< endl;
            tp.buy = listed_pair["base_currency"].GetString();
            tp.sell = listed_pair["quote_currency"].GetString();
            tp.bid = stof(book_data["bids"][0][0].GetString());
            tp.ask = stof(book_data["asks"][0][0].GetString());
            market.add_pair(tp);

            tp.sell = listed_pair["base_currency"].GetString();
            tp.buy = listed_pair["quote_currency"].GetString();
            tp.ask = 1 / stof(book_data["bids"][0][0].GetString());
            tp.bid = 1 / stof(book_data["asks"][0][0].GetString());
            market.add_pair(tp);
        }

    }

}


void build_sample_graph(crypto_market &market){
    cout << "Building Graph..." << endl;

    crypto_exchange* coinbase = new crypto_exchange("foobase");

    trade_pair tp1;
    tp1.exchange = coinbase;
    tp1.buy = "BTC";
    tp1.sell = "USD";
    tp1.bid = 0.000099;
    tp1.ask = 0.000099;
    market.add_pair(tp1);

    trade_pair tp1r;
    tp1r.exchange = coinbase;
    tp1r.buy = "USD";
    tp1r.sell = "BTC";
    tp1r.bid = 10100.49;
    tp1r.ask = 10100.51;
    market.add_pair(tp1r);

    trade_pair tp2;
    tp2.exchange = coinbase;
    tp2.buy = "BTC";
    tp2.sell = "BCH";
    tp2.bid = 0.149;
    tp2.ask = 0.151;
    market.add_pair(tp2);

    trade_pair tp2r;
    tp2r.exchange = coinbase;
    tp2r.buy = "BCH";
    tp2r.sell = "BTC";
    tp2r.bid = 6.66;
    tp2r.ask = 6.66;
    market.add_pair(tp2r);

    trade_pair tp3;
    tp3.exchange = coinbase;
    tp3.buy = "USD";
    tp3.sell = "BCH";
    tp3.bid = 1600;
    tp3.ask = 1600;
    market.add_pair(tp3);

    trade_pair tp3r;
    tp3r.exchange = coinbase;
    tp3r.buy = "BCH";
    tp3r.sell = "USD";
    tp3r.bid = 0.000625;
    tp3r.ask = 0.000625;
    market.add_pair(tp3r);

};

int traverse_graph(crypto_market& market){
    cout << "Traversing Graph..." << endl;
    int trades_added = 0;

    //Naieve O(n^3) terrible graph traversal
    for(const trade_pair& tp1 : market.pairs){
        for(const trade_pair& tp2 : market.pairs) {
            if(tp1.buy != tp2.sell){
                continue;
            }
            for(const trade_pair& tp3 : market.pairs){
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
                ts.print_seq();
                market.add_seq(ts);
                trades_added += 1;

            }
        }
    }

    //market.list_pairs();
    return trades_added;

}

bool execute_trades(crypto_market& market) {
    cout << "Executing Trades..." << endl;

    trade_seq *most_profitable = nullptr;
    int count = 0;
    
    vector<trade_seq>::iterator it;
    for(it = market.seqs.begin(); it != market.seqs.end(); ++it){
        if((*it).net_gain > 1.0) {
            if(most_profitable == nullptr || (*it).net_gain > most_profitable->net_gain){
                most_profitable = &(*it);
            }
        }
        ++count;
    }
    
    if(most_profitable != nullptr){
        most_profitable->print_seq();
        return true;
    } else {
        cout << "No profitable trades found." << endl;
        return false;
    }
}

int main(int argc, char* argv[]){
    cout << "Starting..." << endl;

    bool trade_found = false;
    int trades_checked = 0;
//    while(!trade_found){
        crypto_market market; 

        market.build_poloniex_graph();
            /*
        crypto_exchange poloniex* = new crypto_exchange("poloniex");
        poloniex.build_graph;
        */

        //build_sample_graph(market);
        //build_coinbase_graph(market);

        trades_checked += traverse_graph(market);

        trade_found = execute_trades(market);
        cout << trades_checked << "Potential trade paths checked." << endl;

 //       chrono::seconds duration(10);
 //       this_thread::sleep_for( duration );
 //   }

    return 0;
}
