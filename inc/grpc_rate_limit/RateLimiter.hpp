#pragma once

#include <grpc_rate_limit/IRateLimiter.hpp>
#include <memory>
#include <map>
#include <string>

namespace grpc_rate_limit
{

class TokenBucket;

class RateLimiter final : public IRateLimiter
{
public:
  RateLimiter() = default;
  ~RateLimiter() = default;
  RateLimiter(const RateLimiter&) = delete;
  RateLimiter(RateLimiter&&) = delete;
  RateLimiter& operator=(const RateLimiter&) = delete;
  RateLimiter& operator=(RateLimiter&&) = delete;

  bool shouldLimit(const std::string& method) final;
  void addRateLimit(
    const std::string& method,
    std::shared_ptr<TokenBucket> pTokenBucket);

private:
  std::map<std::string, std::shared_ptr<TokenBucket>> m_limiters;
};

}
