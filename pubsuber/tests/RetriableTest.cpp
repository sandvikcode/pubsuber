#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>
#include <random>
#include <string>

#include "Retriable.h"
#include "google/pubsub/v1/pubsub_mock.grpc.pb.h"

using namespace pubsuber;
using namespace std::chrono;
using google::pubsub::v1::MockPublisherStub;
using testing::_;
using testing::Return;
const std::string ErrorMessage = "This is error in test";

namespace pubsuber {
  namespace retriable {
    template <typename Request, typename Response>
    struct StubSignature<::grpc::Status (::google::pubsub::v1::MockPublisherStub::*)(::grpc::ClientContext *, const Request &, Response *)> {
      using StubObjectType = ::google::pubsub::v1::MockPublisherStub;
      using RequestType = Request;
      using ResponseType = Response;
    };
  }  // namespace retriable
}  // namespace pubsuber

TEST(Retriable, make_call_is_ok) {
  auto publisher = std::make_unique<MockPublisherStub>();

  ::google::pubsub::v1::PublishRequest request;
  ::google::pubsub::v1::PublishResponse response;

  RetryCountPolicy countPolicy;
  MaxRetryTimePolicy timePolicy;
  ExponentialBackoffPolicy backoffPolicy;
  backoffPolicy._maxDelay = 30s;

  EXPECT_CALL(*publisher, Publish(_, _, _)).Times(1);

  ::grpc::Status retStatus = ::grpc::Status::OK;
  ON_CALL(*publisher, Publish(_, _, _)).WillByDefault(Return(retStatus));

  auto [status, now] = retriable::make_call(publisher, &MockPublisherStub::Publish, request, response, countPolicy, timePolicy, backoffPolicy, 1s);
  EXPECT_EQ(status.error_code(), retStatus.error_code());
}

TEST(Retriable, make_call_is_failed) {
  auto publisher = std::make_unique<MockPublisherStub>();

  ::google::pubsub::v1::PublishRequest request;
  ::google::pubsub::v1::PublishResponse response;

  RetryCountPolicy countPolicy;
  MaxRetryTimePolicy timePolicy;
  ExponentialBackoffPolicy backoffPolicy;
  backoffPolicy._maxDelay = 30s;

  EXPECT_CALL(*publisher, Publish(_, _, _)).Times(1);

  ::grpc::Status retStatus(::grpc::StatusCode::UNIMPLEMENTED, ErrorMessage);
  ON_CALL(*publisher, Publish(_, _, _)).WillByDefault(Return(retStatus));

  auto [status, now] = retriable::make_call(publisher, &MockPublisherStub::Publish, request, response, countPolicy, timePolicy, backoffPolicy, 1s);
  EXPECT_EQ(status.error_code(), retStatus.error_code());
  EXPECT_EQ(status.error_message(), retStatus.error_message());
  EXPECT_EQ(status.error_message(), ErrorMessage);
}

TEST(Retriable, make_call_is_deadline) {
  auto publisher = std::make_unique<MockPublisherStub>();

  ::google::pubsub::v1::PublishRequest request;
  ::google::pubsub::v1::PublishResponse response;

  RetryCountPolicy countPolicy;
  MaxRetryTimePolicy timePolicy;
  ExponentialBackoffPolicy backoffPolicy;
  backoffPolicy._maxDelay = 30s;

  EXPECT_CALL(*publisher, Publish(_, _, _)).Times(1 + countPolicy._count);

  ::grpc::Status retStatus(::grpc::StatusCode::DEADLINE_EXCEEDED, ErrorMessage);
  ON_CALL(*publisher, Publish(_, _, _)).WillByDefault(Return(retStatus));

  bool limitReached = false;
  try {
    auto [status, now] = retriable::make_call(publisher, &MockPublisherStub::Publish, request, response, countPolicy, timePolicy, backoffPolicy, 1s);
    EXPECT_EQ(status.error_code(), ::grpc::Status::OK.error_code());

  } catch (Exception &e) {
    limitReached = true;
    EXPECT_EQ(e.Code(), ::grpc::StatusCode::DEADLINE_EXCEEDED);
  }

  EXPECT_TRUE(limitReached);
}

TEST(Retriable, make_call_is_unavailable) {
  auto publisher = std::make_unique<MockPublisherStub>();

  ::google::pubsub::v1::PublishRequest request;
  ::google::pubsub::v1::PublishResponse response;

  RetryCountPolicy countPolicy;
  MaxRetryTimePolicy timePolicy;
  ExponentialBackoffPolicy backoffPolicy;
  backoffPolicy._maxDelay = 30s;

  EXPECT_CALL(*publisher, Publish(_, _, _)).Times(1 + countPolicy._count);

  ::grpc::Status retStatus(::grpc::StatusCode::UNAVAILABLE, ErrorMessage);
  ON_CALL(*publisher, Publish(_, _, _)).WillByDefault(Return(retStatus));

  bool limitReached = false;
  try {
    auto [status, now] = retriable::make_call(publisher, &MockPublisherStub::Publish, request, response, countPolicy, timePolicy, backoffPolicy, 1s);
    EXPECT_EQ(status.error_code(), ::grpc::Status::OK.error_code());

  } catch (Exception &e) {
    limitReached = true;
    EXPECT_EQ(e.Code(), ::grpc::StatusCode::UNAVAILABLE);
  }

  EXPECT_TRUE(limitReached);
}

TEST(Retriable, make_call_is_unavailable_count) {
  auto publisher = std::make_unique<MockPublisherStub>();

  ::google::pubsub::v1::PublishRequest request;
  ::google::pubsub::v1::PublishResponse response;

  RetryCountPolicy countPolicy;
  countPolicy._count = 5;
  MaxRetryTimePolicy timePolicy;
  timePolicy._interval = 130s;
  ExponentialBackoffPolicy backoffPolicy;
  backoffPolicy._maxDelay = 30s;

  EXPECT_CALL(*publisher, Publish(_, _, _)).Times(1 + countPolicy._count);

  ::grpc::Status retStatus(::grpc::StatusCode::UNAVAILABLE, ErrorMessage);
  ON_CALL(*publisher, Publish(_, _, _)).WillByDefault(Return(retStatus));

  bool limitReached = false;
  try {
    auto [status, now] = retriable::make_call(publisher, &MockPublisherStub::Publish, request, response, countPolicy, timePolicy, backoffPolicy, 1s);
    EXPECT_EQ(status.error_code(), ::grpc::Status::OK.error_code());

  } catch (Exception &e) {
    limitReached = true;
    EXPECT_EQ(e.Code(), ::grpc::StatusCode::UNAVAILABLE);
  }

  EXPECT_TRUE(limitReached);
}
