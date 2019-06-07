#include <grpc_rate_limit/RateLimiter.hpp>
#include <grpc_rate_limit/TokenBucket.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <string>

namespace
{
using namespace ::testing;

class RateLimiterTest : public Test
{
protected:
  void SetUp() override
  {
    m_pRateLimiter = std::make_shared<grpc_rate_limit::RateLimiter>();
    auto pTokenBucket = std::make_shared<grpc_rate_limit::TokenBucket>(
      m_kMaxTokensInBucket,
      m_kFillIntervalMs,
      m_kTokensRefreshAmount);
    m_pRateLimiter->addRateLimit(m_kMethod, pTokenBucket);
    auto pTokenBucket2 = std::make_shared<grpc_rate_limit::TokenBucket>(
      m_kMaxTokensInBucket2,
      m_kFillIntervalMs2,
      m_kTokensRefreshAmount2);
    m_pRateLimiter->addRateLimit(m_kMethod2, pTokenBucket2);
  }

  void shouldNoLimit(const std::string& method, uint32_t maxRetries)
  {
    for (uint32_t i = 0; i < maxRetries; ++i)
    {
      ASSERT_FALSE(m_pRateLimiter->shouldLimit(method))
        << "Attempt number: " << i << " method: " << method;
    }
  }

  std::shared_ptr<grpc_rate_limit::RateLimiter> m_pRateLimiter;

  const std::string m_kMethod = "call";
  const uint64_t m_kMaxTokensInBucket = 20;
  const uint64_t m_kFillIntervalMs = 100;
  const uint64_t m_kTokensRefreshAmount = 10;

  const std::string m_kMethod2 = "call2";
  const uint64_t m_kMaxTokensInBucket2 = 10;
  const uint64_t m_kFillIntervalMs2 = 2 * m_kFillIntervalMs;
  const uint64_t m_kTokensRefreshAmount2 = 5;
  const uint64_t m_kAdditionalRefreshTimeMs = 10;
};

TEST_F(RateLimiterTest, ThrowExceptionWhenDoubleRegisteringMethod)
{
  auto pTokenBucket = std::make_shared<grpc_rate_limit::TokenBucket>(2, 1, 2);
  ASSERT_THROW(m_pRateLimiter->addRateLimit(m_kMethod, pTokenBucket), std::runtime_error);
}

TEST_F(RateLimiterTest, ThrowExceptionWhenTokenBucketIsNullptr)
{
  ASSERT_THROW(m_pRateLimiter->addRateLimit(m_kMethod, nullptr), std::invalid_argument);
}

TEST_F(RateLimiterTest, ConsumeAndWaitForRefresh)
{
  shouldNoLimit(m_kMethod, m_kMaxTokensInBucket);
  ASSERT_TRUE(m_pRateLimiter->shouldLimit(m_kMethod));

  shouldNoLimit(m_kMethod2, m_kMaxTokensInBucket2);
  ASSERT_TRUE(m_pRateLimiter->shouldLimit(m_kMethod2));

  std::this_thread::sleep_for(
    std::chrono::milliseconds(m_kFillIntervalMs + m_kAdditionalRefreshTimeMs));
  shouldNoLimit(m_kMethod, m_kTokensRefreshAmount);
  ASSERT_TRUE(m_pRateLimiter->shouldLimit(m_kMethod));
  ASSERT_TRUE(m_pRateLimiter->shouldLimit(m_kMethod2));

  std::this_thread::sleep_for(
    std::chrono::milliseconds(m_kFillIntervalMs + m_kAdditionalRefreshTimeMs));
  shouldNoLimit(m_kMethod2, m_kTokensRefreshAmount2);
  ASSERT_TRUE(m_pRateLimiter->shouldLimit(m_kMethod2));
}

TEST_F(RateLimiterTest, CheckIfMaximumAmountOfTokensIskMaxTokensInBucket)
{
  std::this_thread::sleep_for(
    std::chrono::milliseconds(m_kFillIntervalMs2 + m_kAdditionalRefreshTimeMs));

  shouldNoLimit(m_kMethod, m_kMaxTokensInBucket);
  ASSERT_TRUE(m_pRateLimiter->shouldLimit(m_kMethod));

  shouldNoLimit(m_kMethod2, m_kMaxTokensInBucket2);
  ASSERT_TRUE(m_pRateLimiter->shouldLimit(m_kMethod2));
}

TEST_F(RateLimiterTest, NoLimitForNotRegisteredMethod)
{
  shouldNoLimit("xyz", m_kMaxTokensInBucket + 10);
}

} // namespace
