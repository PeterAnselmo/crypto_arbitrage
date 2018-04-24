arb: arb.cpp arb_util.cpp crypto_market.cpp crypto_exchange.cpp
	g++ arb.cpp -std=c++11 -lcurl -o arb
tests:
	g++ test/crypto_exchange_test.cpp -std=c++11 -lcurl -lcryptopp -lgtest -lpthread -lm -o test/crypto_exchange_test
