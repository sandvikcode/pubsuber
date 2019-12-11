#pragma once

#include "Executor.h"

namespace pubsuber {

  // Implementation is thread safe
  class SubscriptionImpl : public Subscription, public EntityBase {
    SubscriptionImpl() = delete;

  public:
    SubscriptionImpl(std::weak_ptr<Executor> simpl, const std::string &id, const std::string &name);
    virtual ~SubscriptionImpl();

  private:
    virtual std::string ID() noexcept(true) override { return _id; }
    virtual std::string Name() noexcept(true) override { return _fullName; }
    virtual bool Exists() override;
    virtual void Create(Subscription::CreationOptions &&options) override;
    virtual void Delete() override;
    virtual void Receive(Callback cb) override;
    virtual void Stop() override;

  private:
    std::chrono::seconds _ackDeadline;
    std::weak_ptr<Executor> _executor;
    Trimpl _tr;
    bool _receiverActive{false};
  };

}  // namespace pubsuber
