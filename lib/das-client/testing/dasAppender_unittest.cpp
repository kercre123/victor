#include "dasAppender.h"
#pragma GCC diagnostic push

#if __has_warning("-Wundef")
#pragma GCC diagnostic ignored "-Wundef"
#endif

#include "gtest/gtest.h"

#pragma GCC diagnostic pop

#include "dasLogFileAppender.h"
#include "testUtils.h"
#include <unistd.h>

static const std::string testDASDir("./test-dasLogs");
static const std::string testDASURL("http://localhost:6099/event");

TEST(DasAppenderTest, DasAppenderConstructor) {
  Anki::Das::DasAppender* testAppender = new Anki::Das::DasAppender(testDASDir,testDASURL);
  EXPECT_EQ(0, access(testDASDir.c_str(), F_OK)) << testDASDir << " should have been created";
  delete testAppender; testAppender = nullptr;
  (void) rmrf(testDASDir.c_str());
}

TEST(DasAppenderTest, DasAppenderCleanupOldEmptyInProgress) {
  (void) rmrf(testDASDir.c_str());
  bool mkdirSuccess = (0 == mkdir(testDASDir.c_str(), S_IRWXU)) || (EEXIST == errno);
  EXPECT_TRUE(mkdirSuccess);
  std::string inProg2 = testDASDir + "/02." + Anki::Das::DasLogFileAppender::kDasInProgressExtension;
  std::string inProg3 = testDASDir + "/03." + Anki::Das::DasLogFileAppender::kDasInProgressExtension;
  EXPECT_EQ(0, truncate_to_zero(inProg2.c_str()));
  EXPECT_EQ(0, truncate_to_zero(inProg3.c_str()));
  EXPECT_EQ(0, access(inProg2.c_str(), F_OK)) << inProg2 << " should have been created";
  EXPECT_EQ(0, access(inProg3.c_str(), F_OK)) << inProg3 << " should have been created";
  Anki::Das::DasAppender* testAppender = new Anki::Das::DasAppender(testDASDir,testDASURL);
  EXPECT_EQ(0, access(inProg2.c_str(), F_OK)) << inProg2 << " should not be deleted yet";
  EXPECT_EQ(0, access(inProg3.c_str(), F_OK)) << inProg3 << " should not be deleted yet";
  testAppender->SetIsUploadingPaused(false);
  delete testAppender; testAppender = nullptr;
  EXPECT_EQ(-1, access(inProg2.c_str(), F_OK)) << inProg2 << " should have been rolled over";
  EXPECT_EQ(-1, access(inProg3.c_str(), F_OK)) << inProg3 << " should have been rolled over";
  (void) rmrf(testDASDir.c_str());
}

TEST(DasAppenderTest, DasAppenderCleanupOldNonEmptyInProgress) {
  (void) rmrf(testDASDir.c_str());
  bool mkdirSuccess = (0 == mkdir(testDASDir.c_str(), S_IRWXU)) || (EEXIST == errno);
  EXPECT_TRUE(mkdirSuccess);
  std::string inProg2 = testDASDir + "/02." + Anki::Das::DasLogFileAppender::kDasInProgressExtension;
  std::string inProg3 = testDASDir + "/03." + Anki::Das::DasLogFileAppender::kDasInProgressExtension;
  EXPECT_EQ(0, write_string_to_file(inProg2.c_str(), "some data"));
  EXPECT_EQ(0, write_string_to_file(inProg3.c_str(), "some other data"));
  EXPECT_EQ(0, access(inProg2.c_str(), F_OK)) << inProg2 << " should have been created";
  EXPECT_EQ(0, access(inProg3.c_str(), F_OK)) << inProg3 << " should have been created";
  Anki::Das::DasAppender* testAppender = new Anki::Das::DasAppender(testDASDir,testDASURL);
  EXPECT_EQ(0, access(inProg2.c_str(), F_OK)) << inProg2 << " should not be deleted yet";
  EXPECT_EQ(0, access(inProg3.c_str(), F_OK)) << inProg3 << " should not be deleted yet";
  testAppender->SetIsUploadingPaused(false);
  delete testAppender; testAppender = nullptr;
  EXPECT_EQ(-1, access(inProg2.c_str(), F_OK)) << inProg2 << " should have been rolled over";
  EXPECT_EQ(-1, access(inProg3.c_str(), F_OK)) << inProg3 << " should have been rolled over";
  (void) rmrf(testDASDir.c_str());
}

TEST(DasAppenderTest, DasAppenderFlushWithCallback) {
  (void) rmrf(testDASDir.c_str());
  Anki::Das::DasAppender* testAppender = new Anki::Das::DasAppender(testDASDir, testDASURL);
  std::map<std::string, std::string> tempMap;
  testAppender->append(DASLogLevel_Info, "asdf", "asdf", 0, "asdf", "asdf", 0, &tempMap, tempMap);

  bool finished = false;
  std::mutex mutex;
  std::unique_lock<std::mutex> lock{mutex};
  std::condition_variable cv;
  auto const completionFunc = [&finished, &cv, &mutex](const bool lastFlushFailed, std::string response) {
    {
      (void) response;
      std::lock_guard<std::mutex> lg{mutex};
      finished = true;
    }
    cv.notify_one();
  };

  testAppender->ForceFlushWithCallback(completionFunc);
  cv.wait(lock, [&finished] { return finished; });

  delete testAppender; testAppender = nullptr;

  (void) rmrf(testDASDir.c_str());
}
