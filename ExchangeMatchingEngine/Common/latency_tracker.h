#pragma once

#include <atomic>
#include <array>
#include <string>
#include "nanosecond_timer.h"

namespace Common {
  /// Lock-free latency tracker with nanosecond precision
  class LatencyTracker {
  private:
    struct alignas(64) LatencyBucket {
      std::atomic<uint64_t> count_{0};
      std::atomic<uint64_t> sum_{0};
      std::atomic<uint64_t> min_{UINT64_MAX};
      std::atomic<uint64_t> max_{0};
      
      void record_latency(uint64_t latency_ns) noexcept {
        count_.fetch_add(1, std::memory_order_relaxed);
        sum_.fetch_add(latency_ns, std::memory_order_relaxed);
        
        // Update min atomically
        uint64_t current_min = min_.load(std::memory_order_relaxed);
        while (latency_ns < current_min && 
               !min_.compare_exchange_weak(current_min, latency_ns, 
                                         std::memory_order_relaxed)) {}
        
        // Update max atomically
        uint64_t current_max = max_.load(std::memory_order_relaxed);
        while (latency_ns > current_max && 
               !max_.compare_exchange_weak(current_max, latency_ns, 
                                         std::memory_order_relaxed)) {}
      }
    };
    
    // Latency buckets: 0-999ns, 1000-1999ns, etc.
    static constexpr size_t NUM_BUCKETS = 1000;
    std::array<LatencyBucket, NUM_BUCKETS> buckets_;
    alignas(64) std::atomic<uint64_t> total_operations_{0};
    alignas(64) std::atomic<uint64_t> total_latency_{0};
    
  public:
    LatencyTracker() = default;
    
    /// Record a latency measurement in nanoseconds
    void record_latency(uint64_t latency_ns) noexcept {
      auto bucket = std::min(latency_ns / 1000, static_cast<uint64_t>(NUM_BUCKETS - 1));
      buckets_[bucket].record_latency(latency_ns);
      
      total_operations_.fetch_add(1, std::memory_order_relaxed);
      total_latency_.fetch_add(latency_ns, std::memory_order_relaxed);
    }
    
    /// Get current total operations
    uint64_t get_total_operations() const noexcept {
      return total_operations_.load(std::memory_order_relaxed);
    }
    
    /// Get average latency in nanoseconds
    uint64_t get_average_latency() const noexcept {
      auto ops = total_operations_.load(std::memory_order_relaxed);
      auto total = total_latency_.load(std::memory_order_relaxed);
      return ops > 0 ? total / ops : 0;
    }
    
    /// Get minimum latency in nanoseconds
    uint64_t get_min_latency() const noexcept {
      uint64_t min_latency = UINT64_MAX;
      for (const auto& bucket : buckets_) {
        auto count = bucket.count_.load(std::memory_order_relaxed);
        if (count > 0) {
          min_latency = std::min(min_latency, bucket.min_.load(std::memory_order_relaxed));
        }
      }
      return min_latency == UINT64_MAX ? 0 : min_latency;
    }
    
    /// Get maximum latency in nanoseconds
    uint64_t get_max_latency() const noexcept {
      uint64_t max_latency = 0;
      for (const auto& bucket : buckets_) {
        auto count = bucket.count_.load(std::memory_order_relaxed);
        if (count > 0) {
          max_latency = std::max(max_latency, bucket.max_.load(std::memory_order_relaxed));
        }
      }
      return max_latency;
    }
    
    /// Calculate percentile latency (e.g., 99 for 99th percentile)
    uint64_t get_percentile_latency(double percentile) const noexcept {
      auto total_ops = total_operations_.load(std::memory_order_relaxed);
      if (total_ops == 0) return 0;
      
      auto target_op = static_cast<uint64_t>(total_ops * percentile / 100.0);
      uint64_t current_ops = 0;
      
      for (size_t i = 0; i < NUM_BUCKETS; ++i) {
        auto bucket_ops = buckets_[i].count_.load(std::memory_order_relaxed);
        current_ops += bucket_ops;
        
        if (current_ops >= target_op) {
          return i * 1000; // Return bucket midpoint
        }
      }
      
      return (NUM_BUCKETS - 1) * 1000; // Return max bucket
    }
    
    /// Get latency statistics as string
    std::string get_stats_string() const {
      auto total_ops = get_total_operations();
      auto avg_lat = get_average_latency();
      auto min_lat = get_min_latency();
      auto max_lat = get_max_latency();
      auto p99_lat = get_percentile_latency(99.0);
      auto p99_9_lat = get_percentile_latency(99.9);
      
      return "LatencyStats{ops:" + std::to_string(total_ops) + 
             ", avg:" + std::to_string(avg_lat) + "ns" +
             ", min:" + std::to_string(min_lat) + "ns" +
             ", max:" + std::to_string(max_lat) + "ns" +
             ", p99:" + std::to_string(p99_lat) + "ns" +
             ", p99.9:" + std::to_string(p99_9_lat) + "ns}";
    }
    
    /// Reset all statistics
    void reset() noexcept {
      for (auto& bucket : buckets_) {
        bucket.count_.store(0, std::memory_order_relaxed);
        bucket.sum_.store(0, std::memory_order_relaxed);
        bucket.min_.store(UINT64_MAX, std::memory_order_relaxed);
        bucket.max_.store(0, std::memory_order_relaxed);
      }
      total_operations_.store(0, std::memory_order_relaxed);
      total_latency_.store(0, std::memory_order_relaxed);
    }
  };
  
  /// Global latency tracker instance
  extern LatencyTracker g_latency_tracker;
}

/// Convenience macro for recording latency
#define RECORD_LATENCY(latency_ns) Common::g_latency_tracker.record_latency(latency_ns)

/// RAII latency measurement helper
class LatencyMeasure {
private:
    uint64_t start_time_;
    
public:
    explicit LatencyMeasure([[maybe_unused]] const char* name) {
        start_time_ = Common::NanosecondTimer::now_ns();
    }
    
    ~LatencyMeasure() {
        auto end_time = Common::NanosecondTimer::now_ns();
        auto latency = end_time - start_time_;
        RECORD_LATENCY(latency);
    }
};

#define MEASURE_LATENCY(name) LatencyMeasure _latency_measure(name)
