/**
* File: behaviorTurnToFace.cpp
*
* Author: Kevin M. Karol
* Created: 6/6/17
*
* Description: Simple behavior to turn toward a face - face can either be passed
* in as part of WantsToBeActivated, or selected using internal criteria using WantsToBeActivated
* with a robot
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorTurnToFace.h"

#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/faceWorld.h"

namespace Anki {
namespace Cozmo {

namespace {
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTurnToFace::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTurnToFace::DynamicVariables::DynamicVariables()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTurnToFace::BehaviorTurnToFace(const Json::Value& config)
: ICozmoBehavior(config)
{
  
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTurnToFace::WantsToBeActivatedBehavior() const
{
  Pose3d wastedPose;
  TimeStamp_t lastTimeObserved = GetBEI().GetFaceWorld().GetLastObservedFace(wastedPose);
  std::set<Vision::FaceID_t> facesObserved = GetBEI().GetFaceWorld().GetFaceIDsObservedSince(lastTimeObserved);
  if(facesObserved.size() > 0){
    _dVars.targetFace = GetBEI().GetFaceWorld().GetSmartFaceID(*facesObserved.begin());
  }
  
  return _dVars.targetFace.IsValid();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurnToFace::OnBehaviorActivated()
{
  if(_dVars.targetFace.IsValid()){
    DelegateIfInControl(new TurnTowardsFaceAction(_dVars.targetFace)); 
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTurnToFace::OnBehaviorDeactivated()
{
  _dVars.targetFace.Reset();
}

  
} // namespace Cozmo
} // namespace Anki
