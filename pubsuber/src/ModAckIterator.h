#pragma once
#include <grpc++/grpc++.h>
#include <chrono>
#include <memory>
#include <tuple>
#include <unordered_set>
#include "Algorithms.h"
#include "Distribution.h"
#include "Pubsuber.h"
#include "Trimpl.h"
#include "google/pubsub/v1/pubsub.grpc.pb.h"

#include <spdlog/spdlog.h>

namespace pubsuber {
  struct Executor;

  class ModAckIterator {
  public:
    ModAckIterator(std::string subscriptionName, std::shared_ptr<Executor> executor);

    enum class DoneAction : int { Ack, Nack };

    // Called on ACK thread
    std::chrono::milliseconds ProcessModAcks(std::unique_ptr<google::pubsub::v1::Subscriber::Stub> &subscriber);

    // Thread safe
    void AddDeadlineWatcher(std::string &&ackID, std::chrono::steady_clock::time_point nextAck) noexcept(true);

    // Thread safe
    void Done(const std::string &ackID, std::chrono::steady_clock::time_point rt, DoneAction action) noexcept(true);

    // ACK thread only
    auto KeepAliveCount() const { return _keepAliveAckIDs.size(); }

    // Must be accessed under the ACK lock
    auto InputCount() const { return _inputAckIDs.size(); }

    // Must be accessed under the ACK lock
    void MergeInput() {
      _keepAliveAckIDs.merge(_inputAckIDs);
      _inputAckIDs.clear();
    }

  private:
    // Must be called on ACK thread only
    template <class IdsContainer, class FinishCallback>
    void ExtendAckDeadlines(std::unique_ptr<google::pubsub::v1::Subscriber::Stub> &subscriber, IdsContainer &list, std::chrono::seconds newDeadline,
                            FinishCallback callback);

    // Must be called on ACK thread only
    template <class IdsContainer>
    std::chrono::milliseconds ModifyAckDeadlines(std::unique_ptr<google::pubsub::v1::Subscriber::Stub> &subscriber, IdsContainer &ids, std::chrono::seconds dl,
                                                 std::chrono::seconds gracePeriod);
    // Must be called on ACK thread only
    void SendAcks(std::unique_ptr<google::pubsub::v1::Subscriber::Stub> &subscriber, AckIDSet &ids);

    // Must be called on ACK thread only
    void SendNacks(std::unique_ptr<google::pubsub::v1::Subscriber::Stub> &subscriber, AckIDSet &ids);

    // Must be called on ACK thread only
    void AckMessages(std::unique_ptr<google::pubsub::v1::Subscriber::Stub> &subscriber, AckIDSet &list);
    // Thread safe
    std::chrono::seconds AckDeadline() noexcept(true);

  private:
    const std::string _subscriptionName;
    const std::chrono::seconds _gracePeriod;
    Distribution<kMaxAckDeadline.count() + 1> _ackDist;
    std::weak_ptr<Executor> _executor;

    // managed under the lock on any thread
    WatchDescrContainer _inputAckIDs;
    // managed under the lock on any thread
    AckIDSet _pendingAckIDs;
    // managed under the lock on any thread
    AckIDSet _pendingNackIDs;
    // managed on ack thread only
    WatchDescrContainer _keepAliveAckIDs;
  };

  using ModAckIteratorPtr = std::shared_ptr<ModAckIterator>;
}  // namespace pubsuber
