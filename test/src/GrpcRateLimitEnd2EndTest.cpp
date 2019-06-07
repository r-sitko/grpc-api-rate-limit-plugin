#include <grpc_rate_limit/GrpcRateLimitPlugin.hpp>
#include <grpc_rate_limit/RateLimiter.hpp>
#include <grpc_rate_limit/TokenBucket.hpp>
#include "pingpong.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>
#include <tuple>
#include <future>

namespace
{
using namespace pingpong;
using namespace ::testing;

class PingPongServiceImpl final : public PingPong::Service
{
  grpc::Status ping(
    grpc::ServerContext* context,
    const PingRequest* request,
    PongReply* reply) override
  {
    (void)context;
    std::string postfix(" Pong");
    reply->set_data(request->data() + postfix);
    return grpc::Status::OK;
  }

  grpc::Status ping2(
    grpc::ServerContext* context,
    const PingRequest* request,
    PongReply* reply) override
  {
    (void)context;
    std::string postfix(" Pong");
    reply->set_data(request->data() + postfix);
    return grpc::Status::OK;
  }
};

class PingPongClient
{
public:
  explicit PingPongClient(
    const std::shared_ptr<grpc::Channel>& channel)
    : m_pStub(PingPong::NewStub(channel))
  {}

  std::tuple<std::string, grpc::Status> ping(const std::string& textToSend)
  {
    PingRequest request;
    request.set_data(textToSend);

    PongReply reply;
    grpc::ClientContext context;

    auto status = m_pStub->ping(&context, request, &reply);

    return {reply.data(), status};
  }

  std::tuple<std::string, grpc::Status> ping2(const std::string& textToSend)
  {
    PingRequest request;
    request.set_data(textToSend);

    PongReply reply;
    grpc::ClientContext context;

    auto status = m_pStub->ping2(&context, request, &reply);

    return {reply.data(), status};
  }

private:
  std::unique_ptr<PingPong::Stub> m_pStub;
};

class GrpcRateLimitEnd2EndTest : public Test
{
protected:
  void SetUp() override
  {
    std::string serverAddress("0.0.0.0:50051");

    auto pRateLimiter = std::make_unique<grpc_rate_limit::RateLimiter>();
    auto pTokenBucket = std::make_unique<grpc_rate_limit::TokenBucket>(
      m_kMaxTokensInBucket,
      m_kFillIntervalMs,
      m_kTokensRefreshAmount);
    pRateLimiter->addRateLimit("/" + std::string(PingPong::service_full_name()) + "/ping", std::move(pTokenBucket));

    grpc_rate_limit::GrpcRateLimitPlugin::registerGrpcRateLimitPlugin(std::move(pRateLimiter));

    m_pServer = grpc::ServerBuilder()
      .AddListeningPort(serverAddress, grpc::InsecureServerCredentials())
      .RegisterService(&m_greeterService)
      .BuildAndStart();

    m_serverThread = std::thread([this]{m_pServer->Wait();});

    m_pClient = std::make_unique<PingPongClient>(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));
  }

  void TearDown() override
  {
    m_pServer->Shutdown();
    if (m_serverThread.joinable())
    {
      m_serverThread.join();
    }
  }

  void sendSuccessfullyPings(
    std::shared_ptr<PingPongClient> pClient,
    uint32_t requestsNr)
  {
    for (uint32_t callNr = 0; callNr < requestsNr; ++callNr)
    {
      auto [replyData, status] = pClient->ping("Ping");
      ASSERT_TRUE(status.ok())
        << "Attempt number: " << callNr;
      ASSERT_EQ(replyData, "Ping Pong")
        << "Attempt number: " << callNr;
    }
  }

  void sendSuccessfullyPingsRev2(
    std::shared_ptr<PingPongClient> pClient,
    uint32_t requestsNr)
  {
    for (uint32_t callNr = 0; callNr < requestsNr; ++callNr)
    {
      auto [replyData, status] = pClient->ping2("Ping");
      ASSERT_TRUE(status.ok())
        << "Attempt number: " << callNr;
      ASSERT_EQ(replyData, "Ping Pong")
        << "Attempt number: " << callNr;
    }
  }

  void sendUnsuccessfullyPings(
    std::shared_ptr<PingPongClient> pClient,
    uint32_t requestsNr)
  {
    for (uint32_t callNr = 0; callNr < requestsNr; ++callNr)
    {
      auto [replyData, status] = pClient->ping("Ping");
      ASSERT_FALSE(status.ok())
        << "Attempt number: " << callNr;
      ASSERT_EQ(status.error_code(), 8)
        << "Attempt number: " << callNr;
      ASSERT_EQ(status.error_message(), "Too many requests.")
        << "Attempt number: " << callNr;
    }
  }

  std::unique_ptr<grpc::Server> m_pServer;
  PingPongServiceImpl m_greeterService;
  std::thread m_serverThread;
  std::shared_ptr<PingPongClient> m_pClient;

  const uint64_t m_kMaxTokensInBucket = 10;
  const uint64_t m_kFillIntervalMs = 100;
  const uint64_t m_kTokensRefreshAmount = 10;
};

TEST_F(GrpcRateLimitEnd2EndTest, RateLimitAndRefreshMultipleClients)
{
  std::vector<std::shared_ptr<PingPongClient>> testClients = {
    m_pClient,
    std::make_unique<PingPongClient>(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials())),
    std::make_unique<PingPongClient>(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials())),
    std::make_unique<PingPongClient>(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials())),
    std::make_unique<PingPongClient>(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()))
  };
  const uint32_t kRateLimitCalls = m_kMaxTokensInBucket / testClients.size();
  std::vector<std::future<void>> futures;

  for (auto& pClient : testClients)
  {
    futures.emplace_back(
      std::async([&]{sendSuccessfullyPings(pClient, kRateLimitCalls);}));
  }

  for (auto& future : futures)
  {
    future.get();
  }

  futures.clear();

  for (auto& pClient : testClients)
  {
    futures.emplace_back(
      std::async([&]{sendUnsuccessfullyPings(pClient, 2);}));
  }

  for (auto& future : futures)
  {
    future.get();
  }

  futures.clear();

  std::this_thread::sleep_for(std::chrono::milliseconds(m_kFillIntervalMs));

  for (auto& pClient : testClients)
  {
    futures.emplace_back(
      std::async([&]{sendSuccessfullyPings(pClient, kRateLimitCalls);}));
  }

  for (auto& future : futures)
  {
    future.get();
  }

  futures.clear();

  for (auto& pClient : testClients)
  {
    futures.emplace_back(
      std::async([&]{sendUnsuccessfullyPings(pClient, 2);}));
  }

  for (auto& future : futures)
  {
    future.get();
  }

  futures.clear();
}

TEST_F(GrpcRateLimitEnd2EndTest, RateLimitAndRefreshOneClient)
{
  const uint32_t kRateLimitCalls = m_kMaxTokensInBucket;

  sendSuccessfullyPings(m_pClient, kRateLimitCalls);

  sendUnsuccessfullyPings(m_pClient, 2);

  std::this_thread::sleep_for(std::chrono::milliseconds(m_kFillIntervalMs));

  sendSuccessfullyPings(m_pClient, m_kTokensRefreshAmount);

  sendUnsuccessfullyPings(m_pClient, 2);
}

TEST_F(GrpcRateLimitEnd2EndTest, NoLimitForNotRegisteredMethod)
{
  sendSuccessfullyPingsRev2(m_pClient, m_kMaxTokensInBucket + 10);
}

} // namespace
