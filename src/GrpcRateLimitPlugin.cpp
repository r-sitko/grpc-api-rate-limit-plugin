#include <grpc_rate_limit/GrpcRateLimitPlugin.hpp>
#include "GrpcRateLimitServerCallData.hpp"
#include "GrpcRateLimitChannelData.hpp"
#include <src/cpp/common/channel_filter.h>
#include <climits>
#include <cassert>
#include <stdexcept>

namespace grpc_rate_limit
{

void GrpcRateLimitPlugin::registerGrpcRateLimitPlugin(
  std::unique_ptr<IRateLimiter> pRateLimiter)
{
  static bool initDone = false;

  if (!pRateLimiter)
  {
    throw std::invalid_argument("pRateLimiter nullptr");
  }

  if (!initDone)
  {
    grpc::RegisterChannelFilter<GrpcRateLimitChannelData,
      GrpcRateLimitServerCallData>(
      "rate_limit_filter",
      GRPC_SERVER_CHANNEL,
      INT_MAX,
      nullptr);
    initDone = true;
  }

  GrpcRateLimitServerCallData::setRateLimiter(std::move(pRateLimiter));
}

}
