#pragma once

#include <cstdint>
#include <cstddef>
#include <new>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <type_traits>

#include "macros.h"

namespace Common {
  template<typename T>
  class MemPool final {
  public:
    explicit MemPool(std::size_t num_elems) : store_(num_elems) {
      ASSERT((num_elems & (num_elems - 1)) == 0, "MemPool size must be power of 2");
      for (auto &block : store_) {
        block.is_free_.store(true, std::memory_order_relaxed);
      }
    }

    ~MemPool() {
      for (auto &block : store_) {
        if (!block.is_free_.load(std::memory_order_acquire)) {
          block.ptr()->~T();
          block.is_free_.store(true, std::memory_order_relaxed);
        }
      }
    }

    /// Allocate a new object of type T with placement new. Returns nullptr if exhausted.
    template<typename... Args>
    T *allocate(Args &&... args) noexcept {
      for (int retry = 0; retry < 1000; ++retry) {
        auto current = next_free_.load(std::memory_order_relaxed);
        auto next = (current + 1) & (store_.size() - 1);

        if (store_[current].is_free_.exchange(false, std::memory_order_acquire)) {
          next_free_.store(next, std::memory_order_relaxed);
          T *ret = new (store_[current].ptr()) T(std::forward<Args>(args)...);
          return ret;
        }

        if (retry > 10) {
          std::this_thread::yield();
        }
      }

      return nullptr;
    }

    /// Destroy the object and return the slot to the pool.
    auto deallocate(const T *elem) noexcept -> void {
      const auto elem_index = (reinterpret_cast<const ObjectBlock *>(elem) - &store_[0]);
      ASSERT(elem_index >= 0 && static_cast<size_t>(elem_index) < store_.size(),
             "Element being deallocated does not belong to this Memory pool.");
      ASSERT(!store_[elem_index].is_free_.load(std::memory_order_acquire),
             "Double free in MemPool.");

      store_[elem_index].ptr()->~T();
      store_[elem_index].is_free_.store(true, std::memory_order_release);
    }

    MemPool() = delete;
    MemPool(const MemPool &) = delete;
    MemPool(MemPool &&) = delete;
    MemPool &operator=(const MemPool &) = delete;
    MemPool &operator=(MemPool &&) = delete;

  private:
    struct ObjectBlock {
      alignas(T) std::byte storage_[sizeof(T)];
      alignas(64) std::atomic<bool> is_free_ = {true};

      T *ptr() noexcept { return reinterpret_cast<T *>(storage_); }
      const T *ptr() const noexcept { return reinterpret_cast<const T *>(storage_); }
    };

    std::vector<ObjectBlock> store_;
    alignas(64) std::atomic<size_t> next_free_ = {0};
  };
}
