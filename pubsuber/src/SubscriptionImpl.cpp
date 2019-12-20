#include "SubscriptionImpl.h"
#include "Retriable.h"

using google::protobuf::Empty;
using google::pubsub::v1::DeleteSubscriptionRequest;
using google::pubsub::v1::GetSubscriptionRequest;
using google::pubsub::v1::Subscriber;
using grpc::ClientContext;

using namespace pubsuber;

//**********************************************************************************************************************
SubscriptionImpl::SubscriptionImpl(std::weak_ptr<Executor> executor, const std::string &id, const std::string &name, RetryCountPolicy countPolicy,
                                   MaxRetryTimePolicy timePolicy, ExponentialBackoffPolicy backoffPolicy)
: EntityBase(id, name)
, _executor(executor)
, _countPolicy(countPolicy)
, _timePolicy(timePolicy)
, _backoffPolicy(backoffPolicy) {
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
  GetSubscriptionRequest request;
  ::google::pubsub::v1::Subscription subscription;
  request.set_subscription(_fullName);
  auto subscriber = Subscriber::NewStub(_tr._channel);
  _tr.EnsureConnected();

  auto [status, _] = retriable::make_call(subscriber, &Subscriber::Stub::GetSubscription, request, subscription, _countPolicy, _timePolicy, _backoffPolicy);

  switch (status.error_code()) {
    case grpc::StatusCode::NOT_FOUND:
      return false;

    default:
      if (!status.ok()) {
        const auto err = "GetSubscription request failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
        throw Exception(err, status.error_code());
      }

      // Save ack deadline for subscription
      _ackDeadline = std::chrono::seconds(subscription.ack_deadline_seconds());
      return true;
  }
}

void SubscriptionImpl::Create(Subscription::CreationOptions &&options) {
  ::google::pubsub::v1::Subscription request;
  ::google::pubsub::v1::Subscription subscription;
  request.set_name(_fullName);
  request.set_topic(options._fullTopicName);
  request.set_ack_deadline_seconds(options._ackDeadline.count());

  auto subscriber = Subscriber::NewStub(_tr._channel);
  _tr.EnsureConnected();

  auto [status, _] = retriable::make_call(subscriber, &Subscriber::Stub::CreateSubscription, request, subscription, _countPolicy, _timePolicy, _backoffPolicy);

  switch (status.error_code()) {
    default:
      if (!status.ok()) {
        const auto err = "CreateSubscription request failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
        throw Exception(err, status.error_code());
      }
      _ackDeadline = std::chrono::seconds(subscription.ack_deadline_seconds());
      return;
  }
}

void SubscriptionImpl::Delete() {
  DeleteSubscriptionRequest request;
  request.set_subscription(_fullName);
  Empty response;
  auto subscriber = Subscriber::NewStub(_tr._channel);
  _tr.EnsureConnected();

  auto [status, _] = retriable::make_call(subscriber, &Subscriber::Stub::DeleteSubscription, request, response, _countPolicy, _timePolicy, _backoffPolicy);

  switch (status.error_code()) {
    // If subscription is not found then it is fine and not an error
    case grpc::StatusCode::NOT_FOUND:
      return;

    default:
      if (!status.ok()) {
        const auto err = "DeleteSubscription request failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
        throw Exception(err, status.error_code());
      }
      return;
  }
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
