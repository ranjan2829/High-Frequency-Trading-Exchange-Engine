#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <thread>

#include "macros.h"

namespace Common {
  template<typename T>
  class MemPool final {
  public:
    explicit MemPool(std::size_t num_elems) : store_(num_elems) {
      ASSERT((num_elems & (num_elems - 1)) == 0, "MemPool size must be power of 2");
      // Initialize each block separately
      for(auto& block : store_) {
        block.is_free_.store(true, std::memory_order_relaxed);
      }
    }

    /// Allocate a new object of type T, use placement new to initialize the object, mark the block as in-use and return the object.
    template<typename... Args>
    T *allocate(Args... args) noexcept {
      // Lock-free allocation with exponential backoff
      for (int retry = 0; retry < 1000; ++retry) {
        auto current = next_free_.load(std::memory_order_relaxed);
        auto next = (current + 1) & (store_.size() - 1); // Fast modulo for power of 2
        
        if (store_[current].is_free_.exchange(false, std::memory_order_acquire)) {
          next_free_.store(next, std::memory_order_relaxed);
          
          T *ret = &(store_[current].object_);
          ret = new(ret) T(args...); // placement new.
          
          return ret;
        }
        
        // Exponential backoff on contention
        if (retry > 10) {
          std::this_thread::yield();
        }
      }
      
      return nullptr; // Pool exhausted
    }

    /// Return the object back to the pool by marking the block as free again.
    /// Destructor is not called for the object.
    auto deallocate(const T *elem) noexcept -> void {
      const auto elem_index = (reinterpret_cast<const ObjectBlock *>(elem) - &store_[0]);
      ASSERT(elem_index >= 0 && static_cast<size_t>(elem_index) < store_.size(), "Element being deallocated does not belong to this Memory pool.");
      
      store_[elem_index].is_free_.store(true, std::memory_order_release);
    }

    // Deleted default, copy & move constructors and assignment-operators.
    MemPool() = delete;

    MemPool(const MemPool &) = delete;

    MemPool(MemPool &&) = delete;

    MemPool &operator=(const MemPool &) = delete;

    MemPool &operator=(MemPool &&) = delete;

  private:
    /// It is better to have one vector of structs with two objects than two vectors of one object.
    /// Consider how these are accessed and cache performance.
    struct ObjectBlock {
      T object_;
      alignas(64) std::atomic<bool> is_free_ = {true};
    };

    /// We could've chosen to use a std::array that would allocate the memory on the stack instead of the heap.
    /// We would have to measure to see which one yields better performance.
    /// It is good to have objects on the stack but performance starts getting worse as the size of the pool increases.
    std::vector<ObjectBlock> store_;

    /// Atomic tracker for next free index with proper memory alignment
    alignas(64) std::atomic<size_t> next_free_ = {0};
  };
}
