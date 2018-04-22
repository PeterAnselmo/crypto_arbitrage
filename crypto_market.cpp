#ifndef CRYPTO_MARKET_H
#define CRYPTO_MARKET_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;

struct trade_pair {
    crypto_exchange* exchange;
    string buy;
    string sell;
    float bid;
    float ask;
};

struct trade_seq {
    vector<trade_pair> pairs;
    float net_gain = 1;

    void add_pair(trade_pair new_trade_pair){
        pairs.push_back(new_trade_pair);
        net_gain = net_gain * new_trade_pair.ask * (1 - new_trade_pair.exchange->market_fee());
    }

    void print_seq(){
        cout << "Trade Seq: ";
        for(const trade_pair& tp : pairs){
            cout << tp.exchange->name() << ":" << tp.sell << ">" << tp.buy << "@" << tp.ask << ", ";
        }
        cout << "Net Change:" << net_gain << endl;
    }
};

class crypto_market {

    public:
    vector<trade_pair> pairs;
    vector<trade_seq> seqs;


    void add_pair(trade_pair new_trade_pair){
        pairs.push_back(new_trade_pair);
    }
    void add_seq(trade_seq new_trade_seq){
        seqs.push_back(new_trade_seq);
    }

    void list_pairs() {
        for(const trade_pair& tp : pairs){
            cout << (*tp.exchange).name() << " " << tp.buy << "-" << tp.sell << " " << tp.bid << endl;
        }
    }

    /*
    void build_graph(){
       swtich(_name){
           case "coinbase":
               break;
           case "poloniex":
               build_poloniex_graph;
               break;
            default:
               cout << "Error, build_graph called for unknown exchange." << endl;
               break;
       }
    }
    */

    void build_poloniex_graph(){
        
        const std::string url("https://poloniex.com/public?command=returnTicker");
        std::string http_data = curl_get(url);
        rapidjson::Document products;
        products.Parse(http_data.c_str());


        trade_pair tp;
        crypto_exchange* poloniex = new crypto_exchange("coinbase");
        tp.exchange = poloniex;

        for(auto& listed_pair : products.GetObject()){
            cout << "Import Pair: " << listed_pair.name.GetString()
                 << " bid: " << listed_pair.value["highestBid"].GetString()
                 << " ask: " << listed_pair.value["lowestAsk"].GetString() << endl;

            std::vector<std::string> pair_names = split(listed_pair.name.GetString(), '_');
            
            tp.sell = pair_names[1];
            tp.buy = pair_names[0];
            tp.bid = stof(listed_pair.value["highestBid"].GetString());
            tp.ask = stof(listed_pair.value["lowestAsk"].GetString());
            add_pair(tp);

            tp.sell = pair_names[0];
            tp.buy = pair_names[1];
            tp.ask = 1 / stof(listed_pair.value["highestBid"].GetString());
            tp.bid = 1 / stof(listed_pair.value["lowestAsk"].GetString());
            add_pair(tp);

        }
    }

};

#endif
