#pragma once

#include "Pubsuber.h"
#include "gmock/gmock-function-mocker.h"
#include "gmock/gmock.h"

namespace pubsuber {

  class MessageMock : public pubsuber::Message {
  public:
    MOCK_METHOD(const ByteBuffer &, Payload, (), (const, override));
    MOCK_METHOD((const std::map<std::string, std::string> &), Attributes, (), (const, noexcept, override));
    MOCK_METHOD(const std::string &, SubscriptionName, (), (const, noexcept, override));
    MOCK_METHOD(void, Ack, (), (override));
    MOCK_METHOD(void, Nack, (), (override));
  };

  class TopicMock : public pubsuber::Topic {
  public:
    MOCK_METHOD(std::string, ID, (), (noexcept, override));
    MOCK_METHOD(std::string, Name, (), (noexcept, override));
    MOCK_METHOD(bool, Exists, (), (override));
    MOCK_METHOD(void, Create, (), (override));
    MOCK_METHOD(void, Delete, (), (override));
    MOCK_METHOD(std::string, Publish, (const pubsuber::ByteBuffer &buffer, (const std::map<std::string, std::string> &attributes)), (override));
  };

  class SubscriptionMock : public pubsuber::Subscription {
  public:
    MOCK_METHOD(std::string, ID, (), (noexcept, override));
    MOCK_METHOD(std::string, Name, (), (noexcept, override));
    MOCK_METHOD(bool, Exists, (), (override));
    MOCK_METHOD(void, Create, (Subscription::CreationOptions && options), (override));
    MOCK_METHOD(void, Delete, (), (override));
    MOCK_METHOD(void, Receive, (pubsuber::Callback cb), (override));
    MOCK_METHOD(void, Stop, (), (override));
  };
};  // namespace pubsuber
