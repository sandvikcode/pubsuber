#include "pubsuber/Pubsuber.h"

#include <grpc++/grpc++.h>
#include <algorithm>
#include <memory>
#include "Executor.h"
#include "MessageImpl.h"
#include "PSLog.h"
#include "PubsuberVersion.h"
#include "SubscriptionImpl.h"
#include "TopicImpl.h"

using namespace pubsuber;
using namespace std::chrono;

namespace {
  const std::string DefaultPubsubHost = "pubsub.googleapis.com";
}  // namespace

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

void Client::ApplyPolicies() {
  if (_executor) {
    _executor->ApplyPolicies(_countPolicy, _timePolicy, _backoffPolicy);
  }
}

TopicPtr Client::GetTopic(const std::string &id, const std::string &project) {
  if (id.empty()) {
    throw Exception("topic id (non-fully qualified name) must not be empty string");
  }

  const auto &prj = (project == "") ? _executor->_options.Project() : project;
  auto topicName = "projects/" + prj + "/topics/" + id;

  Trimpl impl = _executor->_tr;

  return std::make_shared<TopicImpl>(std::move(impl), id, topicName, _countPolicy, _timePolicy, _backoffPolicy);
}

SubscriptionPtr Client::GetSubscription(const std::string &id, const std::string &project) {
  if (id.empty()) {
    throw Exception("topic id (non-fully qualified name) must not be empty string");
  }

  const auto &prj = (project == "") ? _executor->_options.Project() : project;
  const auto subsName = "projects/" + prj + "/subscriptions/" + id;

  return std::make_shared<SubscriptionImpl>(_executor, id, subsName, _countPolicy, _timePolicy, _backoffPolicy);
}

Client::~Client() { _executor->StopThreads(); }

void Client::SetLogLevel(spdlog::level::level_enum level) { pubsuber::logger::ChangeLevel(level); }

std::string Client::VersionString() { return PUBSUBER_VERSION_STRING; }

//**********************************************************************************************************************

ClientOptions ClientOptions::CreateDefault(std::string &&project) {
  // clang-format
  return ClientOptions{std::move(project), DefaultPubsubHost, true, 4};
}

ClientOptions::ClientOptions(std::string &&project, std::string host, bool secure, int32_t prefetch)
: _project(std::move(project))
, _pubsubHost(host)
, _secureChannel(secure)
, _maxMessagePrefetch(prefetch)
, _logLevel(spdlog::level::info) {}
