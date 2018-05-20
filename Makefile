arb: arb.cpp arb_util.cpp crypto_exchange.cpp order_book.cpp
	g++ arb.cpp -std=c++11 -Wall -O3 -lcurl -lcryptopp -lpthread -lm -lcpprest -o arb
trade: trade.cpp arb_util.cpp crypto_exchange.cpp
	g++ trade.cpp -std=c++11 -Wall -O2 -lcurl -lcryptopp -lpthread -lm -lcpprest -o trade
tests:
	g++ test/main.cpp -std=c++11 -Wall -lcurl -lcryptopp -lgtest -lpthread -lm -lcpprest -o test/main_test
benchmark: benchmark.cpp arb_util.cpp crypto_exchange.cpp
	g++ benchmark.cpp -std=c++11 -Wall -O2 -lcurl -lcryptopp -lpthread -lm -lcpprest -o benchmark
grind: grind.cpp crypto_exchange.cpp arb_util.cpp
	g++ grind.cpp -std=c++11 -Wall -O0 -g -lcurl -lcryptopp -lpthread -lm -lcpprest -o grind
