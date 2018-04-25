arb: arb.cpp arb_util.cpp crypto_exchange.cpp
	g++ arb.cpp -std=c++11 -O3 -lcurl -lcryptopp -lgtest -lpthread -lm -o arb
tests:
	g++ test/main.cpp -std=c++11 -lcurl -lcryptopp -lgtest -lpthread -lm -o test/main_test
