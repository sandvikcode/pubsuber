#pragma once

#include <chrono>
#include <random>
#include "pubsuber/Pubsuber.h"

namespace pubsuber {

  class ExpoBackoff {
    void CalculateNext();

  public:
    ExpoBackoff(const ExponentialBackoffPolicy &backoffPolicy);
    ~ExpoBackoff() = default;

    std::chrono::milliseconds Delay();
    uint32_t RetryCount() const { return _attempt; }

  private:
    const ExponentialBackoffPolicy _backoffPolicy;
    std::mt19937_64 _gen;
    std::chrono::milliseconds _currentInterval;
    const double _jitter;
    uint32_t _attempt;
  };

}  // namespace pubsuber
