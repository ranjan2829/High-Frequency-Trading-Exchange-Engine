#pragma once

#include <cstdint>
#include <limits>
#include <sstream>
#include <array>

#include "macros.h"

namespace Common {
  /// Constants used across the ecosystem to represent upper bounds on various containers.
  /// Trading instruments / TickerIds from [0, ME_MAX_TICKERS].
  constexpr size_t ME_MAX_TICKERS = 8;

  /// Maximum size of lock free queues used to transfer client requests, client responses and market updates between components.
  /// Must be power of 2 for efficient modulo operations in lock-free queues
  constexpr size_t ME_MAX_CLIENT_UPDATES = 262144; // 2^18 = 256K (power of 2)
  constexpr size_t ME_MAX_MARKET_UPDATES = 262144; // 2^18 = 256K (power of 2)

  /// Maximum trading clients.
  constexpr size_t ME_MAX_NUM_CLIENTS = 256;

  /// Maximum number of orders per trading client.
  /// Must be power of 2 for efficient modulo operations in memory pools
  constexpr size_t ME_MAX_ORDER_IDS = 65536; // 2^16 — demo-friendly RSS (was 1M)

  /// Maximum price level depth in the order books.
  /// Must be power of 2 for efficient modulo operations
  constexpr size_t ME_MAX_PRICE_LEVELS = 4096; // 2^12 — fewer hash collisions than 256

  using OrderId = uint64_t;
  constexpr auto OrderId_INVALID = std::numeric_limits<OrderId>::max();

  [[nodiscard]] inline auto orderIdToString(OrderId order_id) -> std::string {
    if (UNLIKELY(order_id == OrderId_INVALID)) {
      return "INVALID";
    }

    return std::to_string(order_id);
  }

  using TickerId = uint32_t;
  constexpr auto TickerId_INVALID = std::numeric_limits<TickerId>::max();

  [[nodiscard]] inline auto tickerIdToString(TickerId ticker_id) -> std::string {
    if (UNLIKELY(ticker_id == TickerId_INVALID)) {
      return "INVALID";
    }

    return std::to_string(ticker_id);
  }

  using ClientId = uint32_t;
  constexpr auto ClientId_INVALID = std::numeric_limits<ClientId>::max();

  [[nodiscard]] inline auto clientIdToString(ClientId client_id) -> std::string {
    if (UNLIKELY(client_id == ClientId_INVALID)) {
      return "INVALID";
    }

    return std::to_string(client_id);
  }

  using Price = int64_t;
  constexpr auto Price_INVALID = std::numeric_limits<Price>::max();

  [[nodiscard]] inline auto priceToString(Price price) -> std::string {
    if (UNLIKELY(price == Price_INVALID)) {
      return "INVALID";
    }

    return std::to_string(price);
  }

  using Qty = uint32_t;
  constexpr auto Qty_INVALID = std::numeric_limits<Qty>::max();

  [[nodiscard]] inline auto qtyToString(Qty qty) -> std::string {
    if (UNLIKELY(qty == Qty_INVALID)) {
      return "INVALID";
    }

    return std::to_string(qty);
  }

  /// Priority represents position in the FIFO queue for all orders with the same side and price attributes.
  using Priority = uint64_t;
  constexpr auto Priority_INVALID = std::numeric_limits<Priority>::max();

  [[nodiscard]] inline auto priorityToString(Priority priority) -> std::string {
    if (UNLIKELY(priority == Priority_INVALID)) {
      return "INVALID";
    }

    return std::to_string(priority);
  }

  enum class Side : int8_t {
    INVALID = 0,
    BUY = 1,
    SELL = -1,
    MAX = 2
  };

  [[nodiscard]] inline auto sideToString(Side side) -> std::string {
    switch (side) {
      case Side::BUY:
        return "BUY";
      case Side::SELL:
        return "SELL";
      case Side::INVALID:
        return "INVALID";
      case Side::MAX:
        return "MAX";
    }

    return "UNKNOWN";
  }

  /// Convert Side to an index which can be used to index into a std::array.
  inline constexpr auto sideToIndex(Side side) noexcept {
    return static_cast<size_t>(side) + 1;
  }

  /// Convert Side::BUY=1 and Side::SELL=-1.
  inline constexpr auto sideToValue(Side side) noexcept {
    return static_cast<int>(side);
  }

  /// Type of trading algorithm.
  enum class AlgoType : int8_t {
    INVALID = 0,
    RANDOM = 1,
    MAKER = 2,
    TAKER = 3,
    MAX = 4
  };

  [[nodiscard]] inline auto algoTypeToString(AlgoType type) -> std::string {
    switch (type) {
      case AlgoType::RANDOM:
        return "RANDOM";
      case AlgoType::MAKER:
        return "MAKER";
      case AlgoType::TAKER:
        return "TAKER";
      case AlgoType::INVALID:
        return "INVALID";
      case AlgoType::MAX:
        return "MAX";
    }

    return "UNKNOWN";
  }

  [[nodiscard]] inline auto stringToAlgoType(const std::string &str) -> AlgoType {
    for (auto i = static_cast<int>(AlgoType::INVALID); i <= static_cast<int>(AlgoType::MAX); ++i) {
      const auto algo_type = static_cast<AlgoType>(i);
      if (algoTypeToString(algo_type) == str)
        return algo_type;
    }

    return AlgoType::INVALID;
  }

  /// Risk configuration containing limits on risk parameters for the RiskManager.
  struct RiskCfg {
    Qty max_order_size_ = 0;
    Qty max_position_ = 0;
    double max_loss_ = 0;

    auto toString() const {
      std::stringstream ss;

      ss << "RiskCfg{"
         << "max-order-size:" << qtyToString(max_order_size_) << " "
         << "max-position:" << qtyToString(max_position_) << " "
         << "max-loss:" << max_loss_
         << "}";

      return ss.str();
    }
  };

  /// Top level configuration to configure the TradeEngine, trading algorithm and RiskManager.
  struct TradeEngineCfg {
    Qty clip_ = 0;
    double threshold_ = 0;
    RiskCfg risk_cfg_;

    auto toString() const {
      std::stringstream ss;
      ss << "TradeEngineCfg{"
         << "clip:" << qtyToString(clip_) << " "
         << "thresh:" << threshold_ << " "
         << "risk:" << risk_cfg_.toString()
         << "}";

      return ss.str();
    }
  };

  using TradeEngineCfgHashMap = std::array<TradeEngineCfg, ME_MAX_TICKERS>;
}
