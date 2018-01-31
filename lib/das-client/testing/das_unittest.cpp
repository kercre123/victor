#include "DAS.h"
#include "DASPrivate.h"
#include "dasGameLogAppender.h"
#include "dasGlobals.h"

#pragma GCC diagnostic push

#if __has_warning("-Wundef")
#pragma GCC diagnostic ignored "-Wundef"
#endif

#include "gtest/gtest.h"

#pragma GCC diagnostic pop


TEST(DAS, GetLevel_Unset) {
  _DAS_ClearSetLevels();
  EXPECT_EQ(DASLogLevel_NumLevels, _DAS_GetLevel("UnSetEvent", DASLogLevel_NumLevels));
}

TEST(DAS, SetLevel_Simple) {
  _DAS_ClearSetLevels();
  _DAS_SetLevel("TestEvent", DASLogLevel_Info);
  EXPECT_EQ(DASLogLevel_Info, _DAS_GetLevel("TestEvent", DASLogLevel_NumLevels));
}

TEST(DAS, SetLevel_Update) {
  _DAS_ClearSetLevels();
  _DAS_SetLevel("TestEvent", DASLogLevel_Info);
  EXPECT_EQ(DASLogLevel_Info, _DAS_GetLevel("TestEvent", DASLogLevel_NumLevels));
  _DAS_SetLevel("TestEvent", DASLogLevel_Warn);
  EXPECT_EQ(DASLogLevel_Warn, _DAS_GetLevel("TestEvent", DASLogLevel_NumLevels));
}

TEST(DAS, SetLevel_Override) {
  _DAS_ClearSetLevels();
  _DAS_SetLevel("TestParent", DASLogLevel_Event);
  _DAS_SetLevel("TestParent.TestChildOne", DASLogLevel_Info);
  _DAS_SetLevel("TestParent.TestChildTwo", DASLogLevel_Debug);

  EXPECT_EQ(DASLogLevel_Event, _DAS_GetLevel("TestParent", DASLogLevel_NumLevels));
  EXPECT_EQ(DASLogLevel_Event, _DAS_GetLevel("TestParent.TestChildThree", DASLogLevel_NumLevels));
  EXPECT_EQ(DASLogLevel_Info, _DAS_GetLevel("TestParent.TestChildOne", DASLogLevel_NumLevels));
  EXPECT_EQ(DASLogLevel_Info, _DAS_GetLevel("TestParent.TestChildOne.Foo", DASLogLevel_NumLevels));
  EXPECT_EQ(DASLogLevel_Debug, _DAS_GetLevel("TestParent.TestChildTwo", DASLogLevel_NumLevels));
  EXPECT_EQ(DASLogLevel_Debug, _DAS_GetLevel("TestParent.TestChildTwo.Bar", DASLogLevel_NumLevels));
}

TEST(DAS, ClearSetLevels) {
  _DAS_SetLevel("AnotherTestEvent", DASLogLevel_Warn);
  _DAS_ClearSetLevels();
  EXPECT_EQ(DASLogLevel_NumLevels, _DAS_GetLevel("AnotherTestEvent", DASLogLevel_NumLevels));
}

TEST(DAS, SetGlobal) {
  _DAS_ClearGlobals();

  std::string expected = "0xbeef1234";
  DAS_SetGlobal("$phys", expected.c_str());
  std::string actual;
  _DAS_GetGlobal("$phys", actual);
  EXPECT_EQ(expected, actual);

  expected = "";
  DAS_SetGlobal("$data", nullptr);
  _DAS_GetGlobal("$data", actual);
  EXPECT_EQ(expected, actual);

  DAS_SetGlobal("$data", "");
  _DAS_GetGlobal("$data", actual);
  EXPECT_EQ(expected, actual);
}

TEST(DAS, SetGlobalGameId) {
  _DAS_ClearGlobals();

  Anki::Das::DasGameLogAppender* appender = _DAS_GetGameLogAppender();
  EXPECT_EQ(nullptr, appender);

  std::string gameId = "ABC-123-DEF-456";
  DAS_SetGlobal(Anki::Das::kGameIdGlobalKey, gameId.c_str());

  appender = _DAS_GetGameLogAppender();
  EXPECT_TRUE(nullptr != appender);

  DAS_SetGlobal(Anki::Das::kGameIdGlobalKey, nullptr);
  appender = _DAS_GetGameLogAppender();
  EXPECT_EQ(nullptr, appender);

  _DAS_ClearGlobals();
}

TEST(DAS, TearDown) {
  DASClose();
}
