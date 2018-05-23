#ifndef TRADE_SEQ_H
#define TRADE_SEQ_H

#include "trade_pair.cpp"

using namespace std;

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
    bool check_pair(trade_pair tp, double passed_sell_amount = 0){

        double amount;
        //first in a sequence, start with passed balance
        if(passed_sell_amount != 0){
            amounts.clear();
            amount = passed_sell_amount;
        } else {
            amount = next_sell_amount;
        }

        if(tp.action == 'b'){//have sell amount, convert for buy amount
            amount = amount / tp.quote;
        }

        //if first trade, move amount down to depth, otherwise invalidate
        if(tp.depth < amount){
            if(passed_sell_amount != 0){
                amount = tp.depth;
            } else {
                //cout << "Order amount " << fixed << setprecision(8) << amount << " greater then market depth " << tp.depth << ". Permutating Orders." << endl;
                return false;
            }
        }

        double total = amount * tp.quote;
        if(total < 0.0001){
            cout << "Trade " << tp.sell << ">" << tp.buy << " (" << tp.action << ") " << fixed << setprecision(8)
                 << amount << "@" << tp.quote << " would have total < 0.0001. Invalidating." << endl;
            return false;
        }

        amounts.push_back(amount);

        //this is doing double math. It could be simplified by making the market fee in scope
        if(tp.action == 'b'){
            next_sell_amount = amount * tp.quote * tp.net;
        } else {
            next_sell_amount = amount * tp.net;
        }
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

#endif
