#pragma once
#include <grpc++/grpc++.h>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "Distribution.h"
#include "ModAckIterator.h"
#include "google/pubsub/v1/pubsub.grpc.pb.h"
#include "pubsuber/Pubsuber.h"

namespace pubsuber {
  struct Executor;

  using Callback = Subscription::Callback;

  class SubscriptionPullIterator {
  public:
    SubscriptionPullIterator(std::string subscriptionName, int32_t maxPrefetch, Callback &callback, ModAckIteratorPtr ackIterator);

    // Called on PULL thread
    void Pull(std::unique_ptr<google::pubsub::v1::Subscriber::Stub> &subscriber);

  private:
    const std::string _subscriptionName;
    const int32_t _maxMessagePrefetch;
    Callback _callback;
    ModAckIteratorPtr _ackIterator;
  };

  using SubscriptionPullIteratorPtr = std::shared_ptr<SubscriptionPullIterator>;
}  // namespace pubsuber
