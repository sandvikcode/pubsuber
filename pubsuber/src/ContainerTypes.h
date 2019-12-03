#pragma once
#include <algorithm>
#include <chrono>
#include <list>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace pubsuber {

  struct AckWatchDescriptior {
    AckWatchDescriptior(std::chrono::steady_clock::time_point next = std::chrono::steady_clock::now());

    using TP = std::chrono::steady_clock::time_point;

    auto ExtendIn(TP now) const { return std::chrono::duration_cast<std::chrono::milliseconds>(_nextAck - now); }

    void UpdateNextAck(std::chrono::steady_clock::time_point t) { _nextAck = t; }

    TP _nextAck;
  };

  using AckID = std::string;
  using WatchDescrContainer = std::unordered_map<AckID, AckWatchDescriptior>;
  using AckIDIteratorList = std::list<WatchDescrContainer::iterator>;
  using AckIDSet = std::unordered_set<AckID>;
  using AckIDPack = std::tuple<AckID, WatchDescrContainer::iterator>;

  // custom hash can be a standalone function object:
  struct PackSetHelper {
    bool operator()(const AckIDPack &lhs, const AckIDPack &rhs) const { return std::get<0>(lhs) == std::get<0>(rhs); }
    std::size_t operator()(AckIDPack const &p) const noexcept { return std::hash<std::string>{}(std::get<0>(p)); }
  };

  using AckIDPackSet = std::unordered_set<AckIDPack, PackSetHelper, PackSetHelper>;

}  // namespace pubsuber
