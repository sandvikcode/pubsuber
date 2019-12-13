#include "TopicImpl.h"
#include "pubsuber/Pubsuber.h"

#include "google/pubsub/v1/pubsub.grpc.pb.h"
#include "google/pubsub/v1/pubsub.pb.h"

using google::protobuf::Empty;
using google::pubsub::v1::DeleteTopicRequest;
using google::pubsub::v1::GetTopicRequest;
using google::pubsub::v1::Publisher;
using google::pubsub::v1::PublishRequest;
using google::pubsub::v1::PublishResponse;
using google::pubsub::v1::PubsubMessage;
using grpc::ClientContext;

using namespace pubsuber;

namespace {
  void copy_attributes(PubsubMessage &msg, const std::map<std::string, std::string> &attributes) {
    auto attrPtr = msg.mutable_attributes();
    for (auto &it : attributes) {
      if (it.first.empty()) {
        throw Exception("attribute key must not be empty");
      }
      attrPtr->insert({it.first, it.second});
    }
  }
}  // namespace

//**********************************************************************************************************************
TopicImpl::TopicImpl(Trimpl &&impl, const std::string &id, const std::string &topic)
: EntityBase(id, topic)
, _tr(std::move(impl)) {}

bool TopicImpl::Exists() {
  using google::pubsub::v1::Topic;

  auto timeout = kDefaultRPCTimeout;
  for (auto i = 0; i < kDefultRetryAttempts; ++i) {
    auto publisher = Publisher::NewStub(_tr._channel);
    _tr.EnsureConnected();
    ClientContext ctx;

    GetTopicRequest request;
    request.set_topic(_fullName);
    Topic topic;

    set_deadline(ctx, timeout);
    switch (auto status = publisher->GetTopic(&ctx, request, &topic); status.error_code()) {
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
          const auto err = "GetTopicRequest request failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
          throw Exception(err);
        }
        return true;
    }
  }  // end of for

  const auto err = "GetTopicRequest request failed " + std::to_string(kDefultRetryAttempts) + " times with error";
  throw Exception(err);
}

void TopicImpl::Create() {
  using google::pubsub::v1::Topic;

  auto timeout = kDefaultRPCTimeout;
  for (auto i = 0; i < kDefultRetryAttempts; ++i) {
    auto publisher = Publisher::NewStub(_tr._channel);
    _tr.EnsureConnected();
    ClientContext ctx;

    Topic request;
    Topic response;
    request.set_name(_fullName);

    set_deadline(ctx, timeout);
    switch (auto status = publisher->CreateTopic(&ctx, request, &response); status.error_code()) {
      case grpc::StatusCode::UNAVAILABLE:
        continue;

      case grpc::StatusCode::DEADLINE_EXCEEDED:
        // Retry once again
        timeout *= 2;
        continue;

      default:
        if (!status.ok()) {
          const auto err = "CreateTopic request failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
          throw Exception(err);
        }
        // its okay
        return;
    }
  }  // end of for

  const auto err = "CreateTopic request failed " + std::to_string(kDefultRetryAttempts) + " times with error";
  throw Exception(err);
}

void TopicImpl::Delete() {
  using google::pubsub::v1::Topic;

  auto timeout = kDefaultRPCTimeout;
  for (auto i = 0; i < kDefultRetryAttempts; ++i) {
    auto publisher = Publisher::NewStub(_tr._channel);
    _tr.EnsureConnected();
    ClientContext ctx;

    DeleteTopicRequest request;
    Empty response;
    request.set_topic(_fullName);

    set_deadline(ctx, timeout);
    switch (auto status = publisher->DeleteTopic(&ctx, request, &response); status.error_code()) {
      // If topic is not found then it is fine and not an error
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
          const auto err = "DeleteTopic request failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
          throw Exception(err);
        }
        // its okay
        return;
    }
  }  // end of for

  const auto err = "DeleteTopic request failed " + std::to_string(kDefultRetryAttempts) + " times with error";
  throw Exception(err);
}

std::string TopicImpl::Publish(const ByteBuffer &buffer, const std::map<std::string, std::string> &attributes) {
  auto timeout = kDefaultRPCTimeout;
  for (auto i = 0; i < kDefultRetryAttempts; ++i) {
    auto publisher = Publisher::NewStub(_tr._channel);
    _tr.EnsureConnected();
    ClientContext ctx;

    PublishRequest request;
    request.set_topic(_fullName);
    PubsubMessage &msg = *request.add_messages();

    msg.set_data(buffer.data(), buffer.size());
    copy_attributes(msg, attributes);

    PublishResponse response;

    set_deadline(ctx, timeout);
    switch (auto status = publisher->Publish(&ctx, request, &response); status.error_code()) {
      case grpc::StatusCode::UNAVAILABLE:
        continue;

      case grpc::StatusCode::DEADLINE_EXCEEDED:
        // Retry once again
        timeout *= 2;
        continue;

      default:
        if (!status.ok()) {
          const auto err = "Publish request failed with error " + std::to_string(status.error_code()) + ": " + status.error_message();
          throw Exception(err);
        }

        if (response.message_ids_size() == 0) {
          throw Exception("received response.message_ids_size() is 0, no msg id to return to caller.");
        }
        return response.message_ids(0);
    }
  }  // end of for

  const auto err = "Publish request failed " + std::to_string(kDefultRetryAttempts) + " times with error";
  throw Exception(err);
}
