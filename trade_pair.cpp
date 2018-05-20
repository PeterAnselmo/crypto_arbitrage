#ifndef TRADE_PAIR_H
#define TRADE_PAIR_H

struct trade_pair {
    unsigned int exchange_id;
    char sell[8];
    char buy[8];
    double quote;
    double net;
    char action; //'b' buy, 's', sell
};

#endif
