#ifndef CRYPTO_MARKET_H
#define CRYPTO_MARKET_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;

class crypto_market {

    public:
    vector<trade_seq> seqs;


    void add_pair(trade_pair new_trade_pair){
        pairs.push_back(new_trade_pair);
    }
    void add_seq(trade_seq new_trade_seq){
        seqs.push_back(new_trade_seq);
    }

    void import_pairs(const& crypto_exchange exchange){
        for(const auto& pair : exchange->get_trade_pairs()){

            pairs.push_back(pair);
        }
    }

    int populate_trades(const& crypto_exchange exchange){
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

    /*
    void list_pairs() {
        for(const trade_pair& tp : pairs){
            cout << (*tp.exchange).name() << " " << tp.quote << "-" << tp.base << " " << tp.bid << endl;
        }
    }
    */

    

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
