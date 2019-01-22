/**
 * File: BehaviorBoxDemoFaceTracking.cpp
 *
 * Author: Andrew Stein
 * Created: 2019-01-04
 *
 * Description: Demo of face tracking for The Box demo
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/box/behaviorBoxDemoFaceTracking.h"
#include "engine/components/visionComponent.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/faceWorld.h"

namespace Anki {
namespace Vector {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoFaceTracking::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoFaceTracking::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoFaceTracking::BehaviorBoxDemoFaceTracking(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // TODO: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoFaceTracking::~BehaviorBoxDemoFaceTracking()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBoxDemoFaceTracking::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoFaceTracking::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = false;
  modifiers.visionModesForActiveScope->insert({VisionMode::MirrorMode,              EVisionUpdateFrequency::High});
  modifiers.visionModesForActiveScope->insert({VisionMode::DetectingFaces,          EVisionUpdateFrequency::High});
  //modifiers.visionModesForActiveScope->insert({VisionMode::FlipScreenOnCloseFace,   EVisionUpdateFrequency::High});
  modifiers.visionModesForActiveScope->insert({VisionMode::OffboardFaceRecognition, EVisionUpdateFrequency::High});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoFaceTracking::OnBehaviorActivated()
{
  // reset dynamic variables
  _dVars = DynamicVariables();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoFaceTracking::BehaviorUpdate()
{
  return;
}

}
}
