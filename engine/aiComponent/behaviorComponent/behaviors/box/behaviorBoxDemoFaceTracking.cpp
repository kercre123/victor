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
  modifiers.visionModesForActiveScope->insert({VisionMode::FlipScreenOnCloseFace,   EVisionUpdateFrequency::High});
  modifiers.visionModesForActiveScope->insert({VisionMode::OffboardFaceRecognition, EVisionUpdateFrequency::High});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoFaceTracking::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoFaceTracking::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
//  const char* list[] = {
//    // TODO: insert any possible root-level json keys that this class is expecting.
//    // TODO: replace this method with a simple {} in the header if this class doesn't use the ctor's "config" argument.
//  };
//  expectedKeys.insert( std::begin(list), std::end(list) );
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
  // TODO: monitor for things you care about here
  if( !IsActivated() ) {
    return;
  }
  
  if(_iConfig.playSoundOnNewFace)
  {
    // Check to see if there are any new faces and play a sound if so
    const auto lastImageTime_ms = GetBEI().GetVisionComponent().GetLastProcessedImageTimeStamp();
    
    const FaceWorld& faceWorld = GetBEI().GetFaceWorld();
    auto facesObservedSinceLastTick = faceWorld.GetSmartFaceIDs(lastImageTime_ms);
    bool sawNewFace = false;
    
    for(const auto& smartFaceID : facesObservedSinceLastTick)
    {
      // A face is "new" if it has a non-negative ID (meaning it as been recognized) and we have
      // not seen it before
      const Vision::TrackedFace* face = faceWorld.GetFace(smartFaceID);
      DEV_ASSERT(nullptr != face, "BehaviorBoxDemoFaceTracking.BehaviorUpdate.NullFace");
      if(face->GetID() > 0)
      {
        sawNewFace |= GetAIComp<AIWhiteboard>().InsertNewFace(smartFaceID);
      }
    }
    
    if(sawNewFace)
    {
      using GE = AudioMetaData::GameEvent::GenericEvent;
      using GO = AudioMetaData::GameObjectType;
      GetBEI().GetRobotAudioClient().PostEvent(GE::Play__Robot_Vic_Sfx__Cube_Search_Ping,
                                               GO::Behavior);
    }
  }
}

}
}
