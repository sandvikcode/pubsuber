#include "Pubsuber.h"

#include <grpc++/grpc++.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include "Executor.h"
#include "MessageImpl.h"
#include "SubscriptionImpl.h"
#include "TopicImpl.h"

using namespace pubsuber;
using namespace std::chrono;

namespace {
  const std::string kDefaultPubsubHost = "pubsub.googleapis.com";
}

//**********************************************************************************************************************

Exception::Exception(const char *what, int code)
: std::runtime_error(what)
, _code(code) {}

Exception::Exception(const std::string &what, int code)
: std::runtime_error(what.c_str())
, _code(code) {}

//**********************************************************************************************************************

Client::Client(ClientOptions &&opts)
: _executor(std::make_shared<Executor>(std::move(opts))) {
  if (!_executor->_tr.EnsureConnected()) {
    // Executor should be stopped before killed bcuz of threw exception
    _executor->StopThreads();
    const auto err = "Unable to connect to " + _executor->_options.Host() + " within given timeout";
    throw Exception(err);
  }
}

TopicPtr Client::GetTopic(const std::string &id, const std::string &project) {
  if (id.empty()) {
    throw Exception("topic id (non-fully qualified name) must not be empty string");
  }

  const auto &prj = (project == "") ? _executor->_options.Project() : project;
  auto topicName = "projects/" + prj + "/topics/" + id;

  Trimpl impl = _executor->_tr;

  return std::make_shared<TopicImpl>(std::move(impl), id, topicName);
}

SubscriptionPtr Client::GetSubscription(const std::string &id, const std::string &project) {
  if (id.empty()) {
    throw Exception("topic id (non-fully qualified name) must not be empty string");
  }

  const auto &prj = (project == "") ? _executor->_options.Project() : project;
  const auto subsName = "projects/" + prj + "/subscriptions/" + id;

  return std::make_shared<SubscriptionImpl>(_executor, id, subsName);
}

std::unique_ptr<Client> Client::Create(ClientOptions &&opts) {
  if (opts.Project().empty()) {
    throw Exception("project must not be empty string");
  }

  if (opts.Host().empty()) {
    throw Exception("pubsubHost must not be empty");
  }

  return std::unique_ptr<Client>(new Client(std::move(opts)));
}

Client::~Client() { _executor->StopThreads(); }

void Client::AddMetricSink(std::shared_ptr<MetricSink> sink) { _executor->AddMetricSink(sink); }

void Client::RemoveMetricSink() { _executor->RemoveMetricSink(); }

void Client::AddLogSink(std::shared_ptr<LogSink> sink) {}

void Client::RemoveLogSink() {}

//**********************************************************************************************************************

ClientOptions ClientOptions::CreateDefault(std::string &&project) {
  // clang-format
  return ClientOptions{std::move(project), kDefaultPubsubHost, true, 4};
}

ClientOptions::ClientOptions(std::string &&project, std::string host, bool secure, int32_t prefetch)
: _project(std::move(project))
, _pubsubHost(host)
, _secureChannel(secure)
, _maxMessagePrefetch(prefetch) {}
