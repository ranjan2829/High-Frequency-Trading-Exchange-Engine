#include <cstdio>
#include <cstdlib>
#include "logging.h"
#include "matching_engine.h"
using namespace Exchange;
using namespace Common;
static void die(const char *m){ std::fprintf(stderr,"FAIL %s\n",m); std::_Exit(1); }
int main() {
  ClientRequestLFQueue reqs(1024);
  ClientResponseLFQueue resps(1024);
  MEMarketUpdateLFQueue md(1024);
  MatchingEngine engine(&reqs, &resps, &md);
  MEClientRequest a{}; a.type_=ClientRequestType::NEW; a.client_id_=1; a.ticker_id_=0; a.order_id_=1; a.side_=Side::BUY; a.price_=100; a.qty_=10;
  MEClientRequest b=a; b.client_id_=2; b.side_=Side::SELL;
  engine.processClientRequest(&a);
  engine.processClientRequest(&b);
  int fills=0;
  while (auto *r = resps.getNextToRead()) {
    if (r->type_ == ClientResponseType::FILLED) ++fills;
    resps.updateReadIndex();
  }
  if (fills < 2) die("expected fills from crossing buy/sell");
  std::puts("test_match_smoke OK");
  std::_Exit(0);
}
