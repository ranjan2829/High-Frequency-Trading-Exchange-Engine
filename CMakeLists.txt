cmake_minimum_required(VERSION 3.20)

project(NanosecondHFT LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -DNDEBUG -flto -funroll-loops -fomit-frame-pointer -falign-functions=64 -falign-loops=64")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer")

# Include directories
include_directories("ExchangeMatchingEngine/Common")
include_directories("ExchangeMatchingEngine/EXCHANGE")
include_directories("ExchangeMatchingEngine/EXCHANGE/matcher")
include_directories("ExchangeMatchingEngine/EXCHANGE/order_server")
include_directories("ExchangeMatchingEngine/EXCHANGE/market_data")
include_directories("trading")
include_directories("trading/strategy")
include_directories("trading/market_data")
include_directories("trading/order_gw")

# Common sources
set(COMMON_SOURCES
    "ExchangeMatchingEngine/Common/latency_tracker.cpp"
    "ExchangeMatchingEngine/Common/performance_dashboard.cpp"
    "ExchangeMatchingEngine/Common/tcp_socket.cpp"
    "ExchangeMatchingEngine/Common/tcp_server.cpp"
    "ExchangeMatchingEngine/Common/mcast_socket.cpp"
)

# Exchange executable
add_executable(exchange_main
    "ExchangeMatchingEngine/EXCHANGE/exchange_main.cpp"
    "ExchangeMatchingEngine/EXCHANGE/matcher/matching_engine.cpp"
    "ExchangeMatchingEngine/EXCHANGE/matcher/me_order_book.cpp"
    "ExchangeMatchingEngine/EXCHANGE/matcher/me_order.cpp"
    "ExchangeMatchingEngine/EXCHANGE/market_data/market_data_publisher.cpp"
    "ExchangeMatchingEngine/EXCHANGE/market_data/snapshot_synthesizer.cpp"
    "ExchangeMatchingEngine/EXCHANGE/order_server/order_server.cpp"
    ${COMMON_SOURCES}
)

target_compile_options(exchange_main PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
target_link_libraries(exchange_main pthread)

# Trading client
add_executable(trading_main
    "trading/trading_main.cpp"
    "trading/strategy/trade_engine.cpp"
    "trading/strategy/market_order_book.cpp"
    "trading/strategy/market_order.cpp"
    "trading/strategy/market_maker.cpp"
    "trading/strategy/liquidity_taker.cpp"
    "trading/strategy/order_manager.cpp"
    "trading/strategy/risk_manager.cpp"
    "trading/market_data/market_data_consumer.cpp"
    "trading/order_gw/order_gateway.cpp"
    ${COMMON_SOURCES}
)

target_compile_options(trading_main PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
target_link_libraries(trading_main pthread)