#pragma once
#include <grpc++/grpc++.h>
#include <chrono>
#include <memory>
#include <string>

namespace pubsuber {

  using namespace std::chrono;
  constexpr auto kMinAckDeadline = 10s;
  constexpr auto kMaxAckDeadline = 600s;

  constexpr auto kDefultRetryAttempts = 3;
  constexpr auto kDefaultRPCTimeout = 30s;

  void set_deadline(grpc::ClientContext &ctx, std::chrono::seconds timeout = kDefaultRPCTimeout);

  // Transport Impl = Trimpl
  struct Trimpl {
    // Call to make sure channel is connected
    bool EnsureConnected();

    std::shared_ptr<grpc::ChannelCredentials> _creds{grpc::GoogleDefaultCredentials()};
    std::shared_ptr<grpc::Channel> _channel;
  };

  //**********************************************************************************************************************
  class EntityBase {
  protected:
    EntityBase(const std::string &id, const std::string &fullName)
    : _id(id)
    , _fullName(fullName) {}

  protected:
    const std::string _id;
    const std::string _fullName;
  };

}  // namespace pubsuber
