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

#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/cozmo/basestation/animation/animation.h"
#include "anki/cozmo/basestation/animationContainers/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/animationKeyFrames.h"
#include "helpers/audio/animationAnimationTestConfig.h"
#include "helpers/audio/robotAudioAnimationOnRobotTest.h"
#include "helpers/audio/robotAudioTestClient.h"
#include "util/logging/logging.h"

#include <functional>
#include <vector>

#define TEST_ROBOT_AUDIO_DEV_LOG 0


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
  using AnimationRunLoopFunc = std::function<bool( RobotAudioAnimationOnRobot& audioAnimation, AnimationAnimationTestConfig& config, uint32_t animationTime_ms, bool& isStream, uint32_t& frameDrift_ms )>;
  
  void RunAnimation( RobotAudioAnimationOnRobot& audioAnimation, AnimationAnimationTestConfig& config, AnimationRunLoopFunc loopFunc )
  {
    ASSERT_TRUE( loopFunc != nullptr );
    uint32_t animationTime = 0;
    bool animationComplete = false;
    
    bool isStream = false;
    uint32_t frameDrift_ms = 0;
    
    while ( !animationComplete ) {
      
      animationComplete = loopFunc( audioAnimation, config, animationTime, isStream, frameDrift_ms );
      
      animationTime += Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS;
    }
  }


  // Vars

  AnimationAnimationTestConfig config;
  RobotAudioTestClient audioClient;

  CannedAnimationContainer* animationContainer = nullptr;
  Animation* animation = nullptr;
  
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Animation Loop methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// No audio events in animation
const RobotAudioTest::AnimationRunLoopFunc noAudioEventLoopFunc = []( RobotAudioAnimationOnRobot& audioAnimation, AnimationAnimationTestConfig& config, uint32_t animationTime_ms, bool& isStream, uint32_t& frameDrift_ms )
{
  EXPECT_TRUE( config.GetAudioEvents().empty() );
  EXPECT_EQ( RobotAudioAnimation::AnimationState::AnimationCompleted, audioAnimation.GetAnimationState() );
  
  return true;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Exercise animation is able to get bufferes when expected under perfect condition
const RobotAudioTest::AnimationRunLoopFunc generalUsesLoopFunc = []( RobotAudioAnimationOnRobot& audioAnimation, AnimationAnimationTestConfig& config, uint32_t animationTime_ms, bool& out_isStream, uint32_t& out_frameDrift_ms )
{
  // Perform Unit Test on Animation HERE!
  // Get Expected events
  
  
  // FIXME: After updating the animation "engine" we need fix our frame start & end indexes
//  const int32_t frameStartTime_ms = animationTime_ms;
//  const int32_t frameEndTime_ms = animationTime_ms + Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS - 1;

  const int32_t frameStartTime_ms = animationTime_ms - Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS;
  const int32_t frameEndTime_ms = animationTime_ms;
  
  std::vector<AnimationAnimationTestConfig::TestAudioEvent> frameEvents = config.GetCurrentPlayingEvents( frameStartTime_ms, frameEndTime_ms, out_frameDrift_ms );
  if ( TEST_ROBOT_AUDIO_DEV_LOG ) {
    printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n" );
    std::string testStr;
    for ( auto& anEvent : frameEvents ) {
      testStr += ( "\n" + std::string( EnumToString( anEvent.event ) ) + " @ " + std::to_string(anEvent.startTime_ms) +
                  " dur_ms: " + std::to_string(anEvent.duration_ms));
    }
    PRINT_NAMED_INFO( "RobotAudioTest.AnimationRunLoopFunc.generalUsesLoopFunc",
                      "animTimeRange_ms: %d - %d | %s",
                      frameStartTime_ms, frameEndTime_ms, testStr.c_str() );
  }
  
  // Check if expecting stream to start
  if (!out_isStream && !frameEvents.empty()) {
    out_isStream = true;
    // Determin frame drift
    out_frameDrift_ms = frameEvents.front().startTime_ms - frameStartTime_ms;
    PRINT_NAMED_INFO( "RobotAudioTest.AnimationRunLoopFunc.generalUsesLoopFunc",
                      "Begining of Stream - FrameDrift_ms: %d", out_frameDrift_ms );
    
    DEV_ASSERT(out_frameDrift_ms >= 0, "generalUsesLoopFunc.out_frameDrift_ms < 0");
  }
  
  
  // Always call update
  RobotAudioAnimation::AnimationState state = audioAnimation.Update( 0 , frameEndTime_ms );
  
  // We are pre-populating audio data for test, not getting data in realtime from Wwise
  EXPECT_EQ( RobotAudioAnimation::AnimationState::AudioFramesReady, state);
  
  // Pop frame
  RobotInterface::EngineToRobot* audioFrame = nullptr;
  RobotAudioAnimationOnRobotTest &audioAnimationTestRef = static_cast<RobotAudioAnimationOnRobotTest&>(audioAnimation);
  audioAnimation.PopRobotAudioMessage( audioFrame, 0, frameEndTime_ms );
  if ( TEST_ROBOT_AUDIO_DEV_LOG ) {
    size_t remainingFrames = audioAnimationTestRef.GetCurrentStreamFrameCount();
    PRINT_NAMED_INFO( "RobotAudioTest.AnimationRunLoopFunc.generalUsesLoopFunc",
                      "FrameEventCount: %zu | Popped Audio Frame: %c | Remaining frames %zu",
                      frameEvents.size(),
                      audioFrame != nullptr ? 'Y' : 'N',
                      remainingFrames );
  }
  
  
  if ( frameEvents.empty() ) {
    EXPECT_EQ( nullptr, audioFrame );
  }
  else {
    EXPECT_NE( nullptr, audioFrame );
  }
  
  // Check if stream is completed
  if ( out_isStream && 0 == audioAnimationTestRef.GetCurrentStreamFrameCount() ) {
    // Clear stream trackers
    out_isStream = false;
    out_frameDrift_ms = 0;
    if ( TEST_ROBOT_AUDIO_DEV_LOG ) {
      PRINT_NAMED_INFO( "RobotAudioTest.AnimationRunLoopFunc.generalUsesLoopFunc",
                        "Stream Completed" );
    }
  }
  
  // Clean up
  if ( audioFrame != nullptr ) {
    Util::SafeDelete( audioFrame );
  }
  
  return frameEndTime_ms > config.GetAudioEvents().back().GetCompletionTime_ms();
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TESTS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test No Events
TEST_F(RobotAudioTest, TestNoEvents)
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.InsertComplete();
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  
  config.LoadAudioKeyFrames( *animation );
  
  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudiobuffer( Anki::Cozmo::Audio::GameObjectType::CozmoBus_1 )) );
  
  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
  
  RunAnimation( audioAnimation, config, noAudioEventLoopFunc );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Single Streams
TEST_F(RobotAudioTest, TestSingleEventAnimation_FirstFrame )
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  // Currently, RobotAudioTest doesn't let me test an event starting @ 0 ms, ironic.
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 1, 100) );
  config.InsertComplete();
  EXPECT_EQ( 1, config.GetAudioEvents().size() );
  
  config.LoadAudioKeyFrames( *animation );
  
  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudiobuffer( Anki::Cozmo::Audio::GameObjectType::CozmoBus_1 )) );
  
  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
  
  RunAnimation( audioAnimation, config, generalUsesLoopFunc );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Single Streams that is not at in the first frame
TEST_F(RobotAudioTest, TestSingleEventAnimation )
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 200, 100) );
  config.InsertComplete();
  EXPECT_EQ( 1, config.GetAudioEvents().size() );
  
  config.LoadAudioKeyFrames( *animation );
  
  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudiobuffer( Anki::Cozmo::Audio::GameObjectType::CozmoBus_1 )) );
  
  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
  
  RunAnimation( audioAnimation, config, generalUsesLoopFunc );
}

// Test single stream multiple events
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Events end in order they were trigured
TEST_F(RobotAudioTest, TestSingleStream_MultipleEvents_EndInOrder)
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 1, 20) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 10, 40) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 30, 100) );
  config.InsertComplete();
  EXPECT_EQ( 3, config.GetAudioEvents().size() );
  
  config.LoadAudioKeyFrames( *animation );
  
  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudiobuffer( Anki::Cozmo::Audio::GameObjectType::CozmoBus_1 )) );
  
  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
  
  RunAnimation( audioAnimation, config, generalUsesLoopFunc );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Events that end out of order of when they were triggered
TEST_F(RobotAudioTest, TestSingleStream_MultipleEvents_EndInReverseOrder)
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 1, 100) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 10, 80) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 30, 40) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 40, 10) );
  config.InsertComplete();
  EXPECT_EQ( 4, config.GetAudioEvents().size() );
  
  config.LoadAudioKeyFrames( *animation );
  
  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudiobuffer( Anki::Cozmo::Audio::GameObjectType::CozmoBus_1 )) );
  
  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
  
  RunAnimation( audioAnimation, config, generalUsesLoopFunc );
}

// Test Multiple Streams
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Multiple Streams with a Single Event
TEST_F(RobotAudioTest, TestMultipleStreams_SingleEvent)
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 1, 100) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 200, 100) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 400, 100) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 580, 12) );
  config.InsertComplete();
  EXPECT_EQ( 4, config.GetAudioEvents().size() );
  
  config.LoadAudioKeyFrames( *animation );
  
  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudiobuffer( Anki::Cozmo::Audio::GameObjectType::CozmoBus_1 )) );
  
  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
  
  RunAnimation( audioAnimation, config, generalUsesLoopFunc );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Multiple Streams with multiple Events
TEST_F(RobotAudioTest, TestMultipleStreams_MultipleEvents)
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 1, 100) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 4, 100) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 20, 100) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 200, 100) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 400, 100) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 580, 12) );
  config.InsertComplete();
  EXPECT_EQ( 6, config.GetAudioEvents().size() );
  
  config.LoadAudioKeyFrames( *animation );
  
  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudiobuffer( Anki::Cozmo::Audio::GameObjectType::CozmoBus_1 )) );
  
  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
  
  RunAnimation( audioAnimation, config, generalUsesLoopFunc );
}

// Test Back to Back Streams
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Back to Back Streams in same frame
TEST_F(RobotAudioTest, TestMultipleStreams_BackToBack_SameFrame)
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 1, 100) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 102, 100) );
  config.InsertComplete();
  EXPECT_EQ( 2, config.GetAudioEvents().size() );
  
  config.LoadAudioKeyFrames( *animation );
  
  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudiobuffer( Anki::Cozmo::Audio::GameObjectType::CozmoBus_1 )) );
  
  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
  
  RunAnimation( audioAnimation, config, generalUsesLoopFunc );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Back to Back Streams Frame Boarder
TEST_F(RobotAudioTest, TestMultipleStreams_BackToBack_NextFrame)
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 1, 66) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 67, 100) );
  config.InsertComplete();
  EXPECT_EQ( 2, config.GetAudioEvents().size() );
  
  config.LoadAudioKeyFrames( *animation );
  
  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudiobuffer( Anki::Cozmo::Audio::GameObjectType::CozmoBus_1 )) );
  
  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
  
  RunAnimation( audioAnimation, config, generalUsesLoopFunc );
}

 
// Test Events on frame edges
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Events on frame edges
// FIXME: There is a bug with the with how audio test data is created & Popped in Test
TEST_F(RobotAudioTest, DISABLED_TestFrameEdgeCase_StartsOnEdge)
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 33, 10) );
  config.Insert( AnimationAnimationTestConfig::TestAudioEvent(Audio::GameEvent::GenericEvent::Play__Sfx__Placeholder, 133, 100) );
  config.InsertComplete();
  EXPECT_EQ( 2, config.GetAudioEvents().size() );
  
  config.LoadAudioKeyFrames( *animation );
  
  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudiobuffer( Anki::Cozmo::Audio::GameObjectType::CozmoBus_1 )) );
  
  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
  
  RunAnimation( audioAnimation, config, generalUsesLoopFunc );
}

// TODO: Add test for more edge cases!!
