#include <cassert>
#include <cstdio>
#include <memory>

// Smoke: LFQ + types compile with sparse maps via matching headers.
#include "types.h"
#include "lf_queue.h"

int main() {
  assert(Common::ME_MAX_TICKERS == 8);
  Common::LFQueue<int> q(16);
  auto *w = q.getNextToWriteTo();
  assert(w);
  *w = 42;
  q.updateWriteIndex();
  assert(*q.getNextToRead() == 42);
  std::puts("test_order_book_smoke OK");
  return 0;
}
