/**
 * File: testRobotAudio.cpp
 *
 * Author: damjan stulic
 * Created: 6/8/16
 *
 * Description: Unit tests for Robot Audio
 *
 * Copyright: Anki, Inc. 2016
 *
 * --gtest_filter=RobotAudio*
 **/


#include "gtest/gtest.h"

#include "anki/cozmo/basestation/animation/animation.h"
#include "helpers/audio/robotAudioAnimationOnRobotTest.h"
#include "anki/cozmo/basestation/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/cozmo/shared/cozmoConfig_common.h"
#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "clad/types/animationKeyFrames.h"
#include "helpers/audio/robotAudioTestClient.h"
#include "helpers/audio/animationAnimationTestConfig.h"
#include "util/logging/logging.h"

#include <functional>
#include <vector>

#define TEST_ROBOT_AUDIO_DEV_LOG 1


using namespace Anki;
using namespace Anki::Cozmo;
using namespace Anki::Cozmo::Audio;

extern Anki::Cozmo::CozmoContext* cozmoContext;
const char* ROBOT_ADVERTISING_HOST_IP = "127.0.0.1";
const char* VIZ_HOST_IP = "127.0.0.1";



class RobotAudioTest : public testing::Test {
  
public:
  
  void SetUp()
  {
    animationContainer = CreateAnimationContainer();
    auto container = CreateAnimationContainer();
    animation = container->GetAnimation("ANIMATION_TEST");
    EXPECT_TRUE( animation != nullptr );
  }

  void TearDown()
  {
    animation = nullptr;
    Util::SafeDelete( animationContainer );
  }
  
  CannedAnimationContainer* CreateAnimationContainer()
  {
    const std::string animationFile = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                      "config/basestation/animations/ANIMATION_TEST.json");
    Json::Value animDefs;
    const bool success = cozmoContext->GetDataPlatform()->readAsJson(animationFile, animDefs);
    EXPECT_TRUE(success);
    
    std::string animationId;
    CannedAnimationContainer* container = new CannedAnimationContainer();
    if (success && !animDefs.empty()) {
      container->DefineFromJson(animDefs, animationId);
    }
    auto names = container->GetAnimationNames();
    for (auto name : names) {
      PRINT_NAMED_INFO("RobotAudio.CreateRobotAudio", "%s", name.c_str());
    }
    EXPECT_EQ( 1, names.size() );
    
    return container;
  }
  
  // Return True when animation is complete
  using AnimationRunLoopFunc = std::function<bool( RobotAudioAnimationOnRobot& audioAnimation, AnimationAnimationTestConfig& config, uint32_t animationTime_ms )>;
  
  void RunAnimation( RobotAudioAnimationOnRobot& audioAnimation, AnimationAnimationTestConfig& config, AnimationRunLoopFunc loopFunc )
  {
    ASSERT_TRUE( loopFunc != nullptr );
    uint32_t animationTime = 0;
    bool animationComplete = false;
    
    while ( !animationComplete ) {
      
      animationComplete = loopFunc( audioAnimation, config, animationTime );
      
      animationTime += Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS;
    }
  }


  // Vars

  AnimationAnimationTestConfig config;
  RobotAudioTestClient audioClient;

  CannedAnimationContainer* animationContainer = nullptr;
  Animation* animation = nullptr;
  
};

// Exercise animation is able to get bufferes when expected under perfect condition
const RobotAudioTest::AnimationRunLoopFunc loopFunc = []( RobotAudioAnimationOnRobot& audioAnimation, AnimationAnimationTestConfig& config, uint32_t animationTime_ms )
{
  // Perform Unit Test on Animation HERE!
  
  // Get Expected events
  const uint32_t frameStartTime_ms = animationTime_ms - Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS + 1;
  std::vector<AnimationAnimationTestConfig::TestAudioEvent> frameEvents = config.GetCurrentPlayingEvents( frameStartTime_ms, animationTime_ms );
  
  // Always call update
  RobotAudioAnimation::AnimationState state = audioAnimation.Update( 0 , animationTime_ms );
  
  EXPECT_EQ( RobotAudioAnimation::AnimationState::AudioFramesReady, state);
  
  // Pop frame
  RobotInterface::EngineToRobot* audioFrame = nullptr;
  audioAnimation.PopRobotAudioMessage( audioFrame, 0, animationTime_ms );
  
  if ( TEST_ROBOT_AUDIO_DEV_LOG && false ) {
    std::string testStr;
    for ( auto& anEvent : frameEvents ) {
      testStr += ( "\n" + std::string( EnumToString( anEvent.event ) ) + " @ " + std::to_string(anEvent.startTime_ms) );
    }
    PRINT_NAMED_INFO("RobotAudioTest.TestAnimaiion01", "animTime: %d - %s", animationTime_ms, testStr.c_str() );
    PRINT_NAMED_INFO( "RobotAudioTest.AnimationRunLoopFunc", "Popped Audio Frame: %c", audioFrame != nullptr ? 'Y' : 'N'  );
  }
  
  
  if ( frameEvents.empty() ) {
    EXPECT_EQ( nullptr, audioFrame );
  }
  else {
    EXPECT_NE( nullptr, audioFrame );
  }
  
  // Clean up
  if ( audioFrame != nullptr ) {
    Util::SafeDelete( audioFrame );
  }
  
  return animationTime_ms > config.GetAudioEvents().back().GetCompletionTime_ms();
};


// TODO: REMOVE THIS IS TEMP
TEST_F(RobotAudioTest, TestAnimation01)
{

  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Vo_Coz_Wakeup_Play, 1, 100) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Vo_Coz_Wakeup_Play, 3, 100) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Vo_Coz_Wakeup_Play, 10, 10) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Vo_Coz_Wakeup_Play, 200, 100) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Vo_Coz_Wakeup_Play, 400, 100) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Vo_Coz_Wakeup_Play, 580, 12) );
  config.InsertComplete();
  EXPECT_EQ( 6, config.GetAudioEvents().size() );
  
  config.LoadAudioKeyFrames( *animation );
  
  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudiobuffer( Anki::Cozmo::Audio::GameObjectType::CozmoAnimation )) );
  
  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
  
  RunAnimation( audioAnimation, config, loopFunc );
}

