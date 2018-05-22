#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H
#include <forward_list>
#include <cmath>

using namespace std;

struct order{
    double price;
    double amount;
    char action;

    order(){
    }
    order(string new_price, string new_amount){
        price = stod(new_price);
        amount = stod(new_amount);
    }
    order(double new_price, double new_amount){
        price = new_price;
        amount = new_amount;
    }
    order(double new_price, double new_amount, char new_action){
        price = new_price;
        amount = new_amount;
        action = new_action;
    }
};

class order_book {
    order _last_executed;
    forward_list<order> _bids;
    forward_list<order> _asks;

public:
    order_book(){
        _last_executed.price = 0.0;
        _last_executed.amount = 0.0;
        _last_executed.action = 'u';
    }
    order highest_bid() const{
        return _bids.front();
    }
    order lowest_ask() const{
        return _asks.front();
    }

    //O(n) - do not use except for tests!
    int book_size(const char action) const{
        if(action == 'b'){
            return book_size(_bids);
        } else if(action == 's'){
            return book_size(_asks);
        }
        return -1;
    }

    void print_book() const{
        cout << "Last Trade: " << _last_executed.action << " : " << _last_executed.price << " : " << _last_executed.amount << endl;
        cout << "===Order Book===" << endl;
        auto bid_pos = _bids.begin();
        auto ask_pos = _asks.begin();
        bool more_bids, more_asks;
        more_bids = more_asks = true;
        int count = 0;
        int max_rows = 7;
        while(count < max_rows && (more_bids || more_asks)){
            if(bid_pos == _bids.end()){
                more_bids = false;
            }
            if(ask_pos == _asks.end()){
                more_asks = false;
            }
            //cout << "more bids" << more_bids << " more asks" << more_asks << endl;

            if(more_bids && more_asks){
                printf("%18.8f : %13.8f  |  %18.8f : %13.8f\n", ask_pos->price, ask_pos->amount, bid_pos->price, bid_pos->amount);
                ++bid_pos;
                ++ask_pos;
            } else if (more_bids){
                printf("%57.8f : %13.8f\n", bid_pos->price, bid_pos->amount);
                ++bid_pos;
            }else if (more_asks){
                printf("%18.8f : %13.8f\n", ask_pos->price, ask_pos->amount);
                ++ask_pos;
            }
            ++count;
        }
        cout << endl;
    }

    bool record_buy(double price, double amount){
        auto pos_before = _bids.before_begin();
        bool highest_changed = true;
        for(auto pos = _bids.begin(); pos != _bids.end(); ++pos, ++pos_before){
            //float equality to 8 decimal places
            if(abs(price - pos->price) < 0.000000001){
                if(amount < 0.00000001){
                    _bids.erase_after(pos_before);
                    return highest_changed;
                } else {
                    pos->amount = amount;
                    return highest_changed;
                }
            } else if(price > pos->price){
                _bids.insert_after(pos_before, order(price, amount));
                return highest_changed;
            }
            highest_changed = false;
        }
        //wasn't bigger then any bid, put at end
        _bids.insert_after(pos_before, order(price, amount));
        return highest_changed;
    }
    bool record_sell(double price, double amount){
        auto pos_before = _asks.before_begin();
        bool lowest_changed = true;
        for(auto pos = _asks.begin(); pos != _asks.end(); ++pos, ++pos_before){
            if(abs(price - pos->price) < 0.000000001){
                if(amount < 0.00000001){
                    _asks.erase_after(pos_before);
                    return lowest_changed;
                } else {
                    pos->amount = amount;
                    return lowest_changed;
                }
            } else if(price < pos->price){
                _asks.insert_after(pos_before, order(price, amount));
                return lowest_changed;
            }
            lowest_changed = false;
        }
        //wasn't smaller then any ask, put at end
        _asks.insert_after(pos_before, order(price, amount));
        return lowest_changed;
    }
    void record_trade(double new_price, double new_amount, char new_action){
        _last_executed.price = new_price;
        _last_executed.amount = new_amount;
        _last_executed.action = new_action;
    }
    order last_trade() {
        return _last_executed;
    }



private:
    int book_size(const forward_list<order>& book) const{
        int count = 0;
        for(auto pos = book.begin(); pos != book.end(); ++pos){
            ++count;
        }
        return count;
    }

};

#endif
