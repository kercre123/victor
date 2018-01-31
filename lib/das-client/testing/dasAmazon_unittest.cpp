#include "dasAppender.h"
#pragma GCC diagnostic push

#if __has_warning("-Wundef")
#pragma GCC diagnostic ignored "-Wundef"
#endif

#include "gtest/gtest.h"

#pragma GCC diagnostic pop

#include "dasLogFileAppender.h"
#include "testUtils.h"
#include "dasPostToServer.h"
#include <unistd.h>
#include <stdio.h>

static const std::string testDASLogsDir("./dasTestLogs");
static const std::string testDASDir("./dasLogs");
static const std::string testDASURL("https://sqs.us-west-2.amazonaws.com/792379844846/DasInternal-dasinternalSqs-1HN6JX3NZPGNT");

TEST(DasAmazonTest, DasAppenderFlushWithCallback) {
  (void) rmrf(testDASDir.c_str());
  Anki::Das::DasAppender* testAppender = new Anki::Das::DasAppender(testDASDir, testDASURL);
  testAppender->SetIsUploadingPaused(false);
  copy_file(testDASLogsDir + "/1_working_upload.das", testDASDir + "/0001.das");

  bool finished = false;
  std::mutex mutex;
  std::unique_lock<std::mutex> lock{mutex};
  std::condition_variable cv;
  auto const completionFunc = [&finished, &cv, &mutex](const bool lastFlushSucceeded, const std::string lastFlushResponse) {
    {
      std::lock_guard<std::mutex> lg{mutex};
      finished = true;
    }
    EXPECT_TRUE(lastFlushResponse.find("MessageId")) <<  "Bad Response";
    EXPECT_EQ(lastFlushSucceeded, true) <<  "Was not able to roll over file";
    cv.notify_one();
  };

  testAppender->ForceFlushWithCallback(completionFunc);
  cv.wait(lock, [&finished] { return finished; });

  delete testAppender; testAppender = nullptr;
  (void) rmrf(testDASDir.c_str());
}
