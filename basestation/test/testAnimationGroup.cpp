/**
 * File: testAnimationGroup
 *
 * Author: Trevor Dasch
 * Created: 01/13/2016
 *
 * Description: Unit tests Animation Group
 *
 * Copyright: Anki, Inc. 2015
 *
 * --gtest_filter=AnimationGroup*
 **/

#include "gtest/gtest.h"

#include "util/math/math.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/animationGroup/animationGroup.h"
#include "anki/cozmo/basestation/animationGroup/animationGroupContainer.h"
#include "util/logging/logging.h"
#include "json/json.h"
#include <assert.h>


using namespace Anki::Cozmo;

static const std::string kMajorWin = "majorWin";
static const std::string kMajorWinBeatBox = "majorWinBeatBox";


static const char* kNoAnimationJson =
"{"
"  \"Animations\": []"
"}";


static const char* kOneAnimationNoScorersJson =
"{"
"  \"Animations\": ["
"    {"
"      \"Name\": \"majorWin\","
"      \"MoodScorer\": []"
"    }"
"  ]"
"}";

static const char* kOneAnimationOneScorerJson =
"{"
"  \"Animations\": ["
"    {"
"      \"Name\": \"majorWin\","
"      \"MoodScorer\": ["
"        {"
"          \"emotionType\": \"Happy\","
"          \"trackDelta\": false,"
"          \"scoreGraph\": {"
"            \"nodes\": ["
"              {"
"                \"x\": -1.0,"
"                \"y\": 0.0"
"              },"
"              {"
"                \"x\": 1.0,"
"                \"y\": 1.0"
"              }"
"            ]"
"          }"
"        }"
"      ]"
"    }"
"  ]"
"}";

static const char* kTwoAnimationsNoScorersJson =
"{"
"  \"Animations\": ["
"    {"
"      \"Name\": \"majorWin\","
"      \"MoodScorer\": []"
"    },"
"    {"
"      \"Name\": \"majorWinBeatBox\","
"      \"MoodScorer\": []"
"    }"
"  ]"
"}";

static const char* kTwoAnimationsTwoScorersJson =
"{"
"  \"Animations\": ["
"    {"
"      \"Name\": \"majorWin\","
"      \"MoodScorer\": ["
"        {"
"          \"emotionType\": \"Happy\","
"          \"trackDelta\": false,"
"          \"scoreGraph\": {"
"            \"nodes\": ["
"              {"
"                \"x\": -1.0,"
"                \"y\": 0.0"
"              },"
"              {"
"                \"x\": 1.0,"
"                \"y\": 1.0"
"              }"
"            ]"
"          }"
"        },"
"        {"
"          \"emotionType\": \"Calm\","
"          \"trackDelta\": false,"
"          \"scoreGraph\": {"
"            \"nodes\": ["
"              {"
"                \"x\": -1.0,"
"                \"y\": 0.0"
"              },"
"              {"
"                \"x\": 1.0,"
"                \"y\": 1.0"
"              }"
"            ]"
"          }"
"        }"
"      ]"
"    },"
"    {"
"      \"Name\": \"majorWinBeatBox\","
"      \"MoodScorer\": ["
"        {"
"          \"emotionType\": \"Happy\","
"          \"trackDelta\": false,"
"          \"scoreGraph\": {"
"            \"nodes\": ["
"              {"
"                \"x\": -1.0,"
"                \"y\": 1.0"
"              },"
"              {"
"                \"x\": 1.0,"
"                \"y\": 0.0"
"              }"
"            ]"
"          }"
"        },"
"        {"
"          \"emotionType\": \"Calm\","
"          \"trackDelta\": false,"
"          \"scoreGraph\": {"
"            \"nodes\": ["
"              {"
"                \"x\": -1.0,"
"                \"y\": 1.0"
"              },"
"              {"
"                \"x\": 1.0,"
"                \"y\": 0.0"
"              }"
"            ]"
"          }"
"        }"
"      ]"
"    }"
"  ]"
"}";


AnimationGroup DeserializeAnimationGroupFromJson(const char* jsonString) {
  Json::Value data;
  Json::Reader reader;
  bool success = reader.parse(jsonString, data);

  EXPECT_TRUE(success);
  
  AnimationGroup animGroup;
  
  auto result = animGroup.DefineFromJson("Win", data);
  
  EXPECT_EQ(Anki::RESULT_OK, result);
  
  return animGroup;
}

void DeserializeAnimationGroupContainerFromJson(AnimationGroupContainer & container, const std::string& name, const char* jsonString) {
  Json::Value data;
  Json::Reader reader;
  bool success = reader.parse(jsonString, data);
  
  EXPECT_TRUE(success);
  
  auto result = container.DefineFromJson(data, name);
  
  EXPECT_EQ(Anki::RESULT_OK, result);
}


// Try deserializing all our hardcoded Json
TEST(AnimationGroup, DeserializeAnimationGroup)
{
  DeserializeAnimationGroupFromJson(kNoAnimationJson);

  DeserializeAnimationGroupFromJson(kOneAnimationNoScorersJson);
  
  DeserializeAnimationGroupFromJson(kOneAnimationOneScorerJson);

  DeserializeAnimationGroupFromJson(kTwoAnimationsNoScorersJson);
  
  DeserializeAnimationGroupFromJson(kTwoAnimationsTwoScorersJson);
}

TEST(AnimationGroupContainer, AnimationGroupContainerDeserialization)
{
  AnimationGroupContainer container;
  DeserializeAnimationGroupContainerFromJson(container, "a", kNoAnimationJson);
  
  EXPECT_EQ(1, container.GetAnimationGroupNames().size());
  
  DeserializeAnimationGroupContainerFromJson(container, "b", kOneAnimationNoScorersJson);

  EXPECT_EQ(2, container.GetAnimationGroupNames().size());

  DeserializeAnimationGroupContainerFromJson(container, "c", kOneAnimationOneScorerJson);

  EXPECT_EQ(3, container.GetAnimationGroupNames().size());

  DeserializeAnimationGroupContainerFromJson(container, "d", kTwoAnimationsNoScorersJson);

  EXPECT_EQ(4, container.GetAnimationGroupNames().size());

  DeserializeAnimationGroupContainerFromJson(container, "e", kTwoAnimationsTwoScorersJson);

  EXPECT_EQ(5, container.GetAnimationGroupNames().size());
  
  // Redeserialize the last one
  DeserializeAnimationGroupContainerFromJson(container, "e", kTwoAnimationsTwoScorersJson);
  
  EXPECT_EQ(5, container.GetAnimationGroupNames().size());
  
  // clear it
  container.Clear();

  EXPECT_EQ(0, container.GetAnimationGroupNames().size());
  
  // now redeserialize the last one.
  DeserializeAnimationGroupContainerFromJson(container, "e", kTwoAnimationsTwoScorersJson);
  
  EXPECT_EQ(1, container.GetAnimationGroupNames().size());

  // now test we can retrieve a group without getting null
  auto group = container.GetAnimationGroup("e");
  
  EXPECT_FALSE(group == nullptr);
  
  // now test we can't retrieve a group that doesn't exist
  group = container.GetAnimationGroup("a");

  EXPECT_TRUE(group == nullptr);
}


TEST(AnimationGroup, GetOneAnimationName)
{
  MoodManager moodManager;
  
  AnimationGroup group = DeserializeAnimationGroupFromJson(kOneAnimationNoScorersJson);
  
  auto name = group.GetAnimationName(moodManager);
  
  EXPECT_EQ(kMajorWin, name);
}

TEST(AnimationGroup, GetNoAnimationName)
{
  MoodManager moodManager;
  
  AnimationGroup group = DeserializeAnimationGroupFromJson(kNoAnimationJson);
  
  auto name = group.GetAnimationName(moodManager);
  
  const std::string empty = "";
  
  EXPECT_EQ(empty, name);
}

// run a maximum of 100 times. It should be a 50-50 chance of getting
// either name, so the chance of getting the same one all 100 times
// is astronomical.
void TestTwoAnimations100Times(const MoodManager& moodManager, bool& foundMajorWin, bool& foundMajorWinBeatBox) {
  AnimationGroup group = DeserializeAnimationGroupFromJson(kTwoAnimationsTwoScorersJson);

  for(int i = 0; i < 100; i++) {
    
    auto name = group.GetAnimationName(moodManager);
    
    foundMajorWin |= (name == kMajorWin);
    foundMajorWinBeatBox |= (name == kMajorWinBeatBox);
  }
}

TEST(AnimationGroup, GetOneOfTwoAnimationNames)
{
  MoodManager moodManager;
  
  bool foundMajorWin = false, foundMajorWinBeatBox = false;
  TestTwoAnimations100Times(moodManager, foundMajorWin, foundMajorWinBeatBox);
  
  EXPECT_TRUE(foundMajorWin);
  EXPECT_TRUE(foundMajorWinBeatBox);
}

TEST(AnimationGroup, GetFirstAnimationNameWithOneEmotions)
{
  MoodManager moodManager;
  
  moodManager.SetEmotion(EmotionType::Happy, 0.5);
  
  bool foundMajorWin = false, foundMajorWinBeatBox = false;
  TestTwoAnimations100Times(moodManager, foundMajorWin, foundMajorWinBeatBox);
  
  EXPECT_TRUE(foundMajorWin);
  EXPECT_TRUE(!foundMajorWinBeatBox);
}

TEST(AnimationGroup, GetSecondAnimationNameWithOneEmotions)
{
  MoodManager moodManager;
  
  moodManager.SetEmotion(EmotionType::Happy, -0.5);
  
  bool foundMajorWin = false, foundMajorWinBeatBox = false;
  TestTwoAnimations100Times(moodManager, foundMajorWin, foundMajorWinBeatBox);
  
  EXPECT_TRUE(!foundMajorWin);
  EXPECT_TRUE(foundMajorWinBeatBox);
}

TEST(AnimationGroup, GetFirstAnimationNameWithTwoEmotions)
{
  MoodManager moodManager;
  
  moodManager.SetEmotion(EmotionType::Happy, 0.5);
  moodManager.SetEmotion(EmotionType::Calm, 0.5);
  
  bool foundMajorWin = false, foundMajorWinBeatBox = false;
  TestTwoAnimations100Times(moodManager, foundMajorWin, foundMajorWinBeatBox);
  
  EXPECT_TRUE(foundMajorWin);
  EXPECT_TRUE(!foundMajorWinBeatBox);
}

TEST(AnimationGroup, GetSecondAnimationNameWithTwoEmotions)
{
  MoodManager moodManager;
  
  moodManager.SetEmotion(EmotionType::Happy, -0.5);
  moodManager.SetEmotion(EmotionType::Calm, -0.5);
  
  bool foundMajorWin = false, foundMajorWinBeatBox = false;
  TestTwoAnimations100Times(moodManager, foundMajorWin, foundMajorWinBeatBox);
  
  EXPECT_TRUE(!foundMajorWin);
  EXPECT_TRUE(foundMajorWinBeatBox);
}


TEST(AnimationGroup, GetOneOfTwoAnimationNamesWithTwoEmotions)
{
  MoodManager moodManager;
  
  moodManager.SetEmotion(EmotionType::Happy, 0.5);
  moodManager.SetEmotion(EmotionType::Calm, -0.5);
  
  bool foundMajorWin = false, foundMajorWinBeatBox = false;
  TestTwoAnimations100Times(moodManager, foundMajorWin, foundMajorWinBeatBox);
  
  EXPECT_TRUE(foundMajorWin);
  EXPECT_TRUE(foundMajorWinBeatBox);
}
