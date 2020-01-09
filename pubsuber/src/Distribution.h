#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <exception>
#include <iterator>
#include <string>
#include <vector>

template <uint32_t HighLimit>
class Distribution {
public:
  // Creates a new distribution capable of holding values from 0 to n-1.
  Distribution() {}

  void Record(uint64_t v) {
    if (v < 0) {
      throw std::runtime_error("Pubsuber: Record: value out of range: " + std::to_string(v));
    } else if (v >= _hist.size()) {
      v = _hist.size() - 1;
    }
    ++_hist[v];
  }

  uint64_t Percentile(double p) const {
    if (p < 0.0 || p > 1.0) {
      throw std::runtime_error("Pubsuber: Percentile: percentile out of range: " + std::to_string(p));
    }

    std::array<uint64_t, HighLimit> sums;
    sums.fill(0);
    uint64_t sum = 0;

    for (uint32_t i = 0; i < _hist.size(); ++i) {
      sum += _hist[i].load();
      sums[i] = sum;
    }

    auto target = uint64_t(std::ceil(double(sum) * p));
    auto it = std::lower_bound(sums.cbegin(), sums.cend(), target);
    if (it == sums.end()) {
      throw std::runtime_error("Pubsuber: target not found in sums: " + std::to_string(target));
    }

    return it - sums.cbegin();
  }

private:
  std::array<std::atomic<uint64_t>, HighLimit> _hist;
};
