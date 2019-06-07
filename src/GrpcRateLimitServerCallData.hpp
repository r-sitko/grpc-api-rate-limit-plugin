#pragma once

#include <grpc_rate_limit/IRateLimiter.hpp>
#include <src/cpp/common/channel_filter.h>

namespace grpc_rate_limit
{

class GrpcRateLimitServerCallData final : public grpc::CallData
{
public:
  GrpcRateLimitServerCallData();
  ~GrpcRateLimitServerCallData() = default;
  GrpcRateLimitServerCallData(const GrpcRateLimitServerCallData&) = delete;
  GrpcRateLimitServerCallData(GrpcRateLimitServerCallData&&) = delete;
  GrpcRateLimitServerCallData& operator=(const GrpcRateLimitServerCallData&) = delete;
  GrpcRateLimitServerCallData& operator=(GrpcRateLimitServerCallData&&) = delete;

  grpc_error* Init(
    grpc_call_element* elem,
    const grpc_call_element_args* args) final;
  void Destroy(
    grpc_call_element* elem,
    const grpc_call_final_info* final_info,
    grpc_closure *then_call_closure) final;
  void StartTransportStreamOpBatch(
    grpc_call_element* elem,
    grpc::TransportStreamOpBatch* op) final;
  static void setRateLimiter(std::unique_ptr<IRateLimiter> pRateLimiter)
  {
    m_pRateLimiter = std::move(pRateLimiter);
  }

private:
  static void onDoneRecvInitialMetadataCb(void *user_data, grpc_error *error);
  static std::string getMethodName(const grpc_slice& pathSlice);

  grpc_metadata_batch *m_recv_initial_metadata;
  grpc_closure *m_initial_on_done_recv_initial_metadata;
  grpc_closure m_on_done_recv_initial_metadata;
  static std::unique_ptr<IRateLimiter> m_pRateLimiter;
};

}
