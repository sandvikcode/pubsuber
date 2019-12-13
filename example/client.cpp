#include <google/protobuf/message.h>
#include <spdlog/spdlog.h>
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

  std::unique_ptr<Client> client;
  try {
    client = pubsuber::Client::Create(std::move(opts));
    client->AddMetricSink(msink);

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

      if (count > 200) {
        std::unique_lock l(mmm);
        messages.back()->Ack();
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

    client->RemoveMetricSink();

  } catch (pubsuber::Exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    google::protobuf::ShutdownProtobufLibrary();
    std::exit(1);
  }
}

int main(int argc, char** argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  spdlog::set_level(spdlog::level::debug);  // Set global log level to debug
  spdlog::set_pattern("%D-%T.%f [%t] [%L]: %v");
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
