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
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"
#include "clad/types/animationKeyFrames.h"
#include "helpers/audio/animationAudioTestConfig.h"
#include "helpers/audio/robotAudioAnimationOnRobotTest.h"
#include "helpers/audio/robotAudioTestClient.h"
#include "helpers/audio/streamingAnimationTest.h"
#include "util/logging/logging.h"

#include <functional>
#include <vector>

#define TEST_ROBOT_AUDIO_DEV_LOG 0


using namespace Anki;
using namespace Anki::AudioMetaData;
using namespace Anki::Cozmo;
using namespace Anki::Cozmo::Audio;
using namespace Anki::Cozmo::RobotAnimation;

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
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// RobotAudioAnimationOnRobot
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Return True when animation is complete
  using RunLoopAudioAnimationFunc = std::function<bool( RobotAudioAnimationOnRobot& audioAnimation, AnimationAudioTestConfig& config, uint32_t animationTime_ms, bool& out_isStream, uint32_t& out_frameDrift_ms )>;
  
  void RunAnimation_AudioAnimation( RobotAudioAnimationOnRobot& audioAnimation, AnimationAudioTestConfig& config, RunLoopAudioAnimationFunc loopFunc )
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
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// StreamingAnimation
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  using AnimationRunLoopStreamingAnimationFunc = std::function<bool( StreamingAnimation& animation, AnimationAudioTestConfig& config, uint32_t animationTime_ms, bool& out_isStream, uint32_t& out_frameDrift_ms )>;
  
  void RunAnimation_StreamingAnim(StreamingAnimation& anim, AnimationAudioTestConfig& config, AnimationRunLoopStreamingAnimationFunc loopFunc)
  {
    ASSERT_TRUE(nullptr != loopFunc);
    uint32_t animationTime = 0;
    bool animationComplete = false;
    
    bool isStream = false;
    uint32_t frameDrift_ms = 0;
    
    while (!animationComplete) {
      animationComplete = loopFunc(anim, config, animationTime, isStream, frameDrift_ms);
      animationTime += Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS;
    }
  }

  // Vars

  AnimationAudioTestConfig config;
  RobotAudioTestClient audioClient;

  CannedAnimationContainer* animationContainer = nullptr;
  Animation* animation = nullptr;
  
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Animation Loop methods
// RobotAudioAnimationOnRobot
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// No audio events in animation
const RobotAudioTest::RunLoopAudioAnimationFunc noAudioEventLoopFunc = []( RobotAudioAnimationOnRobot& audioAnimation, AnimationAudioTestConfig& config, uint32_t animationTime_ms, bool& isStream, uint32_t& frameDrift_ms )
{
  
  // FIXME: After updating the animation "engine" we need fix our frame start & end indexes
  //  const int32_t frameStartTime_ms = animationTime_ms;
  //  const int32_t frameEndTime_ms = animationTime_ms + Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS - 1;
  
  EXPECT_TRUE( config.GetAudioEvents().empty() );
  EXPECT_EQ( RobotAudioAnimation::AnimationState::AnimationCompleted, audioAnimation.GetAnimationState() );
  
  return true;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Exercise animation is able to get buffers when expected under perfect conditions
const RobotAudioTest::RunLoopAudioAnimationFunc generalUsesLoopFunc = []( RobotAudioAnimationOnRobot& audioAnimation, AnimationAudioTestConfig& config, uint32_t animationTime_ms, bool& out_isStream, uint32_t& out_frameDrift_ms )
{
  // Perform Unit Test on Animation HERE!
  // Get Expected events
  
  // FIXME: After updating the animation "engine" we need to fix our frame start & end indexes
  //  const int32_t frameStartTime_ms = animationTime_ms;
  //  const int32_t frameEndTime_ms = animationTime_ms + Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS - 1;

  const int32_t frameStartTime_ms = animationTime_ms - Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS;
  const int32_t frameEndTime_ms = animationTime_ms;
  
  std::vector<AnimationAudioTestConfig::TestAudioEvent> frameEvents = config.GetCurrentPlayingEvents( frameStartTime_ms, frameEndTime_ms, out_frameDrift_ms );
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
    // Determine frame drift
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
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// StreamingAnimation
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// No audio events in animation
const RobotAudioTest::AnimationRunLoopStreamingAnimationFunc noAudioEventLoopStreamingAnimFunc = [](StreamingAnimation& animation, AnimationAudioTestConfig& config, uint32_t animationTime_ms, bool& out_isStream, uint32_t& out_frameDrift_ms)
{
  EXPECT_TRUE(config.GetAudioEvents().empty());
  
  animation.Update();
  StreamingAnimationFrame aFrame;
  animation.TickPlayhead(aFrame);
  EXPECT_EQ(nullptr, aFrame.audioFrameData);
  
  return animation.IsPlaybackComplete();
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Exercise animation is able to get buffers when expected under perfect conditions
const RobotAudioTest::AnimationRunLoopStreamingAnimationFunc generalUsesLoopStreamingAnimFunc = [](StreamingAnimation& animation, AnimationAudioTestConfig& config, uint32_t animationTime_ms, bool& out_isStream, uint32_t& out_frameDrift_ms)
{
  // Perform Unit Test on Animation HERE!
  // Get Expected events
  
  const int32_t frameStartTime_ms = animationTime_ms;
  const int32_t frameEndTime_ms = animationTime_ms + Anki::Cozmo::IKeyFrame::SAMPLE_LENGTH_MS - 1;
  
  std::vector<AnimationAudioTestConfig::TestAudioEvent> frameEvents = config.GetCurrentPlayingEvents( frameStartTime_ms, frameEndTime_ms, out_frameDrift_ms );
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
    // Determine frame drift
    out_frameDrift_ms = frameEvents.front().startTime_ms - frameStartTime_ms;
    PRINT_NAMED_INFO( "RobotAudioTest.AnimationRunLoopFunc.generalUsesLoopFunc",
                     "Beginning of Stream - FrameDrift_ms: %d", out_frameDrift_ms );
    
    DEV_ASSERT(out_frameDrift_ms >= 0, "generalUsesLoopFunc.out_frameDrift_ms < 0");
  }
  
  
  // Always call update
  animation.Update();
  
  if (animation.IsPlaybackComplete()) {
    return true;
  }
  
//  RobotAudioAnimation::AnimationState state = audioAnimation.Update( 0 , frameEndTime_ms );
  
  // We are pre-populating audio data for test, not getting data in realtime from Wwise
  EXPECT_EQ(true, animation.CanPlayNextFrame());
  
  // Pop frame
  StreamingAnimationFrame aFrame;
  animation.TickPlayhead(aFrame);
  
//  RobotInterface::EngineToRobot* audioFrame = nullptr;
//  RobotAudioAnimationOnRobotTest &audioAnimationTestRef = static_cast<RobotAudioAnimationOnRobotTest&>(audioAnimation);
//  audioAnimation.PopRobotAudioMessage( audioFrame, 0, frameEndTime_ms );
//  if ( TEST_ROBOT_AUDIO_DEV_LOG ) {
//    size_t remainingFrames = audioAnimationTestRef.GetCurrentStreamFrameCount();
//    PRINT_NAMED_INFO( "RobotAudioTest.AnimationRunLoopFunc.generalUsesLoopFunc",
//                     "FrameEventCount: %zu | Popped Audio Frame: %c | Remaining frames %zu",
//                     frameEvents.size(),
//                     audioFrame != nullptr ? 'Y' : 'N',
//                     remainingFrames );
//  }
  
  
  if ( frameEvents.empty() ) {
    EXPECT_EQ( nullptr, aFrame.audioFrameData );
  }
  else {
    EXPECT_NE( nullptr, aFrame.audioFrameData );
  }
  
  // Check if stream is completed
  if ( out_isStream && nullptr == aFrame.audioFrameData ) {
    // Clear stream trackers
    out_isStream = false;
    out_frameDrift_ms = 0;
    if ( TEST_ROBOT_AUDIO_DEV_LOG ) {
      PRINT_NAMED_INFO( "RobotAudioTest.AnimationRunLoopFunc.generalUsesLoopFunc",
                       "Stream Completed" );
    }
  }
  
  // Clean up
//  if ( audioFrame != nullptr ) {
//    Util::SafeDelete( audioFrame );
//  }
  
  return false;
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
  
  config.LoadAudioBuffer(*((RobotAudioTestBuffer*)audioClient.GetRobotAudioBuffer(GameObjectType::CozmoBus_1)));
  
  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
  RunAnimation_AudioAnimation( audioAnimation, config, noAudioEventLoopFunc );
  
  
  // Test Streaming animations
  StreamingAnimationTest streamingAnim("NoEventsTest",
                                       GameObjectType::CozmoBus_1,
                                       audioClient.GetRobotAudioBuffer(GameObjectType::CozmoBus_1),
                                       audioClient);
  config.LoadStreamingAnimation(streamingAnim);

  RunAnimation_StreamingAnim(streamingAnim, config, noAudioEventLoopStreamingAnimFunc);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Single Streams
TEST_F(RobotAudioTest, TestSingleEventAnimation_FirstFrame )
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  // Currently, RobotAudioTest doesn't let me test an event starting @ 0 ms, ironic.
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 1, 100) );
  config.InsertComplete();
  EXPECT_EQ( 1, config.GetAudioEvents().size() );
  
//  config.LoadAudioKeyFrames( *animation );
  
  config.LoadAudioBuffer(*((RobotAudioTestBuffer*)audioClient.GetRobotAudioBuffer(GameObjectType::CozmoBus_1)));
//  
//  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
//  RunAnimation_AudioAnimation( audioAnimation, config, generalUsesLoopFunc );
  
  // Test Streaming animations
  StreamingAnimationTest streamingAnim("EventsTest",
                                       GameObjectType::CozmoBus_1,
                                       audioClient.GetRobotAudioBuffer(GameObjectType::CozmoBus_1),
                                       audioClient);
  
  config.LoadAudioKeyFrames( streamingAnim );
  config.LoadStreamingAnimation( streamingAnim );
  
  RunAnimation_StreamingAnim(streamingAnim, config, generalUsesLoopStreamingAnimFunc);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Single Streams that is not at in the first frame
TEST_F(RobotAudioTest, TestSingleEventAnimation )
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 200, 100) );
  config.InsertComplete();
  EXPECT_EQ( 1, config.GetAudioEvents().size() );
  
//  config.LoadAudioKeyFrames( *animation );
//  
//  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudioBuffer( Anki::Cozmo::Audio::GameObjectType::CozmoBus_1 )) );
//  
//  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
//  
//  RunAnimation_AudioAnimation( audioAnimation, config, generalUsesLoopFunc );
  
  // Test Streaming animations
  StreamingAnimationTest streamingAnim("EventsTest",
                                       GameObjectType::CozmoBus_1,
                                       audioClient.GetRobotAudioBuffer(GameObjectType::CozmoBus_1),
                                       audioClient);
  
  config.LoadAudioKeyFrames( streamingAnim );
  config.LoadStreamingAnimation( streamingAnim );
  
  RunAnimation_StreamingAnim(streamingAnim, config, generalUsesLoopStreamingAnimFunc);
}

// Test single stream multiple events
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Events end in order they were triggered
TEST_F(RobotAudioTest, TestSingleStream_MultipleEvents_EndInOrder)
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 1, 20) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 10, 40) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 30, 100) );
  config.InsertComplete();
  EXPECT_EQ( 3, config.GetAudioEvents().size() );
  
//  config.LoadAudioKeyFrames( *animation );
//  
//  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudioBuffer( Anki::Cozmo::Audio::GameObjectType::CozmoBus_1 )) );
//  
//  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
//  
//  RunAnimation_AudioAnimation( audioAnimation, config, generalUsesLoopFunc );
  
  // Test Streaming animations
  StreamingAnimationTest streamingAnim("EventsTest",
                                       GameObjectType::CozmoBus_1,
                                       audioClient.GetRobotAudioBuffer(GameObjectType::CozmoBus_1),
                                       audioClient);
  
  config.LoadAudioKeyFrames( streamingAnim );
  config.LoadStreamingAnimation( streamingAnim );
  
  RunAnimation_StreamingAnim(streamingAnim, config, generalUsesLoopStreamingAnimFunc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Events that end out of order of when they were triggered
TEST_F(RobotAudioTest, TestSingleStream_MultipleEvents_EndInReverseOrder)
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 1, 100) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 10, 80) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 30, 40) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 40, 10) );
  config.InsertComplete();
  EXPECT_EQ( 4, config.GetAudioEvents().size() );
  
//  config.LoadAudioKeyFrames( *animation );
//  
//  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudioBuffer( Anki::Cozmo::Audio::GameObjectType::CozmoBus_1 )) );
//  
//  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
//  
//  RunAnimation_AudioAnimation( audioAnimation, config, generalUsesLoopFunc );
  
  // Test Streaming animations
  StreamingAnimationTest streamingAnim("EventsTest",
                                       GameObjectType::CozmoBus_1,
                                       audioClient.GetRobotAudioBuffer(GameObjectType::CozmoBus_1),
                                       audioClient);
  
  config.LoadAudioKeyFrames( streamingAnim );
  config.LoadStreamingAnimation( streamingAnim );
  
  RunAnimation_StreamingAnim(streamingAnim, config, generalUsesLoopStreamingAnimFunc);
}

// Test Multiple Streams
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Multiple Streams with a Single Event
TEST_F(RobotAudioTest, TestMultipleStreams_SingleEvent)
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 1, 100) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 200, 100) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 400, 100) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 580, 12) );
  config.InsertComplete();
  EXPECT_EQ( 4, config.GetAudioEvents().size() );
  
//  config.LoadAudioKeyFrames( *animation );
//  
//  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudioBuffer( GameObjectType::CozmoBus_1 )) );
//  
//  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
//  
//  RunAnimation_AudioAnimation( audioAnimation, config, generalUsesLoopFunc );
  // Test Streaming animations
  StreamingAnimationTest streamingAnim("EventsTest",
                                       GameObjectType::CozmoBus_1,
                                       audioClient.GetRobotAudioBuffer(GameObjectType::CozmoBus_1),
                                       audioClient);
  
  config.LoadAudioKeyFrames( streamingAnim );
  config.LoadStreamingAnimation( streamingAnim );
  
  RunAnimation_StreamingAnim(streamingAnim, config, generalUsesLoopStreamingAnimFunc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Multiple Streams with multiple Events
TEST_F(RobotAudioTest, TestMultipleStreams_MultipleEvents)
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 1, 100) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 4, 100) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 20, 100) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 200, 100) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 400, 100) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 580, 12) );
  config.InsertComplete();
  EXPECT_EQ( 6, config.GetAudioEvents().size() );
  
//  config.LoadAudioKeyFrames( *animation );
//  
//  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudioBuffer( Anki::Cozmo::Audio::GameObjectType::CozmoBus_1 )) );
//  
//  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
//  
//  RunAnimation_AudioAnimation( audioAnimation, config, generalUsesLoopFunc );
  
  // Test Streaming animations
  StreamingAnimationTest streamingAnim("EventsTest",
                                       GameObjectType::CozmoBus_1,
                                       (RobotAudioTestBuffer*)audioClient.GetRobotAudioBuffer( GameObjectType::CozmoBus_1 ),
                                       audioClient);
  
  config.LoadAudioKeyFrames( streamingAnim );
  config.LoadStreamingAnimation( streamingAnim );
  
  RunAnimation_StreamingAnim(streamingAnim, config, generalUsesLoopStreamingAnimFunc);
}

// Test Back to Back Streams
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Back to Back Streams in same frame
TEST_F(RobotAudioTest, TestMultipleStreams_BackToBack_SameFrame)
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 1, 100) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 102, 100) );
  config.InsertComplete();
  EXPECT_EQ( 2, config.GetAudioEvents().size() );
  
//  config.LoadAudioKeyFrames( *animation );
//  
//  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudioBuffer( GameObjectType::CozmoBus_1 )) );
//  
//  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
//  
//  RunAnimation_AudioAnimation( audioAnimation, config, generalUsesLoopFunc );

  // Test Streaming animations
  StreamingAnimationTest streamingAnim("EventsTest",
                                       GameObjectType::CozmoBus_1,
                                       (RobotAudioTestBuffer*)audioClient.GetRobotAudioBuffer( GameObjectType::CozmoBus_1 ),
                                       audioClient);
  
  config.LoadAudioKeyFrames( streamingAnim );
  config.LoadStreamingAnimation( streamingAnim );
  
  RunAnimation_StreamingAnim(streamingAnim, config, generalUsesLoopStreamingAnimFunc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Back to Back Streams Frame Boarder
TEST_F(RobotAudioTest, TestMultipleStreams_BackToBack_NextFrame)
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 1, 66) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 67, 100) );
  config.InsertComplete();
  EXPECT_EQ( 2, config.GetAudioEvents().size() );
  
//  config.LoadAudioKeyFrames( *animation );
//  
//  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudioBuffer( GameObjectType::CozmoBus_1 )) );
//  
//  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
//  
//  RunAnimation_AudioAnimation( audioAnimation, config, generalUsesLoopFunc );

  // Test Streaming animations
  StreamingAnimationTest streamingAnim("EventsTest",
                                       GameObjectType::CozmoBus_1,
                                       audioClient.GetRobotAudioBuffer(GameObjectType::CozmoBus_1),
                                       audioClient);
  
  config.LoadAudioKeyFrames( streamingAnim );
  config.LoadStreamingAnimation( streamingAnim );
  
  RunAnimation_StreamingAnim(streamingAnim, config, generalUsesLoopStreamingAnimFunc);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Test Events on frame edges
// FIXME: There is a bug with how audio test data is created & Popped in Test
TEST_F(RobotAudioTest, TestFrameEdgeCase_StartsOnEdge)
{
  EXPECT_EQ( 0, config.GetAudioEvents().size() );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 33, 10) );
  config.Insert( AnimationAudioTestConfig::TestAudioEvent(GameEvent::GenericEvent::Play__Sfx__Placeholder, 133, 100) );
  config.InsertComplete();
  EXPECT_EQ( 2, config.GetAudioEvents().size() );
  
//  config.LoadAudioKeyFrames( *animation );
//  
//  config.LoadAudioBuffer( *((RobotAudioTestBuffer*)audioClient.GetRobotAudioBuffer( Anki::Cozmo::Audio::GameObjectType::CozmoBus_1 )) );
//  
//  RobotAudioAnimationOnRobotTest audioAnimation( animation, &audioClient );
//  
//  RunAnimation_AudioAnimation( audioAnimation, config, generalUsesLoopFunc );
  
  // Test Streaming animations
  StreamingAnimationTest streamingAnim("EventsTest",
                                       GameObjectType::CozmoBus_1,
                                       audioClient.GetRobotAudioBuffer(GameObjectType::CozmoBus_1),
                                       audioClient);
  
  config.LoadAudioKeyFrames( streamingAnim );
  config.LoadStreamingAnimation( streamingAnim );
  
  RunAnimation_StreamingAnim(streamingAnim, config, generalUsesLoopStreamingAnimFunc);
}

// TODO: Add test for more edge cases!!
