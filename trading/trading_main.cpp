#include <csignal>
#include <vector>
#include <random>
#include <memory>

#include "types.h"
#include "time_utils.h"
#include "trade_engine.h"
#include "order_gateway.h"
#include "market_data_consumer.h"
#include "logging.h"
#include "runtime_config.h"

static std::unique_ptr<Common::Logger> logger;
static std::unique_ptr<Trading::TradeEngine> trade_engine;
static std::unique_ptr<Trading::MarketDataConsumer> market_data_consumer;
static std::unique_ptr<Trading::OrderGateway> order_gateway;

/// ./trading_main CLIENT_ID ALGO_TYPE [CLIP_1 THRESH_1 MAX_ORDER_SIZE_1 MAX_POS_1 MAX_LOSS_1] [CLIP_2 THRESH_2 MAX_ORDER_SIZE_2 MAX_POS_2 MAX_LOSS_2] ...
int main(int argc, char **argv) {
  if(argc < 3) {
    FATAL("USAGE trading_main CLIENT_ID ALGO_TYPE [CLIP_1 THRESH_1 MAX_ORDER_SIZE_1 MAX_POS_1 MAX_LOSS_1] [CLIP_2 THRESH_2 MAX_ORDER_SIZE_2 MAX_POS_2 MAX_LOSS_2] ...");
  }

  const Common::ClientId client_id = static_cast<Common::ClientId>(std::stoi(argv[1]));

  const auto algo_type = stringToAlgoType(argv[2]);

  logger = std::make_unique<Common::Logger>("trading_main_" + std::to_string(client_id) + ".log");

  // Removed sleep_time - using event-driven architecture for nanosecond performance

  // The lock free queues to facilitate communication between order gateway <-> trade engine and market data consumer -> trade engine.
  Exchange::ClientRequestLFQueue client_requests(ME_MAX_CLIENT_UPDATES);
  Exchange::ClientResponseLFQueue client_responses(ME_MAX_CLIENT_UPDATES);
  Exchange::MEMarketUpdateLFQueue market_updates(ME_MAX_MARKET_UPDATES);

  std::string time_str;

  TradeEngineCfgHashMap ticker_cfg;

  // Parse and initialize the TradeEngineCfgHashMap above from the command line arguments.
  // [CLIP_1 THRESH_1 MAX_ORDER_SIZE_1 MAX_POS_1 MAX_LOSS_1] [CLIP_2 THRESH_2 MAX_ORDER_SIZE_2 MAX_POS_2 MAX_LOSS_2] ...
  size_t next_ticker_id = 0;
  for (int i = 3; i < argc; i += 5, ++next_ticker_id) {
    ticker_cfg.at(next_ticker_id) = {static_cast<Qty>(std::stoi(argv[i])), std::stod(argv[i + 1]),
                                     {static_cast<Qty>(std::stoi(argv[i + 2])),
                                      static_cast<Qty>(std::stoi(argv[i + 3])),
                                      std::stod(argv[i + 4])}};
  }

  logger->log("%:% %() % Starting Trade Engine...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
  trade_engine = std::make_unique<Trading::TradeEngine>(client_id, algo_type,
                                                         ticker_cfg,
                                                         &client_requests,
                                                         &client_responses,
                                                         &market_updates);
  trade_engine->start();

  const auto cfg = Common::RuntimeConfig::fromEnv();
  const std::string order_gw_ip = cfg.order_ip;
  const std::string order_gw_iface = "lo";
  const int order_gw_port = cfg.order_port;

  logger->log("%:% %() % Starting Order Gateway...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
  order_gateway = std::make_unique<Trading::OrderGateway>(client_id, &client_requests, &client_responses, order_gw_ip, order_gw_iface, order_gw_port);
  order_gateway->start();

  const std::string mkt_data_iface = "lo";
  const std::string snapshot_ip = cfg.snapshot_ip;
  const int snapshot_port = cfg.snapshot_port;
  const std::string incremental_ip = cfg.incremental_ip;
  const int incremental_port = cfg.incremental_port;

  logger->log("%:% %() % Starting Market Data Consumer...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
  market_data_consumer = std::make_unique<Trading::MarketDataConsumer>(client_id, &market_updates, mkt_data_iface, snapshot_ip, snapshot_port, incremental_ip, incremental_port);
  market_data_consumer->start();

  // Removed 10 second sleep - using event-driven initialization

  trade_engine->initLastEventTime();

  if (algo_type == AlgoType::RANDOM) {
    Common::OrderId order_id = client_id * 1000;
    std::vector<Exchange::MEClientRequest> client_requests_vec;
    std::array<Price, ME_MAX_TICKERS> ticker_base_price;

    std::mt19937 rng(client_id);
    std::uniform_int_distribution<Price> base_price_dist(100, 199);
    std::uniform_int_distribution<Common::TickerId> ticker_dist(0, Common::ME_MAX_TICKERS - 1);
    std::uniform_int_distribution<Price> price_offset_dist(1, 10);
    std::uniform_int_distribution<Qty> qty_dist(2, 101);
    std::uniform_int_distribution<int> side_dist(0, 1);

    for (size_t i = 0; i < ME_MAX_TICKERS; ++i)
      ticker_base_price[i] = base_price_dist(rng);
    const size_t ORDERS_PER_CLIENT = 10000; // capped for safe demos
    
    auto start_time = Common::getCurrentNanos();
    
    for (size_t i = 0; i < ORDERS_PER_CLIENT; ++i) {
      const Common::TickerId ticker_id = ticker_dist(rng);
      const Price price = ticker_base_price[ticker_id] + price_offset_dist(rng);
      const Qty qty = qty_dist(rng);
      const Side side = (side_dist(rng) ? Common::Side::BUY : Common::Side::SELL);

      auto order_start = Common::getCurrentNanos();
      
      Exchange::MEClientRequest new_request{Exchange::ClientRequestType::NEW, client_id, ticker_id, order_id++, side,
                                            price, qty};
      trade_engine->sendClientRequest(&new_request);
      
      [[maybe_unused]] auto order_latency = Common::getCurrentNanos() - order_start;

      client_requests_vec.push_back(new_request);
      std::uniform_int_distribution<size_t> cxl_dist(0, client_requests_vec.size() - 1);
      const auto cxl_index = cxl_dist(rng);
      auto cxl_request = client_requests_vec[cxl_index];
      cxl_request.type_ = Exchange::ClientRequestType::CANCEL;
      trade_engine->sendClientRequest(&cxl_request);
      
      if (i > 0 && i % 10000000 == 0) {
        logger->log("%:% %() % Client % processed % orders...\n", __FILE__, __LINE__, __FUNCTION__,
                    Common::getCurrentTimeStr(&time_str), client_id, i);
      }
    }
    
    auto end_time = Common::getCurrentNanos();
    auto total_time_ns = end_time - start_time;
    auto avg_latency_ns = total_time_ns / (ORDERS_PER_CLIENT * 2);
    
    logger->log("%:% %() % Client % COMPLETED: % orders in % ns (local enqueue avg (NOT exchange RTT): % ns)\n", 
                __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str),
                client_id, ORDERS_PER_CLIENT * 2, total_time_ns, avg_latency_ns);
  }

  logger->log("%:% %() % All orders processed, shutting down...\n", __FILE__, __LINE__, __FUNCTION__,
              Common::getCurrentTimeStr(&time_str));

  trade_engine->stop();
  market_data_consumer->stop();
  order_gateway->stop();

  order_gateway.reset();
  market_data_consumer.reset();
  trade_engine.reset();
  logger.reset();

  return EXIT_SUCCESS;
}
