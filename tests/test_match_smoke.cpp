#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "logging.h"
#include "matching_engine.h"

using namespace Exchange;
using namespace Common;

static void die(const char *m) {
  std::fprintf(stderr, "FAIL %s\n", m);
  std::exit(1);
}

static void drain_all(ClientResponseLFQueue &q) {
  while (auto *r = q.getNextToRead()) {
    (void)r;
    q.updateReadIndex();
  }
}

static void drain_md(MEMarketUpdateLFQueue &q) {
  while (auto *u = q.getNextToRead()) {
    (void)u;
    q.updateReadIndex();
  }
}

int main() {
  ClientRequestLFQueue reqs(1024);
  ClientResponseLFQueue resps(1024);
  MEMarketUpdateLFQueue md(1024);
  {
    MatchingEngine engine(&reqs, &resps, &md);

    // 1) Full cross → two FILLED
    {
      MEClientRequest a{};
      a.type_ = ClientRequestType::NEW;
      a.client_id_ = 1;
      a.ticker_id_ = 0;
      a.order_id_ = 1;
      a.side_ = Side::BUY;
      a.price_ = 100;
      a.qty_ = 10;
      MEClientRequest b = a;
      b.client_id_ = 2;
      b.order_id_ = 2;
      b.side_ = Side::SELL;
      engine.processClientRequest(&a);
      engine.processClientRequest(&b);
      int fills = 0, accepts = 0;
      while (auto *r = resps.getNextToRead()) {
        if (r->type_ == ClientResponseType::FILLED) ++fills;
        if (r->type_ == ClientResponseType::ACCEPTED) ++accepts;
        resps.updateReadIndex();
      }
      if (accepts < 2) die("expected accepts");
      if (fills < 2) die("expected fills from crossing buy/sell");
      drain_md(md);
    }

    // 2) Partial fill then cancel residual
    {
      MEClientRequest bid{};
      bid.type_ = ClientRequestType::NEW;
      bid.client_id_ = 3;
      bid.ticker_id_ = 0;
      bid.order_id_ = 10;
      bid.side_ = Side::BUY;
      bid.price_ = 50;
      bid.qty_ = 5;
      engine.processClientRequest(&bid);
      drain_all(resps);
      drain_md(md);

      MEClientRequest ask{};
      ask.type_ = ClientRequestType::NEW;
      ask.client_id_ = 4;
      ask.ticker_id_ = 0;
      ask.order_id_ = 11;
      ask.side_ = Side::SELL;
      ask.price_ = 50;
      ask.qty_ = 2;
      engine.processClientRequest(&ask);

      int fills = 0;
      while (auto *r = resps.getNextToRead()) {
        if (r->type_ == ClientResponseType::FILLED) ++fills;
        resps.updateReadIndex();
      }
      if (fills < 2) die("expected partial-fill fills");
      drain_md(md);

      MEClientRequest can{};
      can.type_ = ClientRequestType::CANCEL;
      can.client_id_ = 3;
      can.ticker_id_ = 0;
      can.order_id_ = 10;
      engine.processClientRequest(&can);
      int canceled = 0, reject = 0;
      while (auto *r = resps.getNextToRead()) {
        if (r->type_ == ClientResponseType::CANCELED) ++canceled;
        if (r->type_ == ClientResponseType::CANCEL_REJECTED) ++reject;
        resps.updateReadIndex();
      }
      if (canceled != 1 || reject != 0) die("expected residual cancel");
      drain_md(md);
    }

    // 3) Cancel unknown → reject
    {
      MEClientRequest can{};
      can.type_ = ClientRequestType::CANCEL;
      can.client_id_ = 99;
      can.ticker_id_ = 0;
      can.order_id_ = 999;
      engine.processClientRequest(&can);
      int reject = 0;
      while (auto *r = resps.getNextToRead()) {
        if (r->type_ == ClientResponseType::CANCEL_REJECTED) ++reject;
        resps.updateReadIndex();
      }
      if (reject != 1) die("expected cancel reject");
    }

    // 4) Empty-level re-match (regression for null BBO after full fill)
    {
      MEClientRequest s1{};
      s1.type_ = ClientRequestType::NEW;
      s1.client_id_ = 5;
      s1.ticker_id_ = 1;
      s1.order_id_ = 20;
      s1.side_ = Side::SELL;
      s1.price_ = 200;
      s1.qty_ = 1;
      MEClientRequest s2 = s1;
      s2.order_id_ = 21;
      s2.price_ = 201;
      s2.qty_ = 1;
      engine.processClientRequest(&s1);
      engine.processClientRequest(&s2);
      drain_all(resps);
      drain_md(md);

      MEClientRequest buy{};
      buy.type_ = ClientRequestType::NEW;
      buy.client_id_ = 6;
      buy.ticker_id_ = 1;
      buy.order_id_ = 22;
      buy.side_ = Side::BUY;
      buy.price_ = 201;
      buy.qty_ = 2;
      engine.processClientRequest(&buy);
      int fills = 0;
      while (auto *r = resps.getNextToRead()) {
        if (r->type_ == ClientResponseType::FILLED) ++fills;
        resps.updateReadIndex();
      }
      // buy matches both levels: 2 fills for aggressor + 2 for passives = 4
      if (fills < 4) die("expected multi-level fills after empty-level advance");
      drain_md(md);
    }

    std::puts("test_match_smoke OK");
  } // MatchingEngine + Logger destroy cleanly (no _Exit)
  return 0;
}
