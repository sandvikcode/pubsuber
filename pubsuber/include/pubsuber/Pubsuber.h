#pragma once

#include <chrono>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace pubsuber {

  constexpr auto kUninitializedCode = 0x7FffFFff;

  class Exception : public std::runtime_error {
  public:
    explicit Exception(const char *what, int code = kUninitializedCode);
    explicit Exception(const std::string &what, int code = kUninitializedCode);
    virtual ~Exception() {}

    int Code() const { return _code; }

  private:
    int _code;
  };

  using ByteBuffer = std::string;

  /**
   * Message that encapsulates PubsubMessage and supports AcK/Nack functionality
   * Underlying implementation is thread safe unless stated otherwise
   */
  class Message {
  public:
    virtual ~Message() = default;

    /**
     * Message payload as in pubsub message
     */
    virtual const ByteBuffer &Payload() const = 0;

    /**
     * Message attributes as in pubsub message
     */
    virtual const std::map<std::string, std::string> &Attributes() const noexcept(true) = 0;

    /**
     * Fully qualified subscription name message belongs to
     */
    virtual const std::string &SubscriptionName() const noexcept(true) = 0;

    /**
     * Send acknowledgment request
     */
    virtual void Ack() = 0;

    /**
     * Modifies ack deadline to 0s
     */
    virtual void Nack() = 0;
  };

  using MessagePtr = std::unique_ptr<Message>;

  /**
   * Underlying implementation is thread safe unless stated otherwise
   */
  class Topic {
  public:
    virtual ~Topic() = default;

    /**
     * Short topic name
     */
    virtual std::string ID() noexcept(true) = 0;

    /**
     * Fully qualified topic name
     */
    virtual std::string Name() noexcept(true) = 0;

    /**
     * Checks if topic is exist in pubsub backend
     */
    virtual bool Exists() = 0;

    /**
     * Creates topic in pubsub backend
     */
    virtual void Create() = 0;

    /**
     * Deletes topic in pubsub backend
     */
    virtual void Delete() = 0;

    /**
     * Publish pubsub message with given payload and attributes
     * Binary data could be serialized in string object
     * Attribute keys must not be empty
     */
    virtual std::string Publish(const ByteBuffer &buffer, const std::map<std::string, std::string> &attributes = {}) = 0;
  };

  using TopicPtr = std::shared_ptr<Topic>;

  /**
   *
   * Underlying implementation is thread safe unless stated otherwise
   */
  class Subscription {
  public:
    struct CreationOptions {
      // Fully qualified topic name
      const std::string _fullTopicName;

      /**
       * Subscription ack deadline.
       * The minimum custom deadline you can specify is 10 seconds.
       * The maximum custom deadline you can specify is 600 seconds (10 minutes).
       * If this parameter is 0, a default value of 10 seconds is used.
       */
      const std::chrono::seconds _ackDeadline;
    };

    virtual ~Subscription() = default;

    using Callback = std::function<void(pubsuber::MessagePtr &&m)>;

    /**
     * Short subscription name
     */
    virtual std::string ID() noexcept(true) = 0;

    /**
     * Fully qualified subscription name
     */
    virtual std::string Name() noexcept(true) = 0;

    /**
     * Checks is subscription is exist in pubsub backend
     */
    virtual bool Exists() = 0;

    /**
     * Creates subscription with given deadline in pubsub backend
     */
    virtual void Create(Subscription::CreationOptions &&options) = 0;

    /**
     * Deletes given subscription in pubsub backend
     */
    virtual void Delete() = 0;

    /**
     * Register a callback that is used to receive messages
     * Callback execution must be fast as it is called on pulling thread which should not be blocked
     */
    virtual void Receive(Callback cb) = 0;
    //		virtual void ReceiveOnQueue(Callback cb /*, queue */) = 0;

    /**
     * Stop receiving messages for given subscription
     * Also there is no guaranty that message acks will be sent once Stop is called
     */
    virtual void Stop() = 0;
  };

  using SubscriptionPtr = std::shared_ptr<Subscription>;

  /**
   * Callback type
   */
  using Callback = std::function<void(pubsuber::MessagePtr &&)>;

  // Internal client backbone
  struct Executor;

  /**
   * Report operational metrics.
   *
   * Implementation must be thread safe. Methods could be called from different threads.
   * Methods must be fast and do as least job as possible otherwise it may affect the performance
   *
   */
  class MetricSink {
  public:
    virtual ~MetricSink() = default;

    /**
     * Reports size of the queue that is used to extend message deadlines
     */
    virtual void OnKeepAliveQueueSize(size_t size) = 0;
  };

  /**
   * Sink is used to receive debug logs
   *
   *
   */
  class LogSink {
  public:
    virtual ~LogSink() = default;
    // TODO
  };

  /**
   * Options used to tweak client behavior
   */
  class ClientOptions final {
  public:
    /**
     * Client should call this method to create default options
     */
    static ClientOptions CreateDefault(std::string &&project);

    const std::string &Project() const { return _project; }

    ClientOptions &SetHost(std::string host) {
      _pubsubHost = host;
      return *this;
    }
    const std::string &Host() const { return _pubsubHost; }

    ClientOptions &SetSecureChannel(bool secure) {
      _secureChannel = secure;
      return *this;
    }
    bool SecureChannel() const { return _secureChannel; }

    ClientOptions &SetMaxPrefetch(int32_t max) {
      _maxMessagePrefetch = max;
      return *this;
    }
    int32_t MaxPrefetch() const { return _maxMessagePrefetch; }

  private:
    explicit ClientOptions(std::string &&project, std::string host, bool secure, int32_t prefetch);

  private:
    const std::string _project;
    std::string _pubsubHost;
    bool _secureChannel;
    int32_t _maxMessagePrefetch;
  };

  /**
   * Pubsub client
   * Implementation is thread safe
   */
  class Client final {
  public:
    static std::unique_ptr<Client> Create(ClientOptions &&opts);

    ~Client();

    /**
     * id [in] - non-fully qualified topic name
     * project [in] - project name if different from client's default project
     */
    TopicPtr GetTopic(const std::string &id, const std::string &project = "");

    /**
     * id [in] - non-fully qualified subscription name
     * project [in] - project name if different from client's default project
     *
     */
    SubscriptionPtr GetSubscription(const std::string &id, const std::string &project = "");

    /**
     * Add metric sink
     * Method is thread safe
     */
    // exception
    void AddMetricSink(std::shared_ptr<MetricSink> sink);

    /**
     * Removes previously sink
     * Method is thread safe
     */
    // exception
    void RemoveMetricSink();

    /**
     * Add log sink
     * Method is thread safe
     */
    // exception
    void AddLogSink(std::shared_ptr<LogSink> sink);

    /**
     * Removes previously sink
     * Method is thread safe
     */
    // exception
    void RemoveLogSink();

  private:
    explicit Client(ClientOptions &&opts);

  private:
    std::shared_ptr<Executor> _executor;
  };
}  // namespace pubsuber
