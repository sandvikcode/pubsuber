#include "ExpoBackoff.h"
#include <random>

using namespace pubsuber;
using namespace std::chrono;

namespace {
  const double DefaultJitter = 0.1140430175;

  std::mt19937_64 CreatePRNG() {
    std::random_device rd;
    const auto N = std::mt19937_64::state_size * (std::mt19937_64::word_size / std::numeric_limits<uint32_t>::digits);
    // Get the necessary number of entropy bits
    std::vector<uint32_t> entropy(N);
    std::generate(entropy.begin(), entropy.end(), [&rd]() { return rd(); });
    std::seed_seq seq(entropy.begin(), entropy.end());
    return std::mt19937_64(seq);
  }

  auto toMs(double v) { return std::chrono::milliseconds(static_cast<uint64_t>(v)); }
}  // namespace

ExpoBackoff::ExpoBackoff(ExponentialBackoffPolicy backoffPolicy)
: _backoffPolicy(backoffPolicy)
, _gen(CreatePRNG())
, _jitter(DefaultJitter)
, _attempt(0) {
  _currentInterval = _backoffPolicy._initialDelay;
}

void ExpoBackoff::CalculateNext() {
  double nextTime = _currentInterval.count() * _backoffPolicy._scale;

  if (toMs(nextTime) > _backoffPolicy._maxDelay) {
    nextTime = _backoffPolicy._maxDelay.count();
  }

  if (_jitter < 0.0001) {
    _currentInterval = toMs(nextTime);
  } else {
    const double sigma = _jitter * nextTime;
    std::normal_distribution<> d{nextTime, sigma};
    _currentInterval = toMs(d(_gen));
  }

  if (_currentInterval > _backoffPolicy._maxDelay) {
    _currentInterval = _backoffPolicy._maxDelay;
  }
}

std::chrono::milliseconds ExpoBackoff::Delay() {
  _attempt += 1;
  const milliseconds ret(_currentInterval);
  CalculateNext();
  return ret;
}
