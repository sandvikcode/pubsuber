#include "Executor.h"

#include <algorithm>
#include <cassert>
#include <utility>
#include "google/pubsub/v1/pubsub.grpc.pb.h"
#include "google/pubsub/v1/pubsub.pb.h"

using namespace pubsuber;
using namespace std::chrono;
using google::pubsub::v1::Subscriber;

namespace {
  const auto kLowPullLimit = 250000us;
  const auto kMaxSendRecvBytes = 20 * 1024 * 1024;  // 20M

}  // namespace

//**********************************************************************************************************************
template <class IteratorContainerType>
void Executor::ThreadDataBlock<IteratorContainerType>::Stop() {
  _needStop = true;
  _cond.notify_all();
  _thread.join();
}

template <class IteratorContainerType>
void Executor::ThreadDataBlock<IteratorContainerType>::MergeInputToActive() {
  _activeIterators.merge(_inputIterators);
  _inputIterators.clear();
}

template <class IteratorContainerType>
void Executor::ThreadDataBlock<IteratorContainerType>::ClearInput() {
  ExecuteUnderMutex([this]() { _inputIterators.clear(); }, false);
}

template <class IteratorContainerType>
void Executor::ThreadDataBlock<IteratorContainerType>::ClearRemoved() {
  ExecuteUnderMutex([this]() { _removedIterators.clear(); }, false);
}

template <class IteratorContainerType>
void Executor::ThreadDataBlock<IteratorContainerType>::RemoveFromActive() {
  ExecuteUnderMutex(
      [this]() {
        for (const auto &key : _removedIterators) {
          _activeIterators.erase(key);
        }
        _removedIterators.clear();
      },
      false);
}

//**********************************************************************************************************************
Executor::Executor(ClientOptions &&opts)
: _options(std::move(opts)) {
  SetupLogger();

  if (!_options.SecureChannel()) {
    _tr._creds = grpc::InsecureChannelCredentials();
  }

  // We must first create transport
  grpc::ChannelArguments arg;
  arg.SetMaxReceiveMessageSize(kMaxSendRecvBytes);
  arg.SetMaxSendMessageSize(kMaxSendRecvBytes);
  _tr._channel = grpc::CreateCustomChannel(_options.Host(), _tr._creds, arg);

  // then start threads that uses this transport
  _pullThread._thread = std::thread(&Executor::PullThreadFunc, this);
  _ackThread._thread = std::thread(&Executor::AckThreadFunc, this);
}

void Executor::ApplyPolicies(const RetryCountPolicy &countPolicy, const MaxRetryTimePolicy &timePolicy, ExponentialBackoffPolicy &backoffPolicy) {
  _countPolicy = countPolicy;
  _timePolicy = timePolicy;
  _backoffPolicy = backoffPolicy;
}

void Executor::AddIterator(const std::string &fullSubscriptionName, Callback &callback) {
  logger::debug("AddIterator: {}", fullSubscriptionName);

  if (!callback) {
    throw Exception("callback must be set");
  }
  if (fullSubscriptionName.empty()) {
    throw Exception("subscription name must be set");
  }

  auto ackIterator = std::make_shared<ModAckIterator>(fullSubscriptionName, shared_from_this(), _countPolicy, _timePolicy, _backoffPolicy);
  auto pullIterator = std::make_shared<SubscriptionPullIterator>(fullSubscriptionName, _options.MaxPrefetch(), callback, ackIterator);

  _pullThread.AddIterator(fullSubscriptionName, std::move(pullIterator));
  _ackThread.AddIterator(fullSubscriptionName, std::move(ackIterator));
}

void Executor::RemoveIterator(const std::string &fullSubscriptionName) noexcept(true) {
  logger::debug("RemoveIterator: {}", fullSubscriptionName);

  _pullThread.RemoveIterator(fullSubscriptionName);
  _ackThread.RemoveIterator(fullSubscriptionName);
}

void Executor::AddMetricSink(std::shared_ptr<MetricSink> sink) {
  _ackThread.ExecuteUnderMutex([this, &sink]() { _metricsSinks = sink; }, false);
}

void Executor::RemoveMetricSink() {
  _ackThread.ExecuteUnderMutex([this]() { _metricsSinks.reset(); }, false);
}

void Executor::ReportKeepAliveMetric(size_t count) {
  decltype(_metricsSinks) sink;

  _ackThread.ExecuteUnderMutex([this, &sink]() { sink = _metricsSinks; }, false);

  if (sink) {
    sink->OnKeepAliveQueueSize(count);
  }
}

void Executor::StopThreads() {
  _pullThread.Stop();
  _ackThread.Stop();
  // No need to keep them as threads are stopped.
  _pullThread.ClearInput();
  // Since thread is ctoped can be cleared w/o lock
  _pullThread._activeIterators.clear();
  _pullThread.ClearRemoved();

  _ackThread.ClearInput();
  // Since thread is ctoped can be cleared w/o lock
  _ackThread._activeIterators.clear();
  _ackThread.ClearRemoved();
}

void Executor::PullThreadFunc() {
  _pullThread._subscriber = Subscriber::NewStub(_tr._channel);

  while (!_pullThread._needStop) {
    // Unconditionally process removed iterators first
    _pullThread.RemoveFromActive();

    const bool noActive = (_pullThread._activeIterators.size() == 0);

    if (std::unique_lock l(_pullThread._condMutex); true) {
      // Wait only if input is empty, save one runloop
      if (_pullThread._inputIterators.size() == 0 && noActive) {
        const auto status = _pullThread._cond.wait_for(l, 100ms);
        _tr.EnsureConnected();
        if (status == std::cv_status::timeout || _pullThread._needStop) {
          continue;
        }
      }

      _pullThread.MergeInputToActive();
    }

    // Pull each subscription once and deliver received message
    try {
      auto sleep = Pull();
      if (std::unique_lock l(_pullThread._condMutex); sleep != kSoManyMilliseconds) {
        _pullThread._cond.wait_for(l, sleep);
      }
    } catch (Exception &e) {
      logger::error("Exception in Pull: {}", e.what());
    }
  }
}

std::chrono::milliseconds Executor::Pull() {
  _tr.EnsureConnected();

  auto start = std::chrono::high_resolution_clock::now();

  _pullThread.ForEachActive([this](auto &pair) {
    auto &[name, iterator] = pair;
    iterator->Pull(_pullThread._subscriber);
  });  // end of for_each

  std::chrono::microseconds us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
  logger::trace("Pull took: {}us", us.count());
  if (us > kLowPullLimit) {
    // No need to slowdown the thread
    return kSoManyMilliseconds;
  }
  // wait rest of the time on condition
  return std::chrono::duration_cast<std::chrono::milliseconds>(kLowPullLimit - us);
}

//*********************************************************************
void Executor::AckThreadFunc() {
  _ackThread._subscriber = Subscriber::NewStub(_tr._channel);
  std::chrono::milliseconds sleep = kSoManyMilliseconds;
  bool needSleep = false;

  while (!_ackThread._needStop) {
    if (sleep == kSoManyMilliseconds) {
      sleep = 500ms;
    }

    // Unconditionally process removed iterators first
    _ackThread.RemoveFromActive();

    // Wait condition
    size_t keepAliveCount = 0;
    const bool noKeepAlives = [this, &keepAliveCount]() {
      _ackThread.ForEachActive([&keepAliveCount](auto &pair) {
        const auto &[name, iterator] = pair;
        keepAliveCount += iterator->KeepAliveCount();
      });
      logger::trace("KeepAlivesCount: {}", keepAliveCount);
      return (keepAliveCount == 0);
    }();

    ReportKeepAliveMetric(keepAliveCount);

    // This must be executed under the lock since it accesses shared data
    const auto getInputCount = [this]() {
      uint32_t count = 0;
      _ackThread.ForEachActive([&count](auto &pair) {
        const auto &[name, iterator] = pair;
        count += iterator->InputCount();
      });
      logger::trace("getInputCount: {}", count);
      return count;
    };

    if (std::unique_lock l(_ackThread._condMutex); true) {
      // Wait only if input is empty, save one runloop
      if (needSleep || (getInputCount() == 0 && noKeepAlives)) {
        logger::debug("ACK sleep for: {}ms", sleep.count());
        const auto status = _ackThread._cond.wait_for(l, sleep);
        if ((!needSleep && status == std::cv_status::timeout) || _ackThread._needStop) {
          continue;
        }
        logger::trace("sleep status: {}", status);
      }

      _ackThread.MergeInputToActive();

      _ackThread.ForEachActive([](auto &pair) {
        auto &[name, iterator] = pair;
        // This acceses the data that is supposed to be accessed under _ackThread mutex
        iterator->MergeInput();
      });
    }  // end of ACK lock

    try {
      // reset flag and time before call as exception may happen
      needSleep = false;
      sleep = kSoManyMilliseconds;

      _tr.EnsureConnected();
      sleep = ProcessModAcks();
      needSleep = sleep != kSoManyMilliseconds;

    } catch (Exception &e) {
      logger::error("Exception in ProcessModAcks: {}", e.what());
    }
  }
}

std::chrono::milliseconds Executor::ProcessModAcks() {
  std::chrono::milliseconds sleep = kSoManyMilliseconds;

  _ackThread.ForEachActive([this, &sleep](auto &pair) {
    auto &[name, iterator] = pair;
    const auto ms = iterator->ProcessModAcks(_ackThread._subscriber);
    sleep = std::min(ms, sleep);
  });  // end of for_each

  return sleep;
}

void Executor::SetupLogger() { logger::Setup(); }
