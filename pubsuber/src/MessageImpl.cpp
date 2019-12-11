#include "MessageImpl.h"

#include <cassert>
#include "google/pubsub/v1/pubsub.grpc.pb.h"
#include "google/pubsub/v1/pubsub.pb.h"

using google::protobuf::Empty;
using google::pubsub::v1::DeleteSubscriptionRequest;
using google::pubsub::v1::DeleteTopicRequest;
using google::pubsub::v1::GetSubscriptionRequest;
using google::pubsub::v1::GetTopicRequest;
using google::pubsub::v1::Publisher;
using google::pubsub::v1::PublishRequest;
using google::pubsub::v1::PublishResponse;
using google::pubsub::v1::PubsubMessage;
using google::pubsub::v1::Subscriber;
using grpc::ClientContext;

using namespace pubsuber;

//**********************************************************************************************************************
MessageImpl::MessageImpl(std::weak_ptr<ModAckIterator> iterator, const std::string &subName, const std::string &ackID,
                         std::unique_ptr<::google::pubsub::v1::PubsubMessage> &&msg, std::chrono::steady_clock::time_point rt)
: _iterator(iterator)
, _subscriptionName(subName)
, _ackID(ackID)
, _msg(std::move(msg))
, _receiveTime(rt) {
  for (auto &it : _msg->attributes()) {
    _attributes.insert({it.first, it.second});
  }
}

MessageImpl::~MessageImpl() {
  // We need this to clean up in iterator
  Nack();
}

const ByteBuffer &MessageImpl::Payload() const { return _msg->data(); }

const std::map<std::string, std::string> &MessageImpl::Attributes() const noexcept(true) { return _attributes; }

const std::string &MessageImpl::SubscriptionName() const noexcept(true) { return _subscriptionName; }

void MessageImpl::Ack() {
  if (auto it = _iterator.lock()) {
    it->Done(_ackID, _receiveTime, ModAckIterator::DoneAction::Ack);
    _iterator.reset();
  }
}

void MessageImpl::Nack() {
  if (auto it = _iterator.lock()) {
    it->Done(_ackID, _receiveTime, ModAckIterator::DoneAction::Nack);
    _iterator.reset();
  }
}
