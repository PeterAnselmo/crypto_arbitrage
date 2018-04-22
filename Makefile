arb: arb.cpp arb_util.cpp crypto_market.cpp crypto_exchange.cpp
	g++ arb.cpp -std=c++11 -lcurl -o arb
tests: arb_util.cpp crypto_market.cpp crypto_exchange.cpp crypto_exchange_test.cpp
	g++ crypto_exchange_test.cpp -std=c++11 -lcurl -o crypto_exchange_test
