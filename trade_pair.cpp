#ifndef TRADE_PAIR_H
#define TRADE_PAIR_H

#include <string>

struct trade_pair {
    string sell;
    string buy;
    float quote;
    float net;
    string action;
};

#endif
