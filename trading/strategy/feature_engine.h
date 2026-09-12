#pragma once

#include <array>
#include <limits>

#include "macros.h"
#include "logging.h"
#include "types.h"
#include "market_order_book.h"
#include "market_update.h"

using namespace Common;

namespace Trading {
  constexpr auto Feature_INVALID = std::numeric_limits<double>::quiet_NaN();

  class FeatureEngine {
  public:
    explicit FeatureEngine(Common::Logger *logger) : logger_(logger) { (void)logger_; }

    auto onOrderBookUpdate(TickerId ticker_id, Price price, Side side, MarketOrderBook* book) noexcept -> void {
      if (UNLIKELY(ticker_id >= ME_MAX_TICKERS)) return;
      const auto bbo = book->getBBO();
      auto &f = features_[ticker_id];
      if (LIKELY(bbo->bid_price_ != Price_INVALID && bbo->ask_price_ != Price_INVALID &&
                 (bbo->bid_qty_ + bbo->ask_qty_) > 0)) {
        f.mkt_price_ = (bbo->bid_price_ * bbo->ask_qty_ + bbo->ask_price_ * bbo->bid_qty_) /
                       static_cast<double>(bbo->bid_qty_ + bbo->ask_qty_);
      }
      HOT_LOG(*logger_, "%:% %() % ticker:% price:% side:% mkt-price:% agg-trade-ratio:%\n",
              __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_), ticker_id,
              Common::priceToString(price).c_str(), Common::sideToString(side).c_str(), f.mkt_price_,
              f.agg_trade_qty_ratio_);
    }

    auto onTradeUpdate(const Exchange::MEMarketUpdate *market_update, MarketOrderBook* book) noexcept -> void {
      if (UNLIKELY(market_update->ticker_id_ >= ME_MAX_TICKERS)) return;
      const auto bbo = book->getBBO();
      auto &f = features_[market_update->ticker_id_];
      if (LIKELY(bbo->bid_price_ != Price_INVALID && bbo->ask_price_ != Price_INVALID)) {
        const auto denom = (market_update->side_ == Side::BUY ? bbo->ask_qty_ : bbo->bid_qty_);
        if (denom > 0) {
          f.agg_trade_qty_ratio_ = static_cast<double>(market_update->qty_) / denom;
        }
      }
      HOT_LOG(*logger_, "%:% %() % % mkt-price:% agg-trade-ratio:%\n", __FILE__, __LINE__, __FUNCTION__,
              Common::getCurrentTimeStr(&time_str_), market_update->toString().c_str(), f.mkt_price_,
              f.agg_trade_qty_ratio_);
    }

    auto getMktPrice(TickerId ticker_id) const noexcept -> double {
      if (UNLIKELY(ticker_id >= ME_MAX_TICKERS)) return Feature_INVALID;
      return features_[ticker_id].mkt_price_;
    }

    auto getAggTradeQtyRatio(TickerId ticker_id) const noexcept -> double {
      if (UNLIKELY(ticker_id >= ME_MAX_TICKERS)) return Feature_INVALID;
      return features_[ticker_id].agg_trade_qty_ratio_;
    }

    FeatureEngine() = delete;
    FeatureEngine(const FeatureEngine &) = delete;
    FeatureEngine(FeatureEngine &&) = delete;
    FeatureEngine &operator=(const FeatureEngine &) = delete;
    FeatureEngine &operator=(FeatureEngine &&) = delete;

  private:
    struct Features {
      double mkt_price_ = Feature_INVALID;
      double agg_trade_qty_ratio_ = Feature_INVALID;
    };

    std::string time_str_;
    [[maybe_unused]] [[maybe_unused]] Common::Logger *logger_ = nullptr;
    std::array<Features, ME_MAX_TICKERS> features_{};
  };
}
