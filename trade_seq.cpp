#ifndef TRADE_SEQ_H
#define TRADE_SEQ_H

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
    bool add_pair(trade_pair new_trade_pair, double passed_amount = 0){

        //first in a sequence, start with passed balance or known depth
        double amount;
        if(passed_amount != 0){
            amount = passed_amount;
            if( new_trade_pair.sell_amount < amount){
                amount = new_trade_pair.sell_amount;
            }
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

#endif
