#include <cassert>
#include <cstdio>
#include "lf_queue.h"

int main() {
  Common::LFQueue<int> q(8);
  assert(q.size() == 0);
  assert(q.getNextToRead() == nullptr);

  for (int i = 0; i < 7; ++i) {
    auto *w = q.getNextToWriteTo();
    assert(w);
    *w = i;
    q.updateWriteIndex();
  }
  assert(q.getNextToWriteTo() == nullptr); // full (one slot reserved)

  for (int i = 0; i < 7; ++i) {
    auto *r = q.getNextToRead();
    assert(r && *r == i);
    assert(q.updateReadIndex());
  }
  assert(q.getNextToRead() == nullptr);

  // wrap stress
  for (int i = 0; i < 1000; ++i) {
    auto *w = q.getNextToWriteTo();
    assert(w);
    *w = i;
    q.updateWriteIndex();
    auto *r = q.getNextToRead();
    assert(r && *r == i);
    assert(q.updateReadIndex());
  }

  std::puts("test_lf_queue OK");
  return 0;
}
