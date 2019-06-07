#pragma once

#include <src/cpp/common/channel_filter.h>

namespace grpc_rate_limit
{

class GrpcRateLimitChannelData : public grpc::ChannelData
{
public:
  GrpcRateLimitChannelData() = default;
  ~GrpcRateLimitChannelData() = default;
  GrpcRateLimitChannelData(const GrpcRateLimitChannelData&) = delete;
  GrpcRateLimitChannelData(GrpcRateLimitChannelData&&) = delete;
  GrpcRateLimitChannelData& operator=(const GrpcRateLimitChannelData&) = delete;
  GrpcRateLimitChannelData& operator=(GrpcRateLimitChannelData&&) = delete;

  grpc_error* Init(
    grpc_channel_element* elem,
    grpc_channel_element_args* args) override
  {
    (void)elem;
    (void)args;
    return GRPC_ERROR_NONE;
  }
};

}
