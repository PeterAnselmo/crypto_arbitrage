arb: arb.cpp arb_util.cpp crypto_exchange.cpp
	g++ arb.cpp -std=c++11 -O2 -lcurl -lcryptopp -lpthread -lm -o arb
trade: trade.cpp arb_util.cpp crypto_exchange.cpp
	g++ trade.cpp -std=c++11 -O2 -lcurl -lcryptopp -lpthread -lm -o trade
tests:
	g++ test/main.cpp -std=c++11 -lcurl -lcryptopp -lgtest -lpthread -lm -o test/main_test
