#include <iostream>

#include <vector>

using namespace std;


class crypto_exchange {
    string _name;
    float _market_fee;

    public:

    crypto_exchange(string new_name){
        _name = new_name;

        if(_name == "coinbase"){
            _market_fee = 0.25;
        } else {
            cout << "Unknown Exchange" << endl;
        }
    }

    string name(){
        return _name;
    }
    float market_fee() {
        return _market_fee;
    }

};

struct trade_pair {
    crypto_exchange* exchange;
    string buy;
    string sell;
    float bid;
    float ask;
};

struct trade_seq {
    vector<trade_pair*> pairs;
    float net_gain = 1;

    void add(const trade_pair& new_trade_pair){
        cout << "Saving Pair to Seq..." << endl;
        pairs.push_back(&new_trade_pair);
        cout << "Exchange" << new_trade_pair.exchange->name();
        cout << "fee" << new_trade_pair.exchange->market_fee();
        net_gain = net_gain * new_trade_pair.ask * new_trade_pair.exchange->market_fee();
    }

    void print_seq(){
        cout << "Trade Seq: ";
        for(const trade_pair* tp : pairs){
            cout << tp->sell << ">" << tp->buy << "@" << tp->ask << ", ";
        }
        cout << "Net Change:" << net_gain << endl;
    }
};

class crypto_market {

    public:
    vector<trade_pair> pairs;
    vector<trade_seq> seqs;

    void add_pair(trade_pair &new_trade_pair){
        pairs.push_back(new_trade_pair);
    }
    void add_seq(trade_seq &new_trade_seq){
        seqs.push_back(new_trade_seq);
    }

    void list_pairs() {
        for(const trade_pair& tp : pairs){
            cout << tp.exchange << " " << tp.buy << "-" << tp.sell << " " << tp.bid << endl;
        }
    }
};

void build_graph(crypto_market &market){
    cout << "Building Graph..." << endl;

    crypto_exchange coinbase("coinbase");

    trade_pair tp1;
    tp1.exchange = &coinbase;
    tp1.buy = "BTC";
    tp1.sell = "USD";
    tp1.bid = 0.000099;
    tp1.ask = 0.000099;
    market.add_pair(tp1);

    trade_pair tp1r;
    tp1r.exchange = &coinbase;
    tp1r.buy = "USD";
    tp1r.sell = "BTC";
    tp1r.bid = 10100.49;
    tp1r.ask = 10100.51;
    market.add_pair(tp1r);

    trade_pair tp2;
    tp2.exchange = &coinbase;
    tp2.buy = "BTC";
    tp2.sell = "BCH";
    tp2.bid = 0.149;
    tp2.ask = 0.151;
    market.add_pair(tp2);

    trade_pair tp2r;
    tp2r.exchange = &coinbase;
    tp2r.buy = "BCH";
    tp2r.sell = "BTC";
    tp2r.bid = 6.66;
    tp2r.ask = 6.66;
    market.add_pair(tp2r);

    trade_pair tp3;
    tp3.exchange = &coinbase;
    tp3.buy = "USD";
    tp3.sell = "BCH";
    tp3.bid = 1600;
    tp3.ask = 1600;
    market.add_pair(tp3);

    trade_pair tp3r;
    tp3r.exchange = &coinbase;
    tp3r.buy = "BCH";
    tp3r.sell = "USD";
    tp3r.bid = 0.000625;
    tp3r.ask = 0.000625;
    market.add_pair(tp3r);

};

void traverse_graph(crypto_market& market){
    cout << "Traversing Graph..." << endl;

    //Naieve O(n^3) terrible graph traversal
    for(trade_pair& tp1 : market.pairs){
        for(trade_pair& tp2 : market.pairs) {
            if(tp1.buy != tp2.sell){
                continue;
            }
            for(trade_pair& tp3 : market.pairs){
                if(tp2.buy != tp3.sell){
                    continue;
                }
                //make sure it's a triangle
                if(tp3.buy != tp1.sell) {
                    continue;
                }
                cout << "Saving Seq..." << endl;
                trade_seq ts;
                ts.add(tp1);
                ts.add(tp2);
                ts.add(tp3);
                ts.print_seq();
                market.add_seq(ts);
            }
        }
    }

    //market.list_pairs();

}

void execute_trades(crypto_market& market) {
    cout << "Executing Trades..." << endl;

    trade_seq *most_profitable;
    int count = 0;
    
    vector<trade_seq>::iterator it;
    for(it = market.seqs.begin(); it != market.seqs.end(); ++it){
        if(count == 0){
            most_profitable = &(*it);
        }else if(it->net_gain > most_profitable->net_gain){
            most_profitable = &(*it);
        }
        ++count;
    }
    
    cout << "Excuting Trade:" << endl;
    most_profitable->print_seq();
}

int main(int argc, char* argv[]){
    cout << "Starting..." << endl;

    crypto_market market; 

    build_graph(market);

    traverse_graph(market);

    execute_trades(market);

    return 0;
}
