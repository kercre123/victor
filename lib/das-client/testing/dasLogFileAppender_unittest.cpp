#pragma GCC diagnostic push

#if __has_warning("-Wundef")
#pragma GCC diagnostic ignored "-Wundef"
#endif

#include "gtest/gtest.h"

#pragma GCC diagnostic pop
#include "dasLogFileAppender.h"
#include "fileUtils.h"
#include "testUtils.h"
#include "stringUtils.h"
#include <stdio.h>
#include <ftw.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sstream>

static const std::string testDirectory("unitTestDasLogs");

namespace Anki {
namespace Das {

class DasLogFileAppenderTest : public testing::Test {
protected:
  virtual void SetUp() {
    rmrf(testDirectory.c_str());
    (void) mkdir(testDirectory.c_str(), 0777);
    testMessage_ = "Some Damned Data";
    appender_ = new Anki::Das::DasLogFileAppender(testDirectory);
  }

  virtual void TearDown() {
    delete appender_; appender_ = nullptr;
    rmrf(testDirectory.c_str());
  }

  std::string StringByDeletingPathExtension(const std::string& path) {
    size_t lastDotPos = path.find_last_of('.');
    return path.substr(0, lastDotPos);
  }

  std::string LastPathComponent(const std::string& path) {
    size_t lastSlashPos = path.find_last_of('/');
    return path.substr(lastSlashPos + 1);
  }

  std::string ExpectedPath(uint32_t logNumber, bool inProgress, bool includeDir) {
    std::ostringstream oss;
    if (includeDir) {
      oss << testDirectory << "/";
    }
    oss << std::setfill('0') << std::setw(4) << logNumber << ".";
    if (inProgress) {
      oss << DasLogFileAppender::kDasInProgressExtension;
    } else {
      oss << DasLogFileAppender::kDasLogFileExtension;
    }
    return oss.str();
  }

public:
  bool ArePathsWithoutExtensionsEqual(const std::string& path1, const std::string& path2) {
    return (0 == StringByDeletingPathExtension(path1).compare(StringByDeletingPathExtension(path2)));
  }

protected:
  Anki::Das::DasLogFileAppender* appender_;
  std::string testMessage_;
};

TEST_F(DasLogFileAppenderTest, GetCurrentLogfile_firstTime) {
  std::string currentLogFilePath = appender_->CurrentLogFilePath();
  std::string expectedFilePath = ExpectedPath(1, true, false);
  ASSERT_EQ(expectedFilePath, LastPathComponent(currentLogFilePath)) << "Should have named the new log file properly";
  ASSERT_TRUE(AnkiUtil::FileExistsAtPath(currentLogFilePath)) << "Should have created an empty log file at the returned path";
}

TEST_F(DasLogFileAppenderTest, WriteData_firstTime) {
  appender_->WriteDataToCurrentLogfile(testMessage_);

  std::string currentLogFile = appender_->CurrentLogFilePath();
  ASSERT_TRUE(AnkiUtil::FileExistsAtPath(currentLogFile)) << "File should exist at current log path";

  appender_->Flush();

  std::string logFileData = AnkiUtil::StringFromContentsOfFile(currentLogFile);
  ASSERT_EQ(testMessage_, logFileData) << "logged data should match test data";
}

TEST_F(DasLogFileAppenderTest, Rollover_oldFile) {
  appender_->WriteDataToCurrentLogfile(testMessage_);
  // now rollover the log to another file
  appender_->RolloverCurrentLogFile();
  // now make sure the file we rolled over was saved
  appender_->Flush();
  std::string completedLogFilePath = ExpectedPath(1, false, true);
  ASSERT_TRUE(AnkiUtil::FileExistsAtPath(completedLogFilePath)) << "Should have moved the old file to a .das file (not in progress)";

  std::string logFileData = AnkiUtil::StringFromContentsOfFile(completedLogFilePath);
  ASSERT_EQ(testMessage_, logFileData) << "The old file should match the data we wrote";
}

TEST_F(DasLogFileAppenderTest, Rollover_newFile) {
  appender_->WriteDataToCurrentLogfile(testMessage_);
  // now rollover the log to another file
  appender_->RolloverCurrentLogFile();
  // now make sure we're using a new file
  appender_->Flush();
  std::string currentLogFilePath = appender_->CurrentLogFilePath();
  std::string expectedName = ExpectedPath(2, true, false);
  ASSERT_EQ(expectedName, LastPathComponent(currentLogFilePath)) << "We should be writing to a new file now";
  ASSERT_TRUE(AnkiUtil::FileExistsAtPath(currentLogFilePath)) << "Our new log file should exist";
}

TEST_F(DasLogFileAppenderTest, Rollover_write) {
  appender_->WriteDataToCurrentLogfile(testMessage_);
  // now rollover the log to another file
  appender_->RolloverCurrentLogFile();
  // now write to the new file.
  std::string newMessage = "New Data";
  appender_->WriteDataToCurrentLogfile(newMessage);
  appender_->Flush();
  std::string logFileData = AnkiUtil::StringFromContentsOfFile(appender_->CurrentLogFilePath());
  ASSERT_EQ(newMessage, logFileData) << "Should have written the new log to the new file";
}

TEST_F(DasLogFileAppenderTest, Rollover_limitNumFiles) {
  for (int i = 0 ; i < Anki::Das::kDasDefaultMaxLogFiles + 1; i++) {
    appender_->WriteDataToCurrentLogfile(testMessage_);
    appender_->RolloverCurrentLogFile();
  }
  appender_->Flush();
  std::string newMessage = "Other wrapped data";
  std::string currentLogFilePath = appender_->CurrentLogFilePath();
  std::string expectedName = ExpectedPath(2, true, false);
  ASSERT_EQ(expectedName, LastPathComponent(currentLogFilePath)) << "Should have named the new log file properly (and wrapped)";

  appender_->WriteDataToCurrentLogfile(newMessage);
  appender_->Flush();
  std::string logFileData = AnkiUtil::StringFromContentsOfFile(appender_->CurrentLogFilePath());
  ASSERT_EQ(newMessage, logFileData) << "Should have written the log (and just the log) to the wrapped log file";
}

TEST_F(DasLogFileAppenderTest, ConsumeLogFiles_empty) {
  appender_->ConsumeLogFiles(std::bind([](const std::string& logFilePath, bool *stop) {
        ADD_FAILURE() << "This shouldn't have been called, we have no log files!";
        return true;
      }, std::placeholders::_1, std::placeholders::_2));
}

TEST_F(DasLogFileAppenderTest, ConsumeLogFiles_oneUnfinishedLog) {
  appender_->WriteDataToCurrentLogfile(testMessage_);
  appender_->ConsumeLogFiles(std::bind([](const std::string& logFilePath, bool *stop) {
        ADD_FAILURE() << "No finished log file, we shouldn't be called";
        return true;
      }, std::placeholders::_1, std::placeholders::_2));
}

TEST_F(DasLogFileAppenderTest, ConsumeLogFiles_oneFinishedLog) {
  int numLogFilesProcessed = 0;
  std::string processedLog;
  std::string processedLogPath;
  appender_->WriteDataToCurrentLogfile(testMessage_);
  std::string testLogFilePath = appender_->CurrentLogFilePath();
  appender_->RolloverCurrentLogFile();
  appender_->Flush();
  appender_->ConsumeLogFiles(std::bind([&](const std::string& logFilePath, bool *stop) {
        EXPECT_TRUE(ArePathsWithoutExtensionsEqual(testLogFilePath, logFilePath)) << "Should be consuming the file we logged to";
        processedLogPath = logFilePath;
        processedLog = AnkiUtil::StringFromContentsOfFile(logFilePath);
        numLogFilesProcessed++;
        return true;
      }, std::placeholders::_1, std::placeholders::_2));
  ASSERT_EQ(1, numLogFilesProcessed) << "Should have only consumed one log file";
  ASSERT_EQ(testMessage_, processedLog) << "Logged data should have been consumed";
  ASSERT_FALSE(AnkiUtil::FileExistsAtPath(processedLogPath)) << "Should have deleted the file we consumed";
}

TEST_F(DasLogFileAppenderTest, ConsumeLogFiles_oneFinishedLogNoDelete) {
  int numLogFilesProcessed = 0;
  std::string processedLog;
  std::string processedLogPath;
  appender_->WriteDataToCurrentLogfile(testMessage_);
  std::string testLogFilePath = appender_->CurrentLogFilePath();
  appender_->RolloverCurrentLogFile();
  appender_->Flush();
  appender_->ConsumeLogFiles(std::bind([&](const std::string& logFilePath, bool *stop) {
        EXPECT_TRUE(ArePathsWithoutExtensionsEqual(testLogFilePath, logFilePath)) << "Should be consuming the file we logged to";
        processedLogPath = logFilePath;
        processedLog = AnkiUtil::StringFromContentsOfFile(logFilePath);
        numLogFilesProcessed++;
        return false;
      }, std::placeholders::_1, std::placeholders::_2));
  ASSERT_EQ(1, numLogFilesProcessed) << "Should have only consumed one log file";
  ASSERT_EQ(testMessage_, processedLog) << "Logged data should have been consumed";
  ASSERT_TRUE(AnkiUtil::FileExistsAtPath(processedLogPath)) << "Should NOT have deleted the file we consumed";
}

TEST_F(DasLogFileAppenderTest, ConsumeLogFiles_twoFinishedLogs) {
  int numLogFilesProcessed = 0;
  appender_->WriteDataToCurrentLogfile(testMessage_);
  appender_->RolloverCurrentLogFile(); // 1
  appender_->WriteDataToCurrentLogfile(testMessage_);
  appender_->RolloverCurrentLogFile(); // 2
  appender_->Flush();
  appender_->ConsumeLogFiles(std::bind([&](const std::string& logFilePath, bool *stop) {
        numLogFilesProcessed++;
        return true;
      }, std::placeholders::_1, std::placeholders::_2));
  ASSERT_EQ(2, numLogFilesProcessed) << "Should have consumed two log files";
  // try it again, all logs should be gone.
  appender_->ConsumeLogFiles(std::bind([&](const std::string& logFilePath, bool *stop) {
        numLogFilesProcessed++;
        return true;
      }, std::placeholders::_1, std::placeholders::_2));
  ASSERT_EQ(2, numLogFilesProcessed) << "Should have consumed two log files";
}

TEST_F(DasLogFileAppenderTest, ConsumeLogFiles_twoFinishedLogsNoDelete) {
  int numLogFilesProcessed = 0;
  appender_->WriteDataToCurrentLogfile(testMessage_);
  appender_->RolloverCurrentLogFile(); // 1
  appender_->WriteDataToCurrentLogfile(testMessage_);
  appender_->RolloverCurrentLogFile(); // 2
  appender_->Flush();
  appender_->ConsumeLogFiles(std::bind([&](const std::string& logFilePath, bool *stop) {
        numLogFilesProcessed++;
        return false;
      }, std::placeholders::_1, std::placeholders::_2));
  ASSERT_EQ(2, numLogFilesProcessed) << "Should have consumed two log files";
  // try it again, all logs should be gone.
  appender_->ConsumeLogFiles(std::bind([&](const std::string& logFilePath, bool *stop) {
        numLogFilesProcessed++;
        return true;
      }, std::placeholders::_1, std::placeholders::_2));
  ASSERT_EQ(4, numLogFilesProcessed) << "Should have consumed four log files";
}

TEST_F(DasLogFileAppenderTest, WriteToLog_autoRollover) {
  appender_->SetMaxLogLength(20);
  std::string testMessage = "SomeDamnedData\n";
  appender_->WriteDataToCurrentLogfile(testMessage);
  appender_->WriteDataToCurrentLogfile(testMessage);
  appender_->Flush();

  std::string completedLogFilePath = ExpectedPath(1, false, true);
  ASSERT_TRUE(AnkiUtil::FileExistsAtPath(completedLogFilePath)) << "Should have moved the old file to a .das file (not in progress)";
  std::string verificationData = AnkiUtil::StringFromContentsOfFile(completedLogFilePath);
  ASSERT_EQ(testMessage, verificationData) << "The old file should match the data we wrote";

  std::string inProgressLogFilePath = ExpectedPath(2, true, true);
  ASSERT_TRUE(AnkiUtil::FileExistsAtPath(inProgressLogFilePath)) << "Should have logged the second bind of data to the new file (in progress)";
  verificationData = AnkiUtil::StringFromContentsOfFile(inProgressLogFilePath);
  ASSERT_EQ(testMessage, verificationData) << "The in progress file should match the data we wrote";
}

TEST_F(DasLogFileAppenderTest, Init_resumeInProgress) {
  std::string testMessage = "SomeDamnedData\n";
  appender_->WriteDataToCurrentLogfile(testMessage);
  appender_->RolloverCurrentLogFile();
  appender_->WriteDataToCurrentLogfile(testMessage);
  appender_->Flush();
  delete appender_; appender_ = nullptr;
  appender_ = new Anki::Das::DasLogFileAppender(testDirectory);
  std::string inProgressLog = appender_->CurrentLogFilePath();
  std::string expectedName = ExpectedPath(3, true, false);
  ASSERT_EQ(expectedName, LastPathComponent(inProgressLog)) << "wrong resumed file!";

  appender_->WriteDataToCurrentLogfile(testMessage);
  appender_->Flush();
  std::string verificationData = testMessage;

  std::string dataInFile = AnkiUtil::StringFromContentsOfFile(appender_->CurrentLogFilePath());
  ASSERT_EQ(verificationData, dataInFile) << "We should have actually started a new in-progress log";
}

TEST_F(DasLogFileAppenderTest, Init_rolloverAfterResume) {
  std::string testMessage = "SomeDamnedData\n";
  appender_->WriteDataToCurrentLogfile(testMessage);
  appender_->RolloverCurrentLogFile();
  appender_->WriteDataToCurrentLogfile(testMessage);
  appender_->Flush();
  delete appender_; appender_ = nullptr;
  appender_ = new Anki::Das::DasLogFileAppender(testDirectory);
  appender_->RolloverCurrentLogFile();
  std::string inProgressLog = appender_->CurrentLogFilePath();
  std::string expectedName = ExpectedPath(3, true, false);
  ASSERT_EQ(expectedName, LastPathComponent(inProgressLog));
}

} // namespace Das
} // namespace Anki
