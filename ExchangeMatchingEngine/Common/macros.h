#pragma once

#include <cstring>
#include <iostream>

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#ifdef __APPLE__
  #ifndef MSG_NOSIGNAL
    #define MSG_NOSIGNAL 0
  #endif
#endif

/// Check condition and exit if not true.
inline auto ASSERT(bool cond, const std::string &msg) noexcept {
  if (UNLIKELY(!cond)) {
    std::cerr << "ASSERT : " << msg << std::endl;

    exit(EXIT_FAILURE);
  }
}

inline auto FATAL(const std::string &msg) noexcept {
  std::cerr << "FATAL : " << msg << std::endl;

  exit(EXIT_FAILURE);
}

/// Hot-path logging: compiled out in Release (NDEBUG) for production latency.
#ifdef NDEBUG
  #define HOT_LOG(...) ((void)0)
#else
  #define HOT_LOG(logger, ...) (logger).log(__VA_ARGS__)
#endif
