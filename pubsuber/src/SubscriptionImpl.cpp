#include "SubscriptionImpl.h"

#include <cassert>
#include "google/pubsub/v1/pubsub.grpc.pb.h"
#include "google/pubsub/v1/pubsub.pb.h"

using google::protobuf::Empty;
using google::pubsub::v1::DeleteSubscriptionRequest;
using google::pubsub::v1::GetSubscriptionRequest;
using google::pubsub::v1::Subscriber;
using grpc::ClientContext;

using namespace pubsuber;

//**********************************************************************************************************************
SubscriptionImpl::SubscriptionImpl(std::weak_ptr<Executor> executor, const std::string &id, const std::string &name)
: EntityBase(id, name)
, _executor(executor) {
  // Copy transport
  if (auto exec = _executor.lock()) {
    _tr = exec->_tr;
  } else {
    throw Exception("Executor object is not set");
  }
}

SubscriptionImpl::~SubscriptionImpl() {
  auto exec = _executor.lock();
  if (exec && _receiverActive) {
    exec->RemoveIterator(_fullName);
    _receiverActive = false;
  }
}

bool SubscriptionImpl::Exists() {
  using google::pubsub::v1::Subscription;

  auto timeout = kDefaultRPCTimeout;
  for (auto i = 0; i < kDefultRetryAttempts; ++i) {
    auto subscriber = Subscriber::NewStub(_tr._channel);
    _tr.EnsureConnected();
    ClientContext ctx;

    GetSubscriptionRequest request;
    request.set_subscription(_fullName);
    Subscription subscription;

    set_deadline(ctx, timeout);
    switch (auto status = subscriber->GetSubscription(&ctx, request, &subscription); status.error_code()) {
      case grpc::StatusCode::NOT_FOUND:
        return false;

      case grpc::StatusCode::UNAVAILABLE:
        continue;

      case grpc::StatusCode::DEADLINE_EXCEEDED:
        // Retry once again
        timeout *= 2;
        continue;

      default:
        if (!status.ok()) {
          const auto err = "GetSubscription request failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
          throw Exception(err);
        }

        // Save ack deadline for subscription
        _ackDeadline = std::chrono::seconds(subscription.ack_deadline_seconds());
        return true;
    }
  }  // end of for

  const auto err = "GetSubscription request failed " + std::to_string(kDefultRetryAttempts) + " times with error";
  throw Exception(err);
}

void SubscriptionImpl::Create(Subscription::CreationOptions &&options) {
  using google::pubsub::v1::Subscription;

  auto timeout = kDefaultRPCTimeout;
  for (auto i = 0; i < kDefultRetryAttempts; ++i) {
    auto subscriber = Subscriber::NewStub(_tr._channel);
    _tr.EnsureConnected();
    ClientContext ctx;

    Subscription request;
    request.set_name(_fullName);
    request.set_topic(options._fullTopicName);
    request.set_ack_deadline_seconds(options._ackDeadline.count());
    Subscription subscription;

    // Creating subscription could take too long sometimes
    set_deadline(ctx, timeout);
    switch (auto status = subscriber->CreateSubscription(&ctx, request, &subscription); status.error_code()) {
      case grpc::StatusCode::UNAVAILABLE:
        continue;

      case grpc::StatusCode::DEADLINE_EXCEEDED:
        // Retry once again
        timeout *= 2;
        continue;

      default:
        if (!status.ok()) {
          const auto err = "CreateSubscription request failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
          throw Exception(err);
        }
        _ackDeadline = std::chrono::seconds(subscription.ack_deadline_seconds());
        return;
    }
  }  // end of for

  const auto err = "CreateSubscription request failed " + std::to_string(kDefultRetryAttempts) + " times with error";
  throw Exception(err);
}

void SubscriptionImpl::Delete() {
  auto timeout = kDefaultRPCTimeout;
  for (auto i = 0; i < kDefultRetryAttempts; ++i) {
    auto subscriber = Subscriber::NewStub(_tr._channel);
    _tr.EnsureConnected();
    ClientContext ctx;

    DeleteSubscriptionRequest request;
    request.set_subscription(_fullName);
    Empty response;

    set_deadline(ctx, timeout);
    switch (auto status = subscriber->DeleteSubscription(&ctx, request, &response); status.error_code()) {
      // If subscription is not found then it is fine and not an error
      case grpc::StatusCode::NOT_FOUND:
        return;

      case grpc::StatusCode::UNAVAILABLE:
        continue;

      case grpc::StatusCode::DEADLINE_EXCEEDED:
        // Retry once again
        timeout *= 2;
        continue;

      default:
        if (!status.ok()) {
          const auto err = "DeleteSubscription request failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
          throw Exception(err);
        }
        return;
    }
  }  // end of for

  const auto err = "DeleteSubscription request failed " + std::to_string(kDefultRetryAttempts) + " times with error";
  throw Exception(err);
}

void SubscriptionImpl::Receive(Callback cb) {
  // Check if subscription is exists.
  if (!Exists()) {
    throw Exception("Subscription " + _fullName + " does not exit");
  }

  if (_receiverActive) {
    throw Exception("Subscription object supports only one receiver");
  }

  if (auto exec = _executor.lock()) {
    exec->AddIterator(_fullName, cb);
    // Set flag after the call
    _receiverActive = true;
  } else {
    throw Exception("Executor object is not set");
  }
}

void SubscriptionImpl::Stop() {
  auto exec = _executor.lock();
  if (exec && _receiverActive) {
    exec->RemoveIterator(_fullName);
    _receiverActive = false;
  } else {
    throw Exception("Executor object is not set");
  }
}
