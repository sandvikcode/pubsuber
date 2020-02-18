#include <google/protobuf/message.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <chrono>
#include <cstdio>
#include <future>
#include <iostream>
#include <list>
#include <random>
#include <sstream>
#include <thread>

#include "pubsuber/Pubsuber.h"

using namespace pubsuber;
using namespace std::chrono;

namespace {
  const auto kTopicName = "mwapi-task%1";
  const auto kSubscriptionName = "mwapi-task-subscription%1";

  std::random_device rd;   // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<> dis(1, 4);
}  // namespace

class MetricSinkTest: public pubsuber::MetricSink
{
public:
  MetricSinkTest() = default;
  virtual ~MetricSinkTest() = default;

  virtual void OnKeepAliveQueueSize(size_t size) override {
    spdlog::debug("MMMM: OnKeepAliveQueueSize: {}", size);
  }

};

void DoTheJob(uint32_t limit, const char *project) {

  pubsuber::ClientOptions opts = pubsuber::ClientOptions::CreateDefault(project);
  auto msink = std::make_shared<MetricSinkTest>();
  opts.SetMetricSink(msink);

  auto sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
  sink->set_pattern("%D-%T.%f [%n] [%t] [%L]: %v");
  opts.SetLogSink(sink);
  opts.SetLogLevel(spdlog::level::debug);

  pubsuber::ClientPtr client;
  try {
    pubsuber::RetryCountPolicy countPolicy{4};
    pubsuber::MaxRetryTimePolicy timePolicy{30s};
    pubsuber::ExponentialBackoffPolicy backoffPolicy{250ms, 3s, 2.0};

    spdlog::info("PS version: {}", pubsuber::Client::VersionString());

    client = pubsuber::Client::Create(std::move(opts), std::move(timePolicy), std::move(countPolicy), std::move(backoffPolicy));
    client->SetLogLevel(spdlog::level::debug);

    auto topic = client->GetTopic(kTopicName);
    if (!topic->Exists()) {
      spdlog::debug("Creating topic");
      topic->Create();
    }

    auto subscription = client->GetSubscription(kSubscriptionName);

    subscription->Delete();

    if (!subscription->Exists()) {
      spdlog::debug("Creating subscription for topic: {}", topic->Name());
      subscription->Create({topic->Name(), 10s});
    }

    spdlog::debug("Going to publish");
    // Publish only after subscription is exists so we could actually get the message
    auto id = topic->Publish("My message", {{"key1", "KKK1"}, {"key2", "KKK2"}});
    spdlog::debug("Publish msg id: {}", id);

    std::list<MessagePtr> messages;
    std::mutex mmm;

    subscription->Receive([&messages, &mmm](MessagePtr&& m) {
      spdlog::debug("Receive msg: {}", m->SubscriptionName());
      for (auto& it : m->Attributes()) {
        spdlog::debug("Attrs: {}:{}", it.first, it.second);
      }

      std::unique_lock l(mmm);
      messages.push_back(std::move(m));
    });

    size_t count = 0;
    while (count++ < limit) {
      spdlog::debug("Main thread run...");
      std::this_thread::sleep_for(std::chrono::seconds(dis(gen)));
      auto idd = topic->Publish("My message" + std::to_string(count), {{"key1", "KKK1" + std::to_string(count)}, {"key2", "KKK2"}});
      spdlog::debug("Publish msg id: {}", idd);

      if (count > 10) {
        std::unique_lock l(mmm);
        if (count % 2 == 0) {
          messages.back()->Nack();
        } else {
          messages.back()->Ack();
        }
        messages.pop_back();
      }
    }

    spdlog::debug("=== Subscription STOP ===");
    subscription->Stop();

    for (auto&& m : messages) {
      m->Ack();
    }
    messages.clear();

    spdlog::debug("=== Subscription Delete ===");
    subscription->Delete();

    std::cout << subscription->Exists() << std::endl;
    topic->Delete();
    std::cout << topic->Exists() << std::endl;

    subscription->Delete();
    std::cout << subscription->Exists() << std::endl;
    topic->Delete();
    std::cout << topic->Exists() << std::endl;

    spdlog::debug("============== END ============== ");
    std::this_thread::sleep_for(2s);

  } catch (pubsuber::Exception& e) {
    spdlog::error("Exception: {}", e.what());

    google::protobuf::ShutdownProtobufLibrary();
    std::exit(1);
  }
}

int main(int argc, char** argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  spdlog::set_level(spdlog::level::debug);  // Set global log level to debug
  spdlog::set_pattern("%D-%T.%f [%n] [%t] [%L]: %v");
  spdlog::debug("Start");

  if (argc < 3) {
    std::cerr << "Provide run limit as integer and project name" << std::endl;
    return 1;
  }

  DoTheJob(std::atoi(argv[1]), argv[2]);

  std::this_thread::sleep_for(2s);
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
