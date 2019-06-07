#pragma once

#include <string>

namespace grpc_rate_limit
{

struct IRateLimiter
{
  virtual ~IRateLimiter() = default;
  IRateLimiter(const IRateLimiter&) = default;
  IRateLimiter(IRateLimiter&&) = default;
  IRateLimiter& operator=(const IRateLimiter&) = default;
  IRateLimiter& operator=(IRateLimiter&&) = default;

  virtual bool shouldLimit(const std::string& method) = 0;

protected:
  IRateLimiter() = default;
};

}
