#pragma once

#include "Trimpl.h"
#include "pubsuber/Pubsuber.h"

namespace pubsuber {

  // Implementation is thread safe
  class TopicImpl : public Topic, public EntityBase {
    TopicImpl() = delete;

  public:
    TopicImpl(Trimpl &&impl, const std::string &id, const std::string &topic, RetryCountPolicy countPolicy, MaxRetryTimePolicy timePolicy,
              ExponentialBackoffPolicy backoffPolicy);
    virtual ~TopicImpl() = default;

  private:
    virtual std::string ID() noexcept(true) override { return _id; }
    virtual std::string Name() noexcept(true) override { return _fullName; }
    virtual bool Exists() override;
    virtual void Create() override;
    virtual void Delete() override;
    virtual std::string Publish(const ByteBuffer &buffer, const std::map<std::string, std::string> &attributes = {}) override;

    Trimpl _tr;
    const RetryCountPolicy _countPolicy;
    const MaxRetryTimePolicy _timePolicy;
    const ExponentialBackoffPolicy _backoffPolicy;
  };

}  // namespace pubsuber
