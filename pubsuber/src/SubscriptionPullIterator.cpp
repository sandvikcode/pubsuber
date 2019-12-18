#include "SubscriptionPullIterator.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cassert>
#include <vector>
#include "MessageImpl.h"
#include "Retriable.h"
#include "google/pubsub/v1/pubsub.grpc.pb.h"
#include "google/pubsub/v1/pubsub.pb.h"

using namespace pubsuber;
using namespace std::chrono;
using google::pubsub::v1::PullRequest;
using google::pubsub::v1::PullResponse;
using google::pubsub::v1::Subscriber;
using grpc::ClientContext;

SubscriptionPullIterator::SubscriptionPullIterator(std::string subscriptionName, int32_t maxPrefetch, Callback &callback, ModAckIteratorPtr ackIterator)
: _subscriptionName(subscriptionName)
, _maxMessagePrefetch(maxPrefetch)
, _callback(callback)
, _ackIterator(ackIterator) {}

void SubscriptionPullIterator::Pull(std::unique_ptr<google::pubsub::v1::Subscriber::Stub> &subscriber) {
  ClientContext ctx;
  PullRequest request;
  PullResponse response;

  request.set_subscription(_subscriptionName);
  request.set_max_messages(_maxMessagePrefetch);
  request.set_return_immediately(true);

  retriable::set_deadline(ctx);
  auto status = subscriber->Pull(&ctx, request, &response);

  if (status.ok()) {
    spdlog::trace("Pulled {} message(s)", response.received_messages_size());

    std::vector<MessagePtr> vec;
    const auto pseudoNow = std::chrono::steady_clock::now();
    const auto nextAck = pseudoNow + kMinAckDeadline - 1s;
    // We have separate loop to schedule deadline extention to not mix it with message delivery to user

    for (auto &msg : *response.mutable_received_messages()) {
      std::string ackid(msg.ack_id());
      _ackIterator->AddDeadlineWatcher(std::move(ackid), nextAck);
      std::unique_ptr<::google::pubsub::v1::PubsubMessage> ptr(msg.release_message());
      vec.emplace_back(std::make_unique<MessageImpl>(_ackIterator, _subscriptionName, msg.ack_id(), std::move(ptr), pseudoNow));
    }

    // Deliver message to the user
    while (!vec.empty()) {
      MessagePtr p(std::move(vec.back()));
      vec.pop_back();
      // This call must be fast, sub second
      _callback(std::move(p));
    }

  } else {
    const auto err = "Pull of " + _subscriptionName + " failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
    throw Exception(err, status.error_code());
  }
}
