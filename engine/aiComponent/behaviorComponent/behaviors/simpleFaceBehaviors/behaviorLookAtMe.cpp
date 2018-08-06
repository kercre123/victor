/**
 * File: behaviorLookAtMe.cpp
 *
 * Author: ross
 * Created: 2018-06-22
 *
 * Description: Behavior for tracking a face
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorLookAtMe.h"

#include "engine/actions/trackFaceAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/faceWorld.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const char* const kPanToleranceKey = "panTolerance_deg";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAtMe::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAtMe::DynamicVariables::DynamicVariables()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAtMe::BehaviorLookAtMe(const Json::Value& config)
  : ISimpleFaceBehavior(config)
{
  _iConfig.panTolerance_deg = config.get(kPanToleranceKey, -1.0f).asFloat();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtMe::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActiveScope->insert( {VisionMode::DetectingFaces, EVisionUpdateFrequency::High} );
  modifiers.behaviorAlwaysDelegates = true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtMe::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kPanToleranceKey );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorLookAtMe::WantsToBeActivatedBehavior() const
{
  return GetTargetFace().IsValid();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtMe::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace( GetTargetFace() );
  if(facePtr != nullptr){
    Vision::FaceID_t faceID = facePtr->GetID();
    TrackFaceAction* trackAction = new TrackFaceAction( faceID );
    if( _iConfig.panTolerance_deg >= 0.0f ) {
      trackAction->SetPanTolerance( DEG_TO_RAD(_iConfig.panTolerance_deg) );
    }
    const bool onCharger = GetBEI().GetRobotInfo().IsOnChargerContacts();
    if( onCharger ) {
      trackAction->SetMode( ITrackAction::Mode::HeadOnly );
    }
    DelegateIfInControl( trackAction );
  }
}
  

}
}
