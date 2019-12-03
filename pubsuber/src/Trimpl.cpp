#include "Trimpl.h"
#include <chrono>

using namespace pubsuber;
using namespace std::chrono;

bool Trimpl::EnsureConnected() {
  const auto timeout = 5s;

  std::chrono::system_clock::time_point deadline = std::chrono::system_clock::now() + timeout;
  // Wait for the channel to connect
  return _channel->WaitForConnected(deadline);
}

void pubsuber::set_deadline(grpc::ClientContext &ctx, std::chrono::seconds timeout) {
  std::chrono::system_clock::time_point deadline = std::chrono::system_clock::now() + timeout;
  ctx.set_deadline(deadline);
}
