#include "TopicImpl.h"

#include "pubsuber/Pubsuber.h"

#include "Retriable.h"

using google::protobuf::Empty;
using google::pubsub::v1::DeleteTopicRequest;
using google::pubsub::v1::GetTopicRequest;
using google::pubsub::v1::Publisher;
using google::pubsub::v1::PublishRequest;
using google::pubsub::v1::PublishResponse;
using google::pubsub::v1::PubsubMessage;

using namespace pubsuber;

namespace {
  void copy_attributes(PubsubMessage &msg, const std::map<std::string, std::string> &attributes) {
    auto attrPtr = msg.mutable_attributes();
    for (auto &it : attributes) {
      if (it.first.empty()) {
        throw Exception("Pubsuber: attribute key must not be empty");
      }
      attrPtr->insert({it.first, it.second});
    }
  }

}  // namespace

//**********************************************************************************************************************
TopicImpl::TopicImpl(Trimpl &&impl, const std::string &id, const std::string &topic, RetryCountPolicy countPolicy, MaxRetryTimePolicy timePolicy,
                     ExponentialBackoffPolicy backoffPolicy)
: EntityBase(id, topic)
, _tr(std::move(impl))
, _countPolicy(countPolicy)
, _timePolicy(timePolicy)
, _backoffPolicy(backoffPolicy) {}

bool TopicImpl::Exists() {
  GetTopicRequest request;
  request.set_topic(_fullName);
  google::pubsub::v1::Topic topic;

  auto publisher = Publisher::NewStub(_tr._channel);
  _tr.EnsureConnected();
  auto [status, _] = retriable::make_call(publisher, &Publisher::Stub::GetTopic, request, topic, _countPolicy, _timePolicy, _backoffPolicy);

  switch (status.error_code()) {
    case grpc::StatusCode::NOT_FOUND:
      return false;

    default:
      if (!status.ok()) {
        const auto err = "Pubsuber: GetTopicRequest request failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
        throw Exception(err, status.error_code());
      }
      return true;
  }
}

void TopicImpl::Create() {
  ::google::pubsub::v1::Topic request;
  ::google::pubsub::v1::Topic response;
  request.set_name(_fullName);

  auto publisher = Publisher::NewStub(_tr._channel);
  _tr.EnsureConnected();
  auto [status, _] = retriable::make_call(publisher, &Publisher::Stub::CreateTopic, request, response, _countPolicy, _timePolicy, _backoffPolicy);

  if (!status.ok()) {
    const auto err = "Pubsuber: CreateTopic request failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
    throw Exception(err, status.error_code());
  }
}

void TopicImpl::Delete() {
  DeleteTopicRequest request;
  Empty response;
  request.set_topic(_fullName);

  auto publisher = Publisher::NewStub(_tr._channel);
  _tr.EnsureConnected();
  auto [status, _] = retriable::make_call(publisher, &Publisher::Stub::DeleteTopic, request, response, _countPolicy, _timePolicy, _backoffPolicy);

  switch (status.error_code()) {
    // If topic is not found then it is fine and not an error
    case grpc::StatusCode::NOT_FOUND:
      return;

    default:
      if (!status.ok()) {
        const auto err = "Pubsuber: DeleteTopic request failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
        throw Exception(err, status.error_code());
      }
      // its okay
      return;
  }
}

std::string TopicImpl::Publish(const ByteBuffer &buffer, const std::map<std::string, std::string> &attributes) {
  PublishRequest request;
  request.set_topic(_fullName);
  PubsubMessage &msg = *request.add_messages();

  msg.set_data(buffer.data(), buffer.size());
  copy_attributes(msg, attributes);

  PublishResponse response;
  auto publisher = Publisher::NewStub(_tr._channel);
  _tr.EnsureConnected();
  auto [status, _] = retriable::make_call(publisher, &Publisher::Stub::Publish, request, response, _countPolicy, _timePolicy, _backoffPolicy);

  switch (status.error_code()) {
    default:
      if (!status.ok()) {
        const auto err = "Pubsuber: Publish request failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
        throw Exception(err, status.error_code());
      }

      if (response.message_ids_size() == 0) {
        throw Exception("Pubsuber: received response.message_ids_size() is 0, no msg id to return to caller.");
      }
      return response.message_ids(0);
  }
}
