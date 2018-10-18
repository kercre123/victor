#pragma GCC diagnostic push

#if __has_warning("-Wundef")
#pragma GCC diagnostic ignored "-Wundef"
#endif

#include "gtest/gtest.h"

#pragma GCC diagnostic pop
#include "dasGameLogAppender.h"
#include "dasGlobals.h"
#include "portableTypes.h"
#include "DASPrivate.h"
#include "stringUtils.h"
#include "testUtils.h"
#include <ftw.h>
#include <uuid/uuid.h>

static const std::string testDirectory("unitTestGameLogs");

namespace Anki {
namespace Das {

class DasGameLogAppenderTest : public testing::Test {
protected:
  virtual void SetUp() {
    this_id_ = 57;
    rmrf(testDirectory.c_str());
    (void) mkdir(testDirectory.c_str(), 0777);
    uuid_t gameUUID;
    uuid_generate(gameUUID);
    uuid_string_t gameID;
    uuid_unparse_upper(gameUUID, gameID);
    globals_[kGameIdGlobalKey] = gameID;
    logDir_ = testDirectory + "/" + gameID;

    appender_ = new Anki::Das::DasGameLogAppender(testDirectory.c_str(), gameID);
  }

  virtual void TearDown() {
    delete appender_; appender_ = nullptr;
    rmrf(testDirectory.c_str());
  }

  std::string StringFromContentsOfLogFile() {
    std::string logFilePath = logDir_ + "/" + DasGameLogAppender::kLogFileName;
    std::string logFileContents = AnkiUtil::StringFromContentsOfFile(logFilePath);
    return logFileContents;
  }

  std::string ExpectedLog(const char* eventName,
                          const char* eventValue,
                          bool first) {
    return ExpectedLog(eventName, eventValue, nullptr, first);
  }

  std::string ExpectedLog(const char* eventName,
                          const char* eventValue,
                          const char* physValue,
                          bool first) {
    std::ostringstream logStream;
    std::string phys;

    if (first) {
      logStream << DasGameLogAppender::kCsvHeaderRow << std::endl;
    }

    if (nullptr != physValue) {
      phys = physValue;
    }

    logStream << "," << this_id_ << ",," << eventName << ",";
    logStream << eventValue << ",," << phys  << ",,,,,," << globals_[kGameIdGlobalKey] << ",,," ;
    logStream << kMessageVersionGlobalValue << std::endl;

    return logStream.str();
  }

protected:
  ThreadId_t this_id_;
  std::string logDir_;
  std::map<std::string, std::string> globals_;
  Anki::Das::DasGameLogAppender* appender_;
};

TEST_F(DasGameLogAppenderTest, EscapeCsvValueNone) {
  std::string empty = "";
  std::string actual = empty;
  appender_->escapeStringForCsv(actual);
  EXPECT_EQ(empty, actual);

  std::string simple = "This is a simple string";
  actual = simple;
  appender_->escapeStringForCsv(actual);
  EXPECT_EQ(simple, actual);
}

TEST_F(DasGameLogAppenderTest, EscapeCsvDoubleQuotes) {
  std::string hasDoubleQuote = "I have a \" in me";
  std::string actual = hasDoubleQuote;
  std::string expected = "\"I have a \"\" in me\"";
  appender_->escapeStringForCsv(actual);
  EXPECT_EQ(expected, actual);

  hasDoubleQuote = "\"I have a double quote at the front of me";
  actual = hasDoubleQuote;
  expected = "\"\"\"I have a double quote at the front of me\"";
  appender_->escapeStringForCsv(actual);
  EXPECT_EQ(expected, actual);

  hasDoubleQuote = "I have a double quote at the end of me\"";
  actual = hasDoubleQuote;
  expected = "\"I have a double quote at the end of me\"\"\"";
  appender_->escapeStringForCsv(actual);
  EXPECT_EQ(expected, actual);

  hasDoubleQuote = "I have two double quotes \"\" right next to each other";
  actual = hasDoubleQuote;
  expected = "\"I have two double quotes \"\"\"\" right next to each other\"";
  appender_->escapeStringForCsv(actual);
  EXPECT_EQ(expected, actual);

  hasDoubleQuote = "I have \"lots of \" all over me\"";
  actual = hasDoubleQuote;
  expected = "\"I have \"\"lots of \"\" all over me\"\"\"";
  appender_->escapeStringForCsv(actual);
  EXPECT_EQ(expected, actual);
}

TEST_F(DasGameLogAppenderTest, EscapeCsvEscapables) {
  std::string escapeMe = ",";
  std::string actual = escapeMe;
  std::string expected ="\",\"";
  appender_->escapeStringForCsv(actual);
  EXPECT_EQ(expected, actual);

  actual = escapeMe = "I bought eggs, apples, and ham.";
  expected = "\"I bought eggs, apples, and ham.\"";
  appender_->escapeStringForCsv(actual);
  EXPECT_EQ(expected, actual);

  actual = escapeMe = "This line ends in a comma,";
  expected = "\"This line ends in a comma,\"";
  appender_->escapeStringForCsv(actual);
  EXPECT_EQ(expected, actual);


  actual = escapeMe = "\r";
  expected = "\"\r\"";
  appender_->escapeStringForCsv(actual);
  EXPECT_EQ(expected, actual);

  actual = escapeMe = "I have many \r within me \r and oh yeah \r";
  expected = "\"I have many \r within me \r and oh yeah \r\"";
  appender_->escapeStringForCsv(actual);
  EXPECT_EQ(expected, actual);

  actual = escapeMe = "\n";
  expected = "\"\n\"";
  appender_->escapeStringForCsv(actual);
  EXPECT_EQ(expected, actual);

  actual = escapeMe = "I have many \n within me \n and oh yeah \n";
  expected = "\"I have many \n within me \n and oh yeah \n\"";
  appender_->escapeStringForCsv(actual);
  EXPECT_EQ(expected, actual);
}

TEST_F(DasGameLogAppenderTest, EscapeCsvTorture) {
  std::string escapeMe = "I went to the \"the store\"\r\n and \"bought\" apples,\r\n,,ham,,\r\n,eggs,,,\"ok\"\n";
  std::string actual = escapeMe;
  std::string expected = "\"I went to the \"\"the store\"\"\r\n and \"\"bought\"\" apples,\r\n,,ham,,\r\n,eggs,,,\"\"ok\"\"\n\"";
  appender_->escapeStringForCsv(actual);
  EXPECT_EQ(expected, actual);
}

TEST_F(DasGameLogAppenderTest, WriteOneMessage) {
  std::map<std::string, std::string> data;

  appender_->append(DASLogLevel_Info, "TestEvent", "TestValue", this_id_, nullptr, nullptr, 0, &globals_, data);
  appender_->flush();
  std::string expectedData = ExpectedLog("TestEvent", "TestValue", true);
  std::string verificationData = StringFromContentsOfLogFile();
  ASSERT_EQ(expectedData, verificationData) << "The log file should match the data we wrote";
}

TEST_F(DasGameLogAppenderTest, WriteTwoMessages) {
  std::map<std::string, std::string> data;

  appender_->append(DASLogLevel_Info, "TestEvent1", "TestValue1", this_id_, nullptr, nullptr, 0, &globals_, data);
  appender_->append(DASLogLevel_Info, "TestEvent2", "TestValue2", this_id_, nullptr, nullptr, 0, &globals_, data);
  appender_->flush();
  std::string expectedData = ExpectedLog("TestEvent1", "TestValue1", true)
    + ExpectedLog("TestEvent2", "TestValue2", false);
  std::string verificationData = StringFromContentsOfLogFile();
  ASSERT_EQ(expectedData, verificationData) << "The log file should match the data we wrote";
}

TEST_F(DasGameLogAppenderTest, WriteThreeMessages) {
  std::map<std::string, std::string> data;

  appender_->append(DASLogLevel_Info, "TestEvent1", "TestValue1", this_id_, nullptr, nullptr, 0, &globals_, data);
  appender_->append(DASLogLevel_Info, "TestEvent2", "TestValue2", this_id_, nullptr, nullptr, 0, &globals_, data);
  appender_->append(DASLogLevel_Info, "TestEvent3", "TestValue3", this_id_, nullptr, nullptr, 0, &globals_, data);
  appender_->flush();
  std::string expectedData = ExpectedLog("TestEvent1", "TestValue1", true)
    + ExpectedLog("TestEvent2", "TestValue2", false) + ExpectedLog("TestEvent3", "TestValue3", false);
  std::string verificationData = StringFromContentsOfLogFile();
  ASSERT_EQ(expectedData, verificationData) << "The log file should match the data we wrote";
}

TEST_F(DasGameLogAppenderTest, WriteMessageWithData) {
  std::map<std::string, std::string> data;

  data[Anki::Das::kPhysicalIdGlobalKey] = "0xbeef1234";
  appender_->append(DASLogLevel_Info, "TestEvent1", "TestValue1", this_id_, nullptr, nullptr, 0, &globals_, data);
  appender_->flush();
  std::string expectedData = ExpectedLog("TestEvent1", "TestValue1", "0xbeef1234", true);
  std::string verificationData = StringFromContentsOfLogFile();
  ASSERT_EQ(expectedData, verificationData);
}

TEST_F(DasGameLogAppenderTest, WriteMessageWithLotsOfGlobalsAndData) {
  std::map<std::string, std::string> data;

  data[Anki::Das::kTimeStampGlobalKey] = "123455555";
  data[Anki::Das::kMessageLevelGlobalKey] = "info";
  data[Anki::Das::kDataGlobalKey] = "some data";
  globals_[Anki::Das::kPhysicalIdGlobalKey] = "0xbeef5678";
  data[Anki::Das::kPhoneTypeGlobalKey] = "\"android phone\"";
  data[Anki::Das::kUnitIdGlobalKey] = "0xffff-1234-abcd";
  data[Anki::Das::kApplicationVersionGlobalKey] = "1.5.4,f";
  globals_[Anki::Das::kUserIdGlobalKey] = "original-value";
  data[Anki::Das::kUserIdGlobalKey] = "0xaaaa-987623-bbbb";
  data[Anki::Das::kGroupIdGlobalKey] = "qa";

  appender_->append(DASLogLevel_Info, "TestEvent1", "TestValue1", this_id_, nullptr, nullptr, 0, &globals_, data);
  appender_->flush();

  std::string expectedData = DasGameLogAppender::kCsvHeaderRow;
  expectedData += "\n123455555,";
  std::ostringstream oss;
  oss << this_id_;
  expectedData += oss.str() + ",";
  expectedData += "info,TestEvent1,TestValue1,some data,0xbeef5678,\"\"\"android phone\"\"\",0xffff-1234-abcd,\"1.5.4,f\",0xaaaa-987623-bbbb,,";
  expectedData += globals_[Anki::Das::kGameIdGlobalKey];
  expectedData += ",qa,,1\n";

  std::string verificationData = StringFromContentsOfLogFile();
  ASSERT_EQ(expectedData, verificationData);
}

TEST_F(DasGameLogAppenderTest, ClearGameGlobal) {
  std::map<std::string, std::string> data;

  appender_->append(DASLogLevel_Info, "TestEvent1", "TestValue1", this_id_, nullptr, nullptr, 0, &globals_, data);
  appender_->flush();
  std::string expectedData = ExpectedLog("TestEvent1", "TestValue1", true);
  std::string verificationData = StringFromContentsOfLogFile();
  ASSERT_EQ(expectedData, verificationData) << "The log file should match the data we wrote";

  auto it = globals_.find("$game");
  if (it != globals_.end()) {
    globals_.erase(it);
  }
  appender_->append(DASLogLevel_Info, "TestEvent2", "TestValue2", this_id_, nullptr, nullptr, 0, &globals_, data);
  appender_->flush();
  verificationData = StringFromContentsOfLogFile();
  ASSERT_EQ(expectedData, verificationData) <<
    "The log file should not have changed after we cleared the $game global";
}

} // namespace Das
} // namespace Anki
