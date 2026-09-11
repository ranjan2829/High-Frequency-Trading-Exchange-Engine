#pragma once

#include <vector>
#include <atomic>
#include <cstddef>

#include "macros.h"

namespace Common {
  /// Single-producer / single-consumer ring. Size must be power of 2.
  template<typename T>
  class LFQueue final {
  public:
    explicit LFQueue(std::size_t num_elems) :
        store_(num_elems, T()), mask_(num_elems - 1) {
      ASSERT(num_elems >= 2 && (num_elems & (num_elems - 1)) == 0,
             "LFQueue size must be power of 2");
    }

    auto getNextToWriteTo() noexcept -> T* {
      const auto current_write = write_pos_.load(std::memory_order_relaxed);
      const auto next_write = (current_write + 1) & mask_;
      if (next_write == read_pos_.load(std::memory_order_acquire)) {
        return nullptr; // full (one-slot empty to distinguish full/empty)
      }
      return &store_[current_write & mask_];
    }

    auto updateWriteIndex() noexcept -> void {
      const auto current = write_pos_.load(std::memory_order_relaxed);
      write_pos_.store((current + 1) & mask_, std::memory_order_release);
    }

    auto getNextToRead() const noexcept -> const T * {
      const auto current_read = read_pos_.load(std::memory_order_relaxed);
      if (current_read == write_pos_.load(std::memory_order_acquire)) {
        return nullptr;
      }
      return &store_[current_read & mask_];
    }

    auto updateReadIndex() noexcept -> bool {
      const auto current_read = read_pos_.load(std::memory_order_relaxed);
      if (current_read == write_pos_.load(std::memory_order_acquire)) {
        return false;
      }
      read_pos_.store((current_read + 1) & mask_, std::memory_order_release);
      return true;
    }

    auto size() const noexcept -> size_t {
      const auto w = write_pos_.load(std::memory_order_acquire);
      const auto r = read_pos_.load(std::memory_order_acquire);
      return (w - r) & mask_;
    }

    LFQueue() = delete;
    LFQueue(const LFQueue &) = delete;
    LFQueue(LFQueue &&) = delete;
    LFQueue &operator=(const LFQueue &) = delete;
    LFQueue &operator=(LFQueue &&) = delete;

  private:
    std::vector<T> store_;
    const std::size_t mask_;
    alignas(64) std::atomic<size_t> write_pos_{0};
    alignas(64) std::atomic<size_t> read_pos_{0};
  };
}
