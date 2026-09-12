#include <chrono>
#include <cstdio>
#include "lf_queue.h"

int main() {
  constexpr int N = 1'000'000;
  Common::LFQueue<int> q(1 << 16);
  volatile int sink = 0;
  const auto t0 = std::chrono::steady_clock::now();
  for (int i = 0; i < N; ++i) {
    auto *w = q.getNextToWriteTo();
    while (!w) { w = q.getNextToWriteTo(); }
    *w = i;
    q.updateWriteIndex();
    auto *r = q.getNextToRead();
    while (!r) { r = q.getNextToRead(); }
    sink = *r;
    q.updateReadIndex();
  }
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count();
  std::printf("LFQ roundtrip %d ops: %.2f ns/op sink=%d (host chrono, not exchange RTT)\n", N, double(ns) / N, sink);
  return 0;
}
