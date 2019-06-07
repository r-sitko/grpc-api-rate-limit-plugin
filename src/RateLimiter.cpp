#include <grpc_rate_limit/RateLimiter.hpp>
#include <grpc_rate_limit/TokenBucket.hpp>
#include <stdexcept>

namespace grpc_rate_limit
{

bool RateLimiter::shouldLimit(const std::string& method)
{
  auto it = m_limiters.find(method);
  if (it != m_limiters.end())
  {
    return !it->second->consume(1);
  }
  else
  {
    return false;
  }
}

void RateLimiter::addRateLimit(
  const std::string& method,
  std::shared_ptr<TokenBucket> pTokenBucket)
{
  if (!pTokenBucket)
  {
    throw std::invalid_argument("pTokenBucket nullptr");
  }
  if (m_limiters.find(method) == m_limiters.end())
  {
    m_limiters[method] = pTokenBucket;
  }
  else
  {
    throw std::runtime_error("Rate limit for method "
      + method + " is already registered.");
  }
}

}
