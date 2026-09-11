#include <csignal>
#include <memory>
#include <atomic>

#include "matching_engine.h"
#include "market_data_publisher.h"
#include "order_server.h"
#include "performance_dashboard.h"
#include "latency_tracker.h"

static std::unique_ptr<Common::Logger> logger;
static std::unique_ptr<Exchange::MatchingEngine> matching_engine;
static std::unique_ptr<Exchange::MarketDataPublisher> market_data_publisher;
static std::unique_ptr<Exchange::OrderServer> order_server;
static std::atomic<bool> running{true};

void signal_handler(int) {
  running.store(false, std::memory_order_release);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
  logger = std::make_unique<Common::Logger>("exchange_main.log");

  std::signal(SIGINT, signal_handler);

  std::string time_str;
  
  Common::NanosecondTimer::calibrate();
  Common::g_performance_dashboard.start();
  
  logger->log("%:% %() % Starting NANOSECOND HFT Engine with performance monitoring...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));

  Exchange::ClientRequestLFQueue client_requests(ME_MAX_CLIENT_UPDATES);
  Exchange::ClientResponseLFQueue client_responses(ME_MAX_CLIENT_UPDATES);
  Exchange::MEMarketUpdateLFQueue market_updates(ME_MAX_MARKET_UPDATES);

  logger->log("%:% %() % Starting Nanosecond-Precision Matching Engine...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
  matching_engine = std::make_unique<Exchange::MatchingEngine>(&client_requests, &client_responses, &market_updates);
  matching_engine->start();

  const std::string mkt_pub_iface = "lo";
  const std::string snap_pub_ip = "233.252.14.1", inc_pub_ip = "233.252.14.3";
  const int snap_pub_port = 20000, inc_pub_port = 20001;

  logger->log("%:% %() % Starting Market Data Publisher...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
  market_data_publisher = std::make_unique<Exchange::MarketDataPublisher>(&market_updates, mkt_pub_iface, snap_pub_ip, snap_pub_port, inc_pub_ip, inc_pub_port);
  market_data_publisher->start();

  const std::string order_gw_iface = "lo";
  const int order_gw_port = 12345;

  logger->log("%:% %() % Starting Order Server...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
  order_server = std::make_unique<Exchange::OrderServer>(&client_requests, &client_responses, order_gw_iface, order_gw_port);
  order_server->start();

  logger->log("%:% %() % NANOSECOND HFT Engine started successfully! Performance monitoring active.\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
  
  while (running.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }

  order_server.reset();
  market_data_publisher.reset();
  matching_engine.reset();
  Common::g_performance_dashboard.stop();
  logger.reset();

  return EXIT_SUCCESS;
}
