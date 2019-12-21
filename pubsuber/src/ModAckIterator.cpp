#include "ModAckIterator.h"
#include <algorithm>
#include <cassert>
#include <type_traits>
#include "ContainerTypes.h"
#include "Executor.h"
#include "Retriable.h"
#include "google/pubsub/v1/pubsub.pb.h"

using namespace pubsuber;
using namespace std::chrono;
using google::protobuf::Empty;
using google::pubsub::v1::AcknowledgeRequest;
using google::pubsub::v1::ModifyAckDeadlineRequest;
using google::pubsub::v1::PullRequest;
using google::pubsub::v1::PullResponse;
using google::pubsub::v1::ReceivedMessage;
using google::pubsub::v1::Subscriber;
using grpc::ClientContext;

namespace {
  // Following comment and constants are borrowed from Go pubsub client
  //
  // maxPayload is the maximum number of bytes to devote to actual ids in
  // acknowledgement or modifyAckDeadline requests. A serialized
  // AcknowledgeRequest proto has a small constant overhead, plus the size of the
  // subscription name, plus 3 bytes per ID (a tag byte and two size bytes). A
  // ModifyAckDeadlineRequest has an additional few bytes for the deadline. We
  // don't know the subscription name here, so we just assume the size exclusive
  // of ids is 100 bytes.
  //
  // With gRPC there is no way for the client to know the server's max message size (it is
  // configurable on the server). We know from experience that it
  // it 512K.
  const auto kMaxPayload = 512 * 1024;

}  // namespace

AckWatchDescriptior::AckWatchDescriptior(std::chrono::steady_clock::time_point next)
: _nextAck(next) {}

ModAckIterator::ModAckIterator(std::string subscriptionName, std::shared_ptr<Executor> executor, const RetryCountPolicy &countPolicy,
                               const MaxRetryTimePolicy &timePolicy, ExponentialBackoffPolicy &backoffPolicy)
: _subscriptionName(subscriptionName)
, _gracePeriod(kMinAckDeadline / 2)
, _executor(executor)
, _countPolicy(countPolicy)
, _timePolicy(timePolicy)
, _backoffPolicy(backoffPolicy) {}

void ModAckIterator::AddDeadlineWatcher(std::string &&ackID, std::chrono::steady_clock::time_point nextAck) noexcept(true) {
  AckWatchDescriptior descr{nextAck};
  if (auto exec = _executor.lock()) {
    exec->ExecuteUnderAckMutex(
        [this, &ackID, &descr]() {
          auto [it, inserted] = _inputAckIDs.insert_or_assign(std::move(ackID), std::move(descr));
          logger::debug("AddDeadlineWatcher: sub: {} ackID: {} inserted:{}", _subscriptionName, it->first, inserted);
        },
        true);
  } else {
    // With initial object relationship design this situation is nearly impossible
    // Iterators are only owned by Executor and hence executor weak_ptr will always be valid
    // as longs as iterator is alive. Under this assumption we can consider this case as invariant violation.
    logger::error("_executor.lock() weak ref is nullptr. check objects ownership");
  }
}

void ModAckIterator::Done(const std::string &ackID, std::chrono::steady_clock::time_point rt, ModAckIterator::DoneAction action) noexcept(true) {
  const auto sec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - rt);
  _ackDist.Record(sec.count());

  if (auto exec = _executor.lock()) {
    exec->ExecuteUnderAckMutex(
        [this, action, &ackID]() {
          auto key = ackID;
          logger::debug("Done: ackID: {}, action: {}", ackID, action);

          switch (action) {
            case DoneAction::Ack:
              _pendingNackIDs.erase(key);
              _pendingAckIDs.emplace(std::move(key));
              break;

            case DoneAction::Nack:
              _pendingAckIDs.erase(key);
              _pendingNackIDs.emplace(std::move(key));
              break;
            default:
              assert(!"Done called with unknown action");
              return;
          }
        },
        true);
  } else {
    // With initial object relationship design this situation is nearly impossible
    // Iterators are only owned by Executor and hence executor weak_ptr will always be valid
    // as longs as iterator is alive. Under this assumption we can consider this case as invariant violation.
    logger::error("_executor.lock() weak ref is nullptr. check objects ownership");
  }
}

template <class IdsContainer, class FinishCallback>
void ModAckIterator::ExtendAckDeadlines(std::unique_ptr<Subscriber::Stub> &subscriber, IdsContainer &list, std::chrono::seconds newDeadline,
                                        FinishCallback Callback) {
  static_assert(std::is_invocable_v<FinishCallback, std::chrono::steady_clock::time_point>);

  logger::debug("ExtendAckDeadlines name:{}, cnt:{}", _subscriptionName, list.size());

  Empty empty;
  ModifyAckDeadlineRequest request;
  request.set_ack_deadline_seconds(static_cast<int>(newDeadline.count()));
  request.set_subscription(_subscriptionName);
  populate_ack_ids(*request.mutable_ack_ids(), list);

  auto [status, now] = retriable::make_call(subscriber, &Subscriber::Stub::ModifyAckDeadline, request, empty, _countPolicy, _timePolicy, _backoffPolicy);
  const auto next = now + newDeadline;

  switch (status.error_code()) {
    case grpc::StatusCode::OK:
      Callback(next);
      return;

    default:
      const auto err = "ModifyAckDeadline failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
      throw Exception(err, status.error_code());
  }  // end of switch
}

template <class IdsContainer>
std::chrono::milliseconds ModAckIterator::ModifyAckDeadlines(std::unique_ptr<Subscriber::Stub> &subscriber, IdsContainer &ids, std::chrono::seconds dl,
                                                             std::chrono::seconds gracePeriod) {
  if (ids.empty()) {
    return kSoManyMilliseconds;
  }

  AckIDPackSet toModify;

  if constexpr (std::is_same<AckIDSet, IdsContainer>::value) {
    while (!ids.empty()) {
      toModify.emplace(std::move(ids.extract(ids.begin()).value()), WatchDescrContainer::iterator());
    }

  } else if constexpr (std::is_same<WatchDescrContainer, IdsContainer>::value) {
    const auto pseudoNow = std::chrono::steady_clock::now();
    std::chrono::milliseconds sleepTime = kSoManyMilliseconds;

    give_me_the_keys(ids, toModify, [gracePeriod, pseudoNow, &sleepTime](const auto &descr) -> bool {
      const auto ms = descr.ExtendIn(pseudoNow);
      if (ms > gracePeriod) {
        sleepTime = std::min(sleepTime, ms);
        return false;
      }
      return true;
    });

    if (toModify.empty()) {
      // Half of the smallest interval we need to extend in
      return sleepTime / 2;
    }
  }

  logger::debug("ModifyAckDeadlines: {} for extension", toModify.size());
  while (!toModify.empty()) {
    decltype(toModify) toSend;
    split_request_ids(toModify, toSend, kMaxPayload);
    logger::debug("ModifyAckDeadlines: took {} for extension after split", toSend.size());
    ExtendAckDeadlines(subscriber, toSend, dl, [&toSend](auto next) {
      for (auto &it : toSend) {
        // nop
        std::get<1>(it)->second.UpdateNextAck(next);
      }
    });
  }

  return kSoManyMilliseconds;
}

std::chrono::milliseconds ModAckIterator::ProcessModAcks(std::unique_ptr<google::pubsub::v1::Subscriber::Stub> &subscriber) {
  decltype(_pendingAckIDs) pendingAcks;
  decltype(_pendingNackIDs) pendingNacks;

  if (auto exec = _executor.lock()) {
    exec->ExecuteUnderAckMutex(
        [this, &pendingAcks, &pendingNacks]() {
          pendingAcks.swap(_pendingAckIDs);
          pendingNacks.swap(_pendingNackIDs);
        },
        false);
  } else {
    // With initial object relationship design this situation is nearly impossible
    // Iterators are only owned by Executor and hence executor weak_ptr will always be valid
    // as longs as iterator is alive. Under this assumption we can consider this case as invariant violation.
    logger::error("_executor.lock() weak ref is nullptr. check objects ownership");
  }

  logger::debug("ProcessModAcks: Keep alives: {}, Acks:{} Nacks:{}", _keepAliveAckIDs.size(), pendingAcks.size(), pendingNacks.size());

  // Erase first since pendingAcks will be modified by SandAcks
  erase_keys(_keepAliveAckIDs, pendingAcks);
  // Modify deadline to 0s for removed acksIDs
  SendAcks(subscriber, pendingAcks);

  // Erase first since pendingNacks will be modified by ModifyAckDeadlines
  erase_keys(_keepAliveAckIDs, pendingNacks);
  // Set dl for nacks as 0s, grace is just large around a week
  ModifyAckDeadlines(subscriber, pendingNacks, 0s, kSoManySeconds);

  logger::debug("ProcessModAcks: Final keep alives: {}", _keepAliveAckIDs.size());

  return ModifyAckDeadlines(subscriber, _keepAliveAckIDs, AckDeadline(), _gracePeriod);
}

void ModAckIterator::SendAcks(std::unique_ptr<Subscriber::Stub> &subscriber, AckIDSet &ids) {
  if (ids.empty()) {
    return;
  }

  while (!ids.empty()) {
    AckIDSet toSend;
    split_request_ids(ids, toSend, kMaxPayload);
    AckMessages(subscriber, toSend);
  }
}

void ModAckIterator::SendNacks(std::unique_ptr<google::pubsub::v1::Subscriber::Stub> &subscriber, AckIDSet &ids) {
  if (ids.empty()) {
    return;
  }

  while (!ids.empty()) {
    AckIDSet toSend;
    split_request_ids(ids, toSend, kMaxPayload);
    ExtendAckDeadlines(subscriber, toSend, 0s, [](auto r) {});
  }
}

void ModAckIterator::AckMessages(std::unique_ptr<Subscriber::Stub> &subscriber, AckIDSet &list) {
  logger::debug("AckMessages name:{}, cnt:{}", _subscriptionName, list.size());

  Empty empty;
  AcknowledgeRequest request;
  request.set_subscription(_subscriptionName);
  populate_ack_ids(*request.mutable_ack_ids(), list);

  auto [status, _] = retriable::make_call(subscriber, &Subscriber::Stub::Acknowledge, request, empty, _countPolicy, _timePolicy, _backoffPolicy);

  switch (status.error_code()) {
    case grpc::StatusCode::OK:
      return;

    default:
      const auto err = "ModifyAckDeadline failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
      throw Exception(err, status.error_code());
  }  // end of switch
}

std::chrono::seconds ModAckIterator::AckDeadline() noexcept(true) {
  const auto dl = std::chrono::seconds(_ackDist.Percentile(0.99));
  return std::min(std::max(dl, kMinAckDeadline), kMaxAckDeadline);
}
