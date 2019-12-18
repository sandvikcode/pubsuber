#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

#include "ExpoBackoff.h"

using namespace pubsuber;
using namespace std::chrono;

TEST(ExpoBackoff, MinValues) {
  ExponentialBackoffPolicy policy;
  policy._maxDelay = 10000s;

  std::vector<std::vector<uint32_t>> vec;

  constexpr auto Stride = 10;
  constexpr auto Iterations = 10000;

  vec.resize(Stride);
  for (auto &v : vec) {
    v.resize(Iterations);
  }

  for (auto i = 0; i < Iterations; ++i) {
    ExpoBackoff backoff(policy);
    for (auto s = 0; s < Stride; ++s) {
      uint32_t v = backoff.Delay().count();
      vec[s][i] = v;
    }
  }

  for (auto &v : vec) {
    std::sort(v.begin(), v.end());
    assert(v.size() == Iterations);

    // We expect min value to be equal to start value and not less
    EXPECT_GE(v[0], policy._initialDelay.count());

    double sum = 0.0;
    sum = std::accumulate(v.begin(), v.end(), sum);
    const double avg = sum / v.size();

    // Average must be > 0
    EXPECT_GT(avg, 0.0);

    // Calcualte sigma
    double sigma = 0.0;
    for (auto &x : v) {
      sigma += (x - avg) * (x - avg);
    }
    sigma /= v.size();
    sigma = std::sqrt(sigma);

    std::cout << "Min:" << v[0] << " Max:" << v[v.size() - 1] << " Avg:" << avg << " Sigma:" << sigma << std::endl;
  }
}
