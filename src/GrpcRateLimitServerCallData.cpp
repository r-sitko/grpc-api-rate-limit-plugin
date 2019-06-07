#include "GrpcRateLimitServerCallData.hpp"

namespace grpc_rate_limit
{

std::unique_ptr<IRateLimiter> GrpcRateLimitServerCallData::m_pRateLimiter = nullptr;

GrpcRateLimitServerCallData::GrpcRateLimitServerCallData()
  : m_recv_initial_metadata{nullptr},
    m_initial_on_done_recv_initial_metadata{nullptr}
{
}

grpc_error* GrpcRateLimitServerCallData::Init(
  grpc_call_element* elem,
  const grpc_call_element_args* args)
{
  (void)args;
  GRPC_CLOSURE_INIT(&m_on_done_recv_initial_metadata,
    onDoneRecvInitialMetadataCb,
    elem,
    grpc_schedule_on_exec_ctx);
  return GRPC_ERROR_NONE;
}

void GrpcRateLimitServerCallData::Destroy(
  grpc_call_element* elem,
  const grpc_call_final_info* final_info,
  grpc_closure *then_call_closure)
{
  (void)elem;
  (void)final_info;
  (void)then_call_closure;
}

void GrpcRateLimitServerCallData::StartTransportStreamOpBatch(
  grpc_call_element* elem,
  grpc::TransportStreamOpBatch* op)
{
  if (op->recv_initial_metadata())
  {
    m_recv_initial_metadata = op->recv_initial_metadata()->batch();
    m_initial_on_done_recv_initial_metadata = op->recv_initial_metadata_ready();
    op->set_recv_initial_metadata_ready(&m_on_done_recv_initial_metadata);
  }
  grpc_call_next_op(elem, op->op());
}

void GrpcRateLimitServerCallData::onDoneRecvInitialMetadataCb(
  void* user_data,
  grpc_error* error)
{
  grpc_call_element* elem = static_cast<grpc_call_element*>(user_data);
  GrpcRateLimitServerCallData* calld =
    static_cast<GrpcRateLimitServerCallData*>(elem->call_data);
  GPR_ASSERT(calld);

  if (GRPC_ERROR_NONE == error)
  {
    grpc_metadata_batch* initial_metadata = calld->m_recv_initial_metadata;
    GPR_ASSERT(initial_metadata);
    if(initial_metadata->idx.named.path)
    {
      grpc_slice pathSlice;
      pathSlice = GRPC_MDVALUE(initial_metadata->idx.named.path->md);
      auto methodName = getMethodName(pathSlice);
      if (m_pRateLimiter->shouldLimit(methodName))
      {
        error = grpc_error_set_int(
          GRPC_ERROR_CREATE_FROM_STATIC_STRING(
            "Too many requests."),
            GRPC_ERROR_INT_GRPC_STATUS,
            GRPC_STATUS_RESOURCE_EXHAUSTED);
      }
    }
  }

  GRPC_CLOSURE_RUN(calld->m_initial_on_done_recv_initial_metadata, (error));
}

std::string GrpcRateLimitServerCallData::getMethodName(
  const grpc_slice& pathSlice)
{
  if (GRPC_SLICE_IS_EMPTY(pathSlice))
  {
    return "";
  }
  return std::string(reinterpret_cast<const char*>(
    GRPC_SLICE_START_PTR(pathSlice)),
    GRPC_SLICE_LENGTH(pathSlice));
}

}
