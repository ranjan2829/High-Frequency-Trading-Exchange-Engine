#pragma once

#include <iostream>
#include <atomic>
#include <thread>
#include <memory>
#include <unistd.h>

#include <sys/syscall.h>

namespace Common {
  inline auto setThreadCore([[maybe_unused]] int core_id) noexcept -> bool {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
#else
    return true;
#endif
  }

  template<typename T, typename... A>
  inline auto createAndStartThread(int core_id, const std::string &name, T &&func, A &&... args) noexcept
      -> std::unique_ptr<std::thread> {
    auto t = std::make_unique<std::thread>(
        [core_id, name, fn = std::forward<T>(func), ...captured_args = std::forward<A>(args)]() mutable {
          if (core_id >= 0 && !setThreadCore(core_id)) {
            std::cerr << "Failed to set core affinity for " << name << " " << pthread_self()
                      << " to " << core_id << std::endl;
            exit(EXIT_FAILURE);
          }
          std::cerr << "Set core affinity for " << name << " " << pthread_self()
                    << " to " << core_id << std::endl;

          fn(std::move(captured_args)...);
        });

    return t;
  }
}
