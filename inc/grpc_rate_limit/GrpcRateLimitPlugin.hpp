#pragma once

#include <grpc_rate_limit/IRateLimiter.hpp>
#include <memory>

namespace grpc_rate_limit
{

struct GrpcRateLimitPlugin final
{
  GrpcRateLimitPlugin() = delete;
  ~GrpcRateLimitPlugin() = delete;
  GrpcRateLimitPlugin(const GrpcRateLimitPlugin&) = delete;
  GrpcRateLimitPlugin(GrpcRateLimitPlugin&&) = delete;
  GrpcRateLimitPlugin& operator=(const GrpcRateLimitPlugin&) = delete;
  GrpcRateLimitPlugin& operator=(GrpcRateLimitPlugin&&) = delete;

  static void registerGrpcRateLimitPlugin(
    std::unique_ptr<IRateLimiter> pRateLimiter);
};

}
