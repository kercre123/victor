//
//  testDASUtils.cpp
//
//  Copyright (c) 2018 Anki Inc. All rights reserved.
//

#include "util/helpers/includeGTest.h"
#include "util/logging/DAS.h"
#include <thread>

namespace Anki {
namespace Util {
namespace Test {

class TestDASUtils : public ::testing::Test
{
};

TEST_F(TestDASUtils, TestUptimeMS)
{
  using namespace Anki::Util::DAS;

  // Count should be non-decreasing
  const auto t1 = UptimeMS();
  const auto t2 = UptimeMS();
  ASSERT_LE(t1, t2);

  // Count should increase over time
  const auto t3 = UptimeMS();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  const auto t4 = UptimeMS();
  ASSERT_LT(t3, t4);

}

TEST_F(TestDASUtils, TestEscape)
{
  using namespace Anki::Util::DAS;

  // Null string should become empty string
  EXPECT_EQ("", Escape(nullptr));

  // Empty string should remain empty string
  EXPECT_EQ("", Escape(""));

  // Generic text should remain generic text
  EXPECT_EQ("abc", Escape("abc"));

  // Embedded quote should be escaped
  EXPECT_EQ("abc\\\"def", Escape("abc\"def"));

  // Embedded backslash should be escaped
  EXPECT_EQ("abc\\\\def", Escape("abc\\def"));

  // Embedded newline should be escaped
  EXPECT_EQ("abc\\ndef", Escape("abc\ndef"));

}

} // namespace Test
} // namespace Util
} // namespace Anki
