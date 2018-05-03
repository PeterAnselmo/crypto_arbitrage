arb: arb.cpp arb_util.cpp crypto_exchange.cpp
	g++ arb.cpp -std=c++11 -O2 -lcurl -lcryptopp -lpthread -lm -lcpprest -o arb
trade: trade.cpp arb_util.cpp crypto_exchange.cpp
	g++ trade.cpp -std=c++11 -O2 -lcurl -lcryptopp -lpthread -lm -lcpprest -o trade
tests:
	g++ test/main.cpp -std=c++11 -lcurl -lcryptopp -lgtest -lpthread -lm -lcpprest -o test/main_test
benchmark: benchmark.cpp
	g++ benchmark.cpp -std=c++11 -O2 -lcurl -lcryptopp -lpthread -lm -lcpprest -o benchmark
