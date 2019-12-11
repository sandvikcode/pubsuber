#pragma once

#include "ModAckIterator.h"
#include "google/pubsub/v1/pubsub.pb.h"

namespace pubsuber {

  // Implementation is thread safe
  class MessageImpl : public Message {
    MessageImpl() = delete;

  public:
    MessageImpl(std::weak_ptr<ModAckIterator> iterator, const std::string& subName, const std::string& ackID,
                std::unique_ptr<::google::pubsub::v1::PubsubMessage>&& msg, std::chrono::steady_clock::time_point rt);
    virtual ~MessageImpl();

  private:
    virtual const ByteBuffer& Payload() const override;
    virtual const std::map<std::string, std::string>& Attributes() const noexcept(true) override;
    virtual const std::string& SubscriptionName() const noexcept(true) override;
    virtual void Ack() override;
    virtual void Nack() override;

  private:
    std::weak_ptr<ModAckIterator> _iterator;
    const std::string _subscriptionName;
    const std::string _ackID;
    std::map<std::string, std::string> _attributes;
    std::unique_ptr<::google::pubsub::v1::PubsubMessage> _msg;
    const std::chrono::steady_clock::time_point _receiveTime;
  };

}  // namespace pubsuber
