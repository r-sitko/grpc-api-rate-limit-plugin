#pragma once

#include <chrono>
#include <mutex>

namespace grpc_rate_limit
{

class TokenBucket final
{
public:
  explicit TokenBucket(
    uint64_t maxTokensInBucket,
    uint64_t fillIntervalMs,
    uint64_t tokensRefreshAmount);
  ~TokenBucket() = default;
  TokenBucket(const TokenBucket&) = delete;
  TokenBucket(TokenBucket&&) = delete;
  TokenBucket& operator=(const TokenBucket&) = delete;
  TokenBucket& operator=(TokenBucket&&) = delete;

  bool consume(uint32_t tokens);

private:
  void tryRefill();

  const uint64_t m_kMaxTokensInBucket;
  const uint64_t m_kFillIntervalMs;
  const uint64_t m_kTokensRefreshAmount;
  uint64_t m_tokensAvailable;
  std::chrono::steady_clock::time_point::duration::rep m_lastRefillTime;
  std::mutex m_mutex;
};

}
