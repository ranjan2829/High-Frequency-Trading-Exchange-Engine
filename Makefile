# Lightweight fallback when cmake is awkward on the host.
CXX ?= clang++
CXXFLAGS ?= -std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wno-unused-parameter -pthread
ROOT := $(abspath .)
INC := -I$(ROOT)/ExchangeMatchingEngine/Common \
	-I$(ROOT)/ExchangeMatchingEngine/EXCHANGE \
	-I$(ROOT)/ExchangeMatchingEngine/EXCHANGE/matcher \
	-I$(ROOT)/ExchangeMatchingEngine/EXCHANGE/order_server \
	-I$(ROOT)/ExchangeMatchingEngine/EXCHANGE/market_data \
	-I$(ROOT)/trading \
	-I$(ROOT)/trading/strategy \
	-I$(ROOT)/trading/market_data \
	-I$(ROOT)/trading/order_gw

COMMON := \
	ExchangeMatchingEngine/Common/latency_tracker.cpp \
	ExchangeMatchingEngine/Common/performance_dashboard.cpp \
	ExchangeMatchingEngine/Common/tcp_socket.cpp \
	ExchangeMatchingEngine/Common/tcp_server.cpp \
	ExchangeMatchingEngine/Common/mcast_socket.cpp

EX_SRC := \
	ExchangeMatchingEngine/EXCHANGE/exchange_main.cpp \
	ExchangeMatchingEngine/EXCHANGE/matcher/matching_engine.cpp \
	ExchangeMatchingEngine/EXCHANGE/matcher/me_order_book.cpp \
	ExchangeMatchingEngine/EXCHANGE/matcher/me_order.cpp \
	ExchangeMatchingEngine/EXCHANGE/market_data/market_data_publisher.cpp \
	ExchangeMatchingEngine/EXCHANGE/market_data/snapshot_synthesizer.cpp \
	ExchangeMatchingEngine/EXCHANGE/order_server/order_server.cpp \
	$(COMMON)

TR_SRC := \
	trading/trading_main.cpp \
	trading/strategy/trade_engine.cpp \
	trading/strategy/market_order_book.cpp \
	trading/strategy/market_order.cpp \
	trading/strategy/market_maker.cpp \
	trading/strategy/liquidity_taker.cpp \
	trading/strategy/order_manager.cpp \
	trading/strategy/risk_manager.cpp \
	trading/market_data/market_data_consumer.cpp \
	trading/order_gw/order_gateway.cpp \
	$(COMMON)

.PHONY: all clean
all: exchange_main trading_main

exchange_main: $(EX_SRC)
	$(CXX) $(CXXFLAGS) $(INC) $^ -o $@

trading_main: $(TR_SRC)
	$(CXX) $(CXXFLAGS) $(INC) $^ -o $@

clean:
	rm -f exchange_main trading_main


.PHONY: test bench
test: tests/test_lf_queue tests/test_order_book_smoke tests/test_match_smoke
	./tests/test_lf_queue
	./tests/test_order_book_smoke
	./tests/test_match_smoke

tests/test_lf_queue: tests/test_lf_queue.cpp
	$(CXX) $(CXXFLAGS) $(INC) $< -o $@

tests/test_order_book_smoke: tests/test_order_book_smoke.cpp
	$(CXX) $(CXXFLAGS) $(INC) $< -o $@

bench: bench/bench_lf_queue
	./bench/bench_lf_queue

bench/bench_lf_queue: bench/bench_lf_queue.cpp
	$(CXX) $(CXXFLAGS) $(INC) $< -o $@

tests/test_match_smoke: tests/test_match_smoke.cpp $(filter-out ExchangeMatchingEngine/EXCHANGE/exchange_main.cpp,$(EX_SRC))
	$(CXX) $(CXXFLAGS) $(INC) $^ -o $@
