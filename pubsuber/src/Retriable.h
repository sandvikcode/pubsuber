#pragma once
#include <chrono>
#include <cstring>
#include <thread>
#include <tuple>

#include "ExpoBackoff.h"
#include "PSLog.h"

#include "google/pubsub/v1/pubsub.grpc.pb.h"

#ifdef WIN32
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

namespace pubsuber::retriable {
  // Helper to extract function parameter name for debugging purposes
  template <typename NamedParamType>
  struct Namer {
    static const char *Name() {
      static const auto func_size = sizeof(__PRETTY_FUNCTION__);
      static const char *const open_term =
#ifdef WIN32
          "pubsuber::retriable::Namer<";
#else
          "NamedParamType = ";
#endif

      static const auto open_term_len =
#ifdef WIN32
          sizeof("pubsuber::retriable::Namer<") - 1;
#else
          sizeof("NamedParamType = ") - 1;
#endif

      static const auto close_term_len =
#ifdef WIN32
          sizeof(">::Name(void)") - 1;
#else
          sizeof("]") - 1;
#endif
      static char buff[func_size] = {};

      strncpy(buff, __PRETTY_FUNCTION__, func_size);
      auto p = strstr(buff, open_term);
      if (p == nullptr) return "???";
      memmove(buff, p + open_term_len, buff + func_size - p - open_term_len);
      buff[strlen(buff) - close_term_len] = '\0';
      return buff;
    }
  };

  template <typename F>
  const char *GetName() {
    return Namer<F>::Name();
  }

  constexpr inline bool retries_exahusted(std::chrono::steady_clock::time_point start, uint32_t retryCount, const RetryCountPolicy &countPolicy,
                                          const MaxRetryTimePolicy &timePolicy) {
    return (countPolicy._count > 0 && retryCount > countPolicy._count) || (std::chrono::steady_clock::now() - start) > timePolicy._interval;
  }

  using namespace std::chrono;
  constexpr auto kDefaultRPCTimeout = 20s;

  inline void set_deadline(::grpc::ClientContext &ctx, std::chrono::seconds timeout = kDefaultRPCTimeout) {
    std::chrono::system_clock::time_point deadline = std::chrono::system_clock::now() + timeout;
    ctx.set_deadline(deadline);
  }

  template <typename Fn>
  struct StubSignature {};

  // Publisher specialization
  template <typename Request, typename Response>
  struct StubSignature<::grpc::Status (::google::pubsub::v1::Publisher::Stub::*)(::grpc::ClientContext *, const Request &, Response *)> {
    using RequestType = Request;
    using ResponseType = Response;
  };

  // Subscriber specialization
  template <typename Request, typename Response>
  struct StubSignature<::grpc::Status (::google::pubsub::v1::Subscriber::Stub::*)(::grpc::ClientContext *, const Request &, Response *)> {
    using RequestType = Request;
    using ResponseType = Response;
  };

  using ReturnType = std::tuple<::grpc::Status, std::chrono::steady_clock::time_point>;

  template <typename StubPtr, typename MemberFunc>
  ReturnType make_call(StubPtr &stub, MemberFunc function, typename StubSignature<MemberFunc>::RequestType &request,
                       typename StubSignature<MemberFunc>::ResponseType &response, RetryCountPolicy countPolicy, MaxRetryTimePolicy timePolicy,
                       ExponentialBackoffPolicy backoffPolicy, std::chrono::seconds rpcTimeout = retriable::kDefaultRPCTimeout) {
    ExpoBackoff backoff(backoffPolicy);
    const auto start = std::chrono::steady_clock::now();

    ::grpc::Status status = ::grpc::Status::OK;
    while (!retries_exahusted(start, backoff.RetryCount(), countPolicy, timePolicy)) {
      ::grpc::ClientContext ctx;
      set_deadline(ctx, rpcTimeout);

      status = ::grpc::Status::OK;
      const auto next = std::chrono::steady_clock::now();
      switch (status = (*stub.*function)(&ctx, request, &response); status.error_code()) {
        case ::grpc::StatusCode::DEADLINE_EXCEEDED:
          // Retry once again
          rpcTimeout *= 2;
          logger::debug("make_call failed function call '{}' with DEADLINE_EXCEEDED, new timeout is {}s", GetName<MemberFunc>(), rpcTimeout.count());
          [[fallthrough]];

        case ::grpc::StatusCode::UNAVAILABLE: {
          const auto delay = backoff.Delay();
          logger::debug("make_call will retry in {}ms, attempt: {}, function: '{}'", delay.count(), backoff.RetryCount() - 1, GetName<MemberFunc>());
          std::this_thread::sleep_for(delay);
        }
          continue;

        default:
          return {status, next};

      }  // switch
    }    // while
    const auto err = "GRPC call '" + std::string(GetName<MemberFunc>()) + "' failed after " + std::to_string(countPolicy._count) + " times or " +
                     std::to_string(timePolicy._interval.count()) + "s with error";
    throw Exception(err, status.error_code());
  }  // function
}  // namespace pubsuber::retriable
