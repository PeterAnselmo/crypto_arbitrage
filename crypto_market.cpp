#ifndef CRYPTO_MARKET_H
#define CRYPTO_MARKET_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;

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


};

#endif
