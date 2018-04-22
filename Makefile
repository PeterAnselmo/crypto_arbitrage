arb: arb.cpp arb_util.cpp crypto_market.cpp crypto_exchange.cpp
	g++ arb.cpp -std=c++11 -lcurl -o arb
