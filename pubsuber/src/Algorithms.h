#pragma once

#include <google/protobuf/message.h>
#include <algorithm>
#include <type_traits>
#include "ContainerTypes.h"

namespace pubsuber {
  const auto kReqFixedOverhead = 100;
  const auto kOverheadPerID = 3;

  inline void erase_keys(WatchDescrContainer &from, const AckIDSet &what) {
    for (auto i = what.begin(), last = what.end(); i != last; ++i) {
      from.erase(*i);
    }
  }

  /**
   * give_me_the_keys returns filled toModify according to redicate return value
   * If predicate returns true item is taken otherwise thrown away
   */
  template <class UnaryPredicate>
  inline void give_me_the_keys(WatchDescrContainer &ids, AckIDPackSet &toModify, UnaryPredicate pred) {
    static_assert(std::is_invocable_r_v<bool, UnaryPredicate, AckWatchDescriptior &>);

    for (auto &pair : ids) {
      const auto &[key, descr] = pair;
      if (pred(descr)) {
        toModify.emplace(key, ids.find(key));
      }
    }
  }

  template <class Container>
  inline void split_request_ids(Container &remainder, Container &toSend, const uint32_t maxSize) {
    auto size = kReqFixedOverhead;
    auto it = remainder.begin();
    toSend.clear();

    for (; size < maxSize && it != remainder.end();) {
      const auto ackIdSize = [&it]() {
        if constexpr (std::is_same<Container, AckIDSet>::value) {
          return (*it).length();
        } else if constexpr (std::is_same<Container, AckIDPackSet>::value) {
          return std::get<0>(*it).length();
        }
      }();

      size += kOverheadPerID + ackIdSize;
      if (size < maxSize) {
        toSend.insert(remainder.extract(it));
        it = remainder.begin();
      }
    }
  }

  template <class Container>
  inline void populate_ack_ids(::google::protobuf::RepeatedPtrField<std::string> &to, Container &from) {
    to.Reserve(static_cast<int>(from.size()));

    if constexpr (std::is_same<Container, AckIDSet>::value) {
      while (!from.empty()) {
        auto elem = to.Add();
        auto node = from.extract(from.begin());
        *elem = std::move(node.value());
      }

    } else if constexpr (std::is_same<Container, AckIDPackSet>::value) {
      for (auto &it : from) {
        auto elem = to.Add();
        *elem = std::get<0>(it);
      }
    } else {
      assert(!"This must be implemented");
    }
  }
}  // namespace pubsuber
