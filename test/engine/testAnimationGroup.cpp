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

#include "coretech/common/engine/utils/timer.h"

#include "engine/moodSystem/moodManager.h"
#include "engine/animations/animationGroup/animationGroup.h"
#include "engine/animations/animationGroup/animationGroupContainer.h"

#include "util/entityComponent/dependencyManagedEntity.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/random/randomGenerator.h"

#include "json/json.h"
#include <assert.h>


using namespace Anki::Cozmo;

static const std::string kMajorWin = "majorWin";
static const std::string kMajorWinBeatBox = "majorWinBeatBox";
static const std::string kEmpty = "";


static const char* kNoAnimationJson =
"{"
"  \"Animations\": []"
"}";


static const char* kOneAnimationDefaultMoodJson =
"{"
"  \"Animations\": ["
"    {"
"      \"Name\": \"majorWin\","
"      \"Mood\": \"Default\","
"      \"Weight\": 1.0"
"    }"
"  ]"
"}";

static const char* kOneAnimationDefaultMoodCooldownJson =
"{"
"  \"Animations\": ["
"    {"
"      \"Name\": \"majorWin\","
"      \"Mood\": \"Default\","
"      \"Weight\": 1.0,"
"      \"CooldownTime_Sec\": 10.0"
"    }"
"  ]"
"}";

static const char* kTwoAnimationsDefaultMoodWithCooldownJson =
"{"
"  \"Animations\": ["
"    {"
"      \"Name\": \"majorWin\","
"      \"Mood\": \"Default\","
"      \"Weight\": 1.0,"
"      \"CooldownTime_Sec\": 10.0"
"    },"
"    {"
"      \"Name\": \"majorWinBeatBox\","
"      \"Mood\": \"Default\","
"      \"Weight\": 0.01"
"    }"
"  ]"
"}";

static const char* kOneAnimationDefaultMoodUnweightedJson =
"{"
"  \"Animations\": ["
"    {"
"      \"Name\": \"majorWin\","
"      \"Mood\": \"Default\","
"      \"Weight\": 0.0"
"    }"
"  ]"
"}";

static const char* kOneAnimationHighStimJson =
"{"
"  \"Animations\": ["
"    {"
"      \"Name\": \"majorWin\","
"      \"Mood\": \"HighStim\","
"      \"Weight\": 1.0"
"    }"
"  ]"
"}";

static const char* kTwoAnimationsDefaultMoodsJson =
"{"
"  \"Animations\": ["
"    {"
"      \"Name\": \"majorWin\","
"      \"Mood\": \"Default\","
"      \"Weight\": 1.0"
"    },"
"    {"
"      \"Name\": \"majorWinBeatBox\","
"      \"Mood\": \"Default\","
"      \"Weight\": 1.0"
"    }"
"  ]"
"}";

static const char* kTwoAnimationsHighStimDefaultMoodsJson =
"{"
"  \"Animations\": ["
"    {"
"      \"Name\": \"majorWin\","
"      \"Mood\": \"HighStim\","
"      \"Weight\": 1.0"
"    },"
"    {"
"      \"Name\": \"majorWinBeatBox\","
"      \"Mood\": \"Default\","
"      \"Weight\": 1.0"
"    }"
"  ]"
"}";

static const char* kTwoAnimationsHighFrustratedMoodsJson =
"{"
"  \"Animations\": ["
"    {"
"      \"Name\": \"majorWin\","
"      \"Mood\": \"HighStim\","
"      \"Weight\": 1.0"
"    },"
"    {"
"      \"Name\": \"majorWinBeatBox\","
"      \"Mood\": \"Frustrated\","
"      \"Weight\": 1.0"
"    }"
"  ]"
"}";

static const uint32_t kRandomSeed = 123;
static Anki::Util::RandomGenerator gRNG(kRandomSeed);

AnimationGroup DeserializeAnimationGroupFromJson(const char* jsonString) {
  Json::Value data;
  Json::Reader reader;
  bool success = reader.parse(jsonString, data);

  EXPECT_TRUE(success);
  
  AnimationGroup animGroup(gRNG);
  
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

  DeserializeAnimationGroupFromJson(kOneAnimationDefaultMoodJson);
  
  DeserializeAnimationGroupFromJson(kOneAnimationHighStimJson);

  DeserializeAnimationGroupFromJson(kTwoAnimationsDefaultMoodsJson);
  
  DeserializeAnimationGroupFromJson(kTwoAnimationsHighStimDefaultMoodsJson);
  
  DeserializeAnimationGroupFromJson(kTwoAnimationsHighFrustratedMoodsJson);
}

TEST(AnimationGroupContainer, AnimationGroupContainerDeserialization)
{
  AnimationGroupContainer container(gRNG);
  DeserializeAnimationGroupContainerFromJson(container, "a", kNoAnimationJson);
  
  EXPECT_EQ(1, container.GetAnimationGroupNames().size());
  
  DeserializeAnimationGroupContainerFromJson(container, "b", kOneAnimationDefaultMoodJson);

  EXPECT_EQ(2, container.GetAnimationGroupNames().size());

  DeserializeAnimationGroupContainerFromJson(container, "c", kOneAnimationHighStimJson);

  EXPECT_EQ(3, container.GetAnimationGroupNames().size());

  DeserializeAnimationGroupContainerFromJson(container, "d", kTwoAnimationsDefaultMoodsJson);

  EXPECT_EQ(4, container.GetAnimationGroupNames().size());

  DeserializeAnimationGroupContainerFromJson(container, "e", kTwoAnimationsHighStimDefaultMoodsJson);

  EXPECT_EQ(5, container.GetAnimationGroupNames().size());
  
  // Redeserialize the last one
  DeserializeAnimationGroupContainerFromJson(container, "e", kTwoAnimationsHighStimDefaultMoodsJson);
  
  EXPECT_EQ(5, container.GetAnimationGroupNames().size());
  
  // clear it
  container.Clear();

  EXPECT_EQ(0, container.GetAnimationGroupNames().size());
  
  // now redeserialize the last one.
  DeserializeAnimationGroupContainerFromJson(container, "e", kTwoAnimationsHighStimDefaultMoodsJson);
  
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
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  AnimationGroup group = DeserializeAnimationGroupFromJson(kOneAnimationDefaultMoodJson);
  
  const std::string& name = group.GetAnimationName(moodManager, groupContainer);
  
  EXPECT_EQ(kMajorWin, name);
}

TEST(AnimationGroup, GetDefaultAnimationName)
{
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  moodManager.SetEmotion(EmotionType::Happy, 0.5);
  
  AnimationGroup group = DeserializeAnimationGroupFromJson(kOneAnimationDefaultMoodJson);
  
  const std::string& name = group.GetAnimationName(moodManager, groupContainer);
  
  EXPECT_EQ(kMajorWin, name);
}

TEST(AnimationGroup, GetAnimationNameBeforeCooldownSingle)
{
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  moodManager.SetEmotion(EmotionType::Happy, 0.5);
  
  Anki::DependencyManagedEntity<RobotComponentID> dependencies;
  Anki::BaseStationTimer::getInstance()->UpdateTime( Anki::Util::SecToNanoSec( 0.f ) );
  moodManager.UpdateDependent(dependencies);
  
  AnimationGroup group = DeserializeAnimationGroupFromJson(kOneAnimationDefaultMoodCooldownJson);
  
  const std::string& name = group.GetAnimationName(moodManager, groupContainer);
  
  EXPECT_EQ(kMajorWin, name);
  
  Anki::BaseStationTimer::getInstance()->UpdateTime( Anki::Util::SecToNanoSec( 5.f ) );
  moodManager.UpdateDependent(dependencies);
  
  const std::string& name2 = group.GetAnimationName(moodManager, groupContainer);

  // all are on cooldown, so should still pick major win
  EXPECT_EQ(kMajorWin, name2);
}

TEST(AnimationGroup, GetAnimationNameBeforeCooldownMultiple)
{
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  moodManager.SetEmotion(EmotionType::Happy, 0.5);
  
  Anki::DependencyManagedEntity<RobotComponentID> dependencies;
  Anki::BaseStationTimer::getInstance()->UpdateTime( Anki::Util::SecToNanoSec( 0.f ) );
  moodManager.UpdateDependent(dependencies);
  
  AnimationGroup group = DeserializeAnimationGroupFromJson(kTwoAnimationsDefaultMoodWithCooldownJson);
  
  std::string name;

  // either animation could be selected, so keep going until we get major win (because that one has a cool
  // down)
  int tries = 0;
  while( name != kMajorWin ) {
    name = group.GetAnimationName(moodManager, groupContainer);
    EXPECT_TRUE( name == kMajorWin || name == kMajorWinBeatBox ) << name;
    tries++;
    if( tries > 10000 ) {
      ADD_FAILURE() << "very likely failure. Couldn't get majorWin animation";
    }
  }
  
  Anki::BaseStationTimer::getInstance()->UpdateTime( Anki::Util::SecToNanoSec( 5.f ) );
  moodManager.UpdateDependent(dependencies);
  
  const std::string& name2 = group.GetAnimationName(moodManager, groupContainer);

  // major win should be on cooldown, so should still pick major win
  EXPECT_EQ(kMajorWinBeatBox, name2);
}

TEST(AnimationGroup, GetAnimationNameOnCooldown)
{
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  moodManager.SetEmotion(EmotionType::Happy, 0.5);
  
  Anki::DependencyManagedEntity<RobotComponentID> dependencies;
  Anki::BaseStationTimer::getInstance()->UpdateTime( Anki::Util::SecToNanoSec( 0.f ) );
  moodManager.UpdateDependent(dependencies);
  
  AnimationGroup group = DeserializeAnimationGroupFromJson(kOneAnimationDefaultMoodCooldownJson);
  
  const std::string& name = group.GetAnimationName(moodManager, groupContainer);
  
  EXPECT_EQ(kMajorWin, name);
  
  Anki::BaseStationTimer::getInstance()->UpdateTime( Anki::Util::SecToNanoSec( 10.f ) );
  moodManager.UpdateDependent(dependencies);

  const std::string& name2 = group.GetAnimationName(moodManager, groupContainer);
  
  EXPECT_EQ(kMajorWin, name2);
}

TEST(AnimationGroup, GetAnimationNameAfterCooldown)
{
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  moodManager.SetEmotion(EmotionType::Happy, 0.5);
  
  Anki::DependencyManagedEntity<RobotComponentID> dependencies;
  Anki::BaseStationTimer::getInstance()->UpdateTime( Anki::Util::SecToNanoSec( 0.f ) );
  moodManager.UpdateDependent(dependencies);
  
  AnimationGroup group = DeserializeAnimationGroupFromJson(kOneAnimationDefaultMoodCooldownJson);
  
  const std::string& name = group.GetAnimationName(moodManager, groupContainer);
  
  EXPECT_EQ(kMajorWin, name);
  
  Anki::BaseStationTimer::getInstance()->UpdateTime( Anki::Util::SecToNanoSec( 15.f ) );
  moodManager.UpdateDependent(dependencies);
  
  const std::string& name2 = group.GetAnimationName(moodManager, groupContainer);
  
  EXPECT_EQ(kMajorWin, name2);
}

TEST(AnimationGroup, GetDefaultAnimationNameUnweighted)
{
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  moodManager.SetEmotion(EmotionType::Happy, 0.5);
  
  AnimationGroup group = DeserializeAnimationGroupFromJson(kOneAnimationDefaultMoodUnweightedJson);
  
  const std::string& name = group.GetAnimationName(moodManager, groupContainer);
  
  EXPECT_EQ(kMajorWin, name);
}

TEST(AnimationGroup, GetOneHighStimAnimationName)
{
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  moodManager.SetEmotion(EmotionType::Stimulated, 0.9);
  
  AnimationGroup group = DeserializeAnimationGroupFromJson(kOneAnimationHighStimJson);
  
  const std::string& name = group.GetAnimationName(moodManager, groupContainer);
  
  EXPECT_EQ(kMajorWin, name);
}

TEST(AnimationGroup, GetNoAnimationName)
{
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  AnimationGroup group = DeserializeAnimationGroupFromJson(kNoAnimationJson);
  
  const std::string& name = group.GetAnimationName(moodManager, groupContainer);
  
  EXPECT_EQ(kEmpty, name);
}

TEST(AnimationGroup, GetNoDefaultAnimationName)
{
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  AnimationGroup group = DeserializeAnimationGroupFromJson(kOneAnimationHighStimJson);
  
  const std::string& name = group.GetAnimationName(moodManager, groupContainer);
  
  EXPECT_EQ(kEmpty, name);
}

// run a maximum of 100 times. It should be a 50-50 chance of getting
// either name, so the chance of getting the same one all 100 times
// is astronomical.
void TestTwoAnimations100Times(const char* json, const MoodManager& moodManager, AnimationGroupContainer& groupContainer, bool& foundMajorWin, bool& foundMajorWinBeatBox) {
  AnimationGroup group = DeserializeAnimationGroupFromJson(json);

  for(int i = 0; i < 100; i++) {
    
    const std::string& name = group.GetAnimationName(moodManager, groupContainer);
    
    foundMajorWin |= (name == kMajorWin);
    foundMajorWinBeatBox |= (name == kMajorWinBeatBox);
  }
}

TEST(AnimationGroup, GetEitherAnimationNameOfTwo)
{
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  bool foundMajorWin = false, foundMajorWinBeatBox = false;
  TestTwoAnimations100Times(kTwoAnimationsDefaultMoodsJson, moodManager, groupContainer, foundMajorWin, foundMajorWinBeatBox);
  
  EXPECT_TRUE(foundMajorWin);
  EXPECT_TRUE(foundMajorWinBeatBox);
}


TEST(AnimationGroup, GetEitherDefaultAnimationNameOfTwo)
{
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  moodManager.SetEmotion(EmotionType::Happy, 0.5);
  
  bool foundMajorWin = false, foundMajorWinBeatBox = false;
  TestTwoAnimations100Times(kTwoAnimationsDefaultMoodsJson, moodManager, groupContainer, foundMajorWin, foundMajorWinBeatBox);
  
  EXPECT_TRUE(foundMajorWin);
  EXPECT_TRUE(foundMajorWinBeatBox);
}

TEST(AnimationGroup, GetNeitherAnimationNameOfTwo)
{
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  bool foundMajorWin = false, foundMajorWinBeatBox = false;
  TestTwoAnimations100Times(kTwoAnimationsHighFrustratedMoodsJson, moodManager, groupContainer, foundMajorWin, foundMajorWinBeatBox);
  
  EXPECT_TRUE(!foundMajorWin);
  EXPECT_TRUE(!foundMajorWinBeatBox);
}


TEST(AnimationGroup, GetFirstAnimationNameOfTwo)
{
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  moodManager.SetEmotion(EmotionType::Stimulated, 0.9);
  
  bool foundMajorWin = false, foundMajorWinBeatBox = false;
  TestTwoAnimations100Times(kTwoAnimationsHighStimDefaultMoodsJson, moodManager, groupContainer, foundMajorWin, foundMajorWinBeatBox);
  
  EXPECT_TRUE(foundMajorWin);
  EXPECT_TRUE(!foundMajorWinBeatBox);
}

TEST(AnimationGroup, GetSecondAnimationNameOfTwo)
{
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  bool foundMajorWin = false, foundMajorWinBeatBox = false;
  TestTwoAnimations100Times(kTwoAnimationsHighStimDefaultMoodsJson, moodManager, groupContainer, foundMajorWin, foundMajorWinBeatBox);
  
  EXPECT_TRUE(!foundMajorWin);
  EXPECT_TRUE(foundMajorWinBeatBox);
}

TEST(AnimationGroup, GetDefaultAnimationNameOfTwo)
{
  AnimationGroupContainer groupContainer(gRNG);
  MoodManager moodManager;
  
  moodManager.SetEmotion(EmotionType::Happy, -0.5);
  
  bool foundMajorWin = false, foundMajorWinBeatBox = false;
  TestTwoAnimations100Times(kTwoAnimationsHighStimDefaultMoodsJson, moodManager, groupContainer, foundMajorWin, foundMajorWinBeatBox);
  
  EXPECT_TRUE(!foundMajorWin);
  EXPECT_TRUE(foundMajorWinBeatBox);
}
