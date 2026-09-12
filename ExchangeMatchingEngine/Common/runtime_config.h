#pragma once

#include <cstdlib>
#include <string>

namespace Common {
  /// Runtime endpoints — overridable via env for production deploys.
  struct RuntimeConfig {
    std::string iface = "lo";
    std::string order_ip = "127.0.0.1";
    int order_port = 12345;
    std::string snapshot_ip = "233.252.14.1";
    int snapshot_port = 20000;
    std::string incremental_ip = "233.252.14.3";
    int incremental_port = 20001;

    static auto fromEnv() -> RuntimeConfig {
      RuntimeConfig c;
      if (const char *v = std::getenv("HFT_IFACE")) c.iface = v;
      if (const char *v = std::getenv("HFT_ORDER_IP")) c.order_ip = v;
      if (const char *v = std::getenv("HFT_ORDER_PORT")) c.order_port = std::atoi(v);
      if (const char *v = std::getenv("HFT_SNAPSHOT_IP")) c.snapshot_ip = v;
      if (const char *v = std::getenv("HFT_SNAPSHOT_PORT")) c.snapshot_port = std::atoi(v);
      if (const char *v = std::getenv("HFT_INCREMENTAL_IP")) c.incremental_ip = v;
      if (const char *v = std::getenv("HFT_INCREMENTAL_PORT")) c.incremental_port = std::atoi(v);
      return c;
    }
  };
}
