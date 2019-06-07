#include <grpc_rate_limit/TokenBucket.hpp>
#include <algorithm>

namespace grpc_rate_limit
{

TokenBucket::TokenBucket(
  uint64_t maxTokensInBucket,
  uint64_t fillIntervalMs,
  uint64_t tokensRefreshAmount)
  : m_kMaxTokensInBucket{maxTokensInBucket},
    m_kFillIntervalMs{fillIntervalMs},
    m_kTokensRefreshAmount{tokensRefreshAmount},
    m_tokensAvailable{m_kMaxTokensInBucket},
    m_lastRefillTime{std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count()}
{}

bool TokenBucket::consume(uint32_t tokens)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  tryRefill();
  if (m_tokensAvailable >= tokens)
  {
    m_tokensAvailable -= tokens;
    return true;
  }
  else
  {
    return false;
  }
}

void TokenBucket::tryRefill()
{
  using namespace std::chrono;
  const auto now =
    duration_cast<std::chrono::microseconds>(
      steady_clock::now().time_since_epoch())
      .count();
  auto timeDiff = now - m_lastRefillTime;
  const uint64_t numberOfNewTokenPacks = timeDiff / (m_kFillIntervalMs * 1000);
  const uint64_t newTokens = m_kTokensRefreshAmount * numberOfNewTokenPacks;
  if (newTokens)
  {
    m_tokensAvailable =
      std::min(m_kMaxTokensInBucket, m_tokensAvailable + newTokens);
    m_lastRefillTime += numberOfNewTokenPacks * (m_kFillIntervalMs * 1000);
  }
}

}
