#include "latency_tracker.h"

namespace Common {
  // Static member definitions
  std::atomic<double> NanosecondTimer::tsc_frequency_ns_{0.0};
  std::atomic<bool> NanosecondTimer::calibrated_{false};
  
  /// Global latency tracker instance
  LatencyTracker g_latency_tracker;
}
