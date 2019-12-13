#pragma once
#include <grpc++/grpc++.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <list>
#include <memory>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include "ModAckIterator.h"
#include "pubsuber/Pubsuber.h"
#include "SubscriptionPullIterator.h"
#include "Trimpl.h"
#include "google/pubsub/v1/pubsub.grpc.pb.h"

#include <spdlog/spdlog.h>

namespace pubsuber {
  const auto kSoManyMilliseconds = 1000000000ms;
  const auto kSoManySeconds = 1000000000s;

  //**********************************************************************************************************************
  /**
   * Implementation of this class is thread safe unless otherwise noted
   */
  struct Executor : public std::enable_shared_from_this<Executor> {
    using SubscriptionName = std::string;
    using IteratorMap = std::unordered_map<SubscriptionName, ModAckIteratorPtr>;
    using PullIteratorMap = std::unordered_map<SubscriptionName, SubscriptionPullIteratorPtr>;
    using Callback = Subscription::Callback;

    template <class IteratorContainerType>
    struct ThreadDataBlock {
      template <class Block>
      void ExecuteUnderMutex(Block b, bool notify) {
        if (std::unique_lock l(_condMutex); true) {
          b();
        }
        if (notify) {
          _cond.notify_all();
        }
      }
      void Stop();
      // Called under lock inside
      void ClearInput();
      void ClearRemoved();
      // Called without lock inside
      void MergeInputToActive();
      // Called under lock inside
      void RemoveFromActive();

      template <typename Lambda>
      void ForEachActive(Lambda b) {
        for (auto &it : _activeIterators) {
          b(it);
        }
      }

      // Called under lock inside with notify
      template <class IteratorType>
      void AddIterator(const std::string &subscription, IteratorType &&it) {
        if (std::unique_lock l(_condMutex); true) {
          _inputIterators.insert_or_assign(subscription, std::forward<IteratorType>(it));
        }
        _cond.notify_one();
      }

      // Called under lock inside with notify
      void RemoveIterator(const std::string &subscription) {
        if (std::unique_lock l(_condMutex); true) {
          _removedIterators.push_back(subscription);
        }
        _cond.notify_one();
      }

      std::unique_ptr<google::pubsub::v1::Subscriber::Stub> _subscriber;
      std::atomic<bool> _needStop{false};
      std::mutex _condMutex;
      std::condition_variable _cond;
      // managed under the lock on any thread
      IteratorContainerType _inputIterators;
      // managed under the lock on any thread
      std::list<SubscriptionName> _removedIterators;
      // Managed on current thread only
      IteratorContainerType _activeIterators;
      // Must be certainly last one, i.e. after all the fields that can be used in the thread func
      // So the rule of thumb: it is last one.
      std::thread _thread;
    };

  private:
    Executor(const Executor &) = delete;
    Executor(Executor &&) = delete;
    Executor &operator=(const Executor &) = delete;
    Executor &operator=(Executor &&) = delete;

    // PULL thread
    void PullThreadFunc();
    // ACK thread
    void AckThreadFunc();

    // Must be called on PULL thread only
    std::chrono::milliseconds Pull();

    // Must be called on ACK thread only
    std::chrono::milliseconds ProcessModAcks();

  public:
    Executor(ClientOptions &&opts);

    // Thread safe
    void StopThreads();

    // Thread safe
    void AddIterator(const std::string &fullSubscriptionName, Callback &cb);
    // Thread safe
    void RemoveIterator(const std::string &fullSubscriptionName) noexcept(true);

    // Thread safe
    void AddMetricSink(std::shared_ptr<MetricSink> sink);
    // Thread safe
    void RemoveMetricSink();

    // Thread safe
    template <class Block>
    void ExecuteUnderAckMutex(Block b, bool notify) {
      _ackThread.ExecuteUnderMutex(std::forward<Block>(b), notify);
    }

    // Must be called from ACK thread
    void ReportKeepAliveMetric(size_t count);

  public:
    ClientOptions _options;
    Trimpl _tr;

    // Ack thread data
    ThreadDataBlock<IteratorMap> _ackThread;

    // Pull thread data
    ThreadDataBlock<PullIteratorMap> _pullThread;

    // Be aware under which lock this field is managed
    // WARNING: Could only be used from one thread so far, ACK
    std::shared_ptr<MetricSink> _metricsSinks;
  };

}  // namespace pubsuber
