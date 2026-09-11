#pragma once

#include <chrono>
#include <atomic>
#include <thread>
#include "macros.h"

namespace Common {
  /// Nanosecond precision timer using TSC (Time Stamp Counter) for maximum performance
  class NanosecondTimer {
  private:
    static std::atomic<double> tsc_frequency_ns_;
    static std::atomic<bool> calibrated_;
    
  public:
    /// Get current nanosecond timestamp using high-resolution clock
    static inline uint64_t now_ns() noexcept {
      return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
    
    /// Get current TSC cycles for high-precision timing (fallback to nanoseconds on Mac)
    static inline uint64_t rdtsc() noexcept {
      return now_ns();
    }
    
    /// Convert TSC cycles to nanoseconds
    static inline uint64_t tsc_to_ns(uint64_t tsc_cycles) noexcept {
      if (UNLIKELY(!calibrated_.load(std::memory_order_acquire))) {
        calibrate();
      }
      return static_cast<uint64_t>(tsc_cycles / tsc_frequency_ns_.load(std::memory_order_relaxed));
    }
    
    /// Calibrate TSC frequency against wall-clock time
    static void calibrate() noexcept {
      if (calibrated_.load(std::memory_order_acquire)) {
        return;
      }
      
      // Get initial timestamps
      auto start_wall = std::chrono::high_resolution_clock::now();
      auto start_tsc = rdtsc();
      
      // Sleep for calibration period (100ms for accuracy)
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      
      auto end_wall = std::chrono::high_resolution_clock::now();
      auto end_tsc = rdtsc();
      
      // Calculate TSC frequency in GHz
      auto wall_duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
          end_wall - start_wall).count();
      auto tsc_duration = end_tsc - start_tsc;
      
      double frequency = static_cast<double>(tsc_duration) / wall_duration_ns;
      tsc_frequency_ns_.store(frequency, std::memory_order_release);
      calibrated_.store(true, std::memory_order_release);
    }
    
    /// Force recalibration (useful for CPU frequency changes)
    static void force_recalibrate() noexcept {
      calibrated_.store(false, std::memory_order_release);
      calibrate();
    }
  };
}

/// High-performance latency measurement macros
#define START_LATENCY_MEASURE(TAG) const auto TAG##_start = Common::NanosecondTimer::now_ns()
#define END_LATENCY_MEASURE(TAG, LOGGER) \
  do { \
    const auto TAG##_end = Common::NanosecondTimer::now_ns(); \
    const auto TAG##_latency = TAG##_end - TAG##_start; \
    LOGGER.log("LATENCY %s: %lu ns\n", #TAG, TAG##_latency); \
  } while(false)

#define START_TSC_MEASURE(TAG) const auto TAG##_start = Common::NanosecondTimer::rdtsc()
#define END_TSC_MEASURE(TAG, LOGGER) \
  do { \
    const auto TAG##_end = Common::NanosecondTimer::rdtsc(); \
    const auto TAG##_cycles = TAG##_end - TAG##_start; \
    const auto TAG##_ns = Common::NanosecondTimer::tsc_to_ns(TAG##_cycles); \
    LOGGER.log("TSC %s: %lu cycles (%lu ns)\n", #TAG, TAG##_cycles, TAG##_ns); \
  } while(false)
