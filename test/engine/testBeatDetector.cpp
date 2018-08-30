/**
 * File: testBeatDetector
 *
 * Author: Matt Michini
 * Created: 08/25/2018
 *
 * Description: Tests for the engine's beat detector component
 *
 * Copyright: Anki, Inc. 2018
 *
 * --gtest_filter=BeatDetectorComponentTest.*
 **/

#include "util/helpers/includeGTest.h"

#define private public
#define protected public

#include "engine/components/mics/beatDetectorComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"

#include "test/engine/helpers/messaging/stubRobotMessageHandler.h"

#include "util/time/universalTime.h"

using namespace Anki;
using namespace Vector;

extern CozmoContext* cozmoContext;


class BeatDetectorComponentTest : public testing::Test
{
protected:
  
  virtual void SetUp() override {
    _msgHandler = new StubMessageHandler;
    
    // stub in our own outgoing message handler
    cozmoContext->GetRobotManager()->_robotMessageHandler.reset(_msgHandler);
    
    _robot.reset(new Robot(1, cozmoContext));
    _beatDetectorComponent = &(_robot->GetBeatDetectorComponent());
    
    _robot->FakeSyncRobotAck();
    
    // Fake a state message update for robot
    RobotState stateMsg = _robot->GetDefaultRobotState();
    
    bool result = _robot->UpdateFullRobotState(stateMsg);
    ASSERT_EQ(result, RESULT_OK);
  }
  
private:
  std::unique_ptr<Robot> _robot;
  BeatDetectorComponent* _beatDetectorComponent = nullptr;
  StubMessageHandler* _msgHandler = nullptr;
  
  // Convenience function to execute one tick of beat detector component
  void Update(Anki::Vector::BeatDetectorComponent *beatDetectorComponent)
  {
    static const Anki::Vector::RobotCompMap dependentComps;
    beatDetectorComponent->UpdateDependent(dependentComps);
  }
  
};

TEST_F(BeatDetectorComponentTest, Create)
{
  ASSERT_TRUE(_robot != nullptr);
  ASSERT_TRUE(_beatDetectorComponent != nullptr);
}

TEST_F(BeatDetectorComponentTest, OnBeatCallback)
{
  int nCallbacks = 0;
  auto callback = [&nCallbacks]() {
    ++nCallbacks;
  };
  
  const auto callbackId = _beatDetectorComponent->RegisterOnBeatCallback(callback);
  ASSERT_GT(callbackId, 0) << "Callback ID expected to be positive";
  
  BeatInfo beat{120.f, 1.f, 0.f};
  
  _beatDetectorComponent->OnBeat(beat);
  
  ASSERT_EQ(nCallbacks, 1) << "Expected callback to be called";
  
  const bool result = _beatDetectorComponent->UnregisterOnBeatCallback(callbackId);
  ASSERT_TRUE(result) << "Failed unregistering OnBeat callback";
  
  _beatDetectorComponent->OnBeat(beat);
  ASSERT_EQ(nCallbacks, 1) << "Expected callback _not_ to be called";
}

TEST_F(BeatDetectorComponentTest, DISABLED_PossibleBeatDetection)
{
  _beatDetectorComponent->Reset();
  
  ASSERT_FALSE(_beatDetectorComponent->IsPossibleBeatDetected()) << "Unexpected possible beat detected";
  ASSERT_FALSE(_beatDetectorComponent->IsBeatDetected()) << "Unexpected beat detected";
  
  // Note: Using UniversalTime is a bit hairy here, since it will use the system time. But within a reasonable margin
  // this should be fine.
  const float now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
  
  // Generate a series of fake, low confidence beats
  const float startTimeAgo_sec = 20.f; // Start the sequence 20 seconds ago.
  const float kFakeTempo_bpm = 100.f;
  int nFakeBeats = 0;
  for (float beatTime = now_sec - startTimeAgo_sec;
       beatTime < now_sec;
       beatTime += 60.f / kFakeTempo_bpm) {
    const auto& fakeBeat = _beatDetectorComponent->TEST_fakeLowConfidenceBeat(kFakeTempo_bpm, beatTime);
    _beatDetectorComponent->OnBeat(fakeBeat);
    ++nFakeBeats;
  }
  
  // Updating the beat detector component should cull the list to a more recent window
  Update(_beatDetectorComponent);
  
  ASSERT_LT(_beatDetectorComponent->_recentBeats.size(), nFakeBeats) << "Expected beats to be culled from list";
  
  // We should not have a detected beat right now.
  ASSERT_FALSE(_beatDetectorComponent->IsPossibleBeatDetected()) << "Unexpected possible beat detected";
  ASSERT_FALSE(_beatDetectorComponent->IsBeatDetected()) << "Unexpected beat detected";
  
  // If we replace the latest beat to have a high confidence, we should have a possible beat
  auto& lastBeat = _beatDetectorComponent->_recentBeats.back();
  lastBeat = _beatDetectorComponent->TEST_fakeHighConfidenceBeat(lastBeat.tempo_bpm, lastBeat.time_sec);
  
  ASSERT_TRUE(_beatDetectorComponent->IsPossibleBeatDetected()) << "Expected possible beat detected";
  ASSERT_FALSE(_beatDetectorComponent->IsBeatDetected()) << "Unexpected beat detected";
  
  // If the latest beat has a _very_ high confidence, we should also have a normal beat detected
  lastBeat.confidence = 10000.f;
  
  ASSERT_TRUE(_beatDetectorComponent->IsPossibleBeatDetected()) << "Expected possible beat detected";
  ASSERT_TRUE(_beatDetectorComponent->IsBeatDetected()) << "Expected beat detected";
  
  // If we modify the latest beat with a different tempo, no beat should be detected
  lastBeat.tempo_bpm *= 1.5;
  
  // We should not have a detected beat right now.
  ASSERT_FALSE(_beatDetectorComponent->IsPossibleBeatDetected()) << "Unexpected possible beat detected";
  ASSERT_FALSE(_beatDetectorComponent->IsBeatDetected()) << "Unexpected beat detected";
  
  _beatDetectorComponent->Reset();
  
  ASSERT_FALSE(_beatDetectorComponent->IsPossibleBeatDetected()) << "Unexpected possible beat detected";
  ASSERT_FALSE(_beatDetectorComponent->IsBeatDetected()) << "Unexpected beat detected";
}


TEST_F(BeatDetectorComponentTest, DISABLED_BeatDetection)
{
  _beatDetectorComponent->Reset();
  
  ASSERT_FALSE(_beatDetectorComponent->IsPossibleBeatDetected()) << "Unexpected possible beat detected";
  ASSERT_FALSE(_beatDetectorComponent->IsBeatDetected()) << "Unexpected beat detected";
  
  // Note: Using UniversalTime is a bit hairy here, since it will use the system time. But within a reasonable margin
  // this should be fine.
  const float now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
  
  // Generate a series of fake, high confidence beats
  const float startTimeAgo_sec = 20.f; // Start the sequence 20 seconds ago.
  const float kFakeTempo_bpm = 100.f;
  int nFakeBeats = 0;
  for (float beatTime = now_sec - startTimeAgo_sec;
       beatTime < now_sec;
       beatTime += 60.f / kFakeTempo_bpm) {
    const auto& fakeBeat = _beatDetectorComponent->TEST_fakeHighConfidenceBeat(kFakeTempo_bpm, beatTime);
    _beatDetectorComponent->OnBeat(fakeBeat);
    ++nFakeBeats;
  }
  
  // Updating the beat detector component should cull the list to a more recent window
  Update(_beatDetectorComponent);
  
  ASSERT_LT(_beatDetectorComponent->_recentBeats.size(), nFakeBeats) << "Expected beats to be culled from list";
  
  ASSERT_TRUE(_beatDetectorComponent->IsPossibleBeatDetected()) << "Expected possible beat detected";
  ASSERT_TRUE(_beatDetectorComponent->IsBeatDetected()) << "Expected beat detected";
  
  _beatDetectorComponent->Reset();
  
  ASSERT_FALSE(_beatDetectorComponent->IsPossibleBeatDetected()) << "Unexpected possible beat detected";
  ASSERT_FALSE(_beatDetectorComponent->IsBeatDetected()) << "Unexpected beat detected";
}




