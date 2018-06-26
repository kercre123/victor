/**
 * File: BehaviorLookAtFaceInFront.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-24
 *
 * Description: Robot looks up to see if there's a face in front of it, and centers on a face if found
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorLookAtFaceInFront.h"

#include "engine/aiComponent/faceSelectionComponent.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/basicActions.h"
#include "engine/faceWorld.h"

namespace Anki {
namespace Cozmo {

namespace{
const TimeStamp_t kSeenFaceWindow_ms = 5000;
}  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAtFaceInFront::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAtFaceInFront::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAtFaceInFront::BehaviorLookAtFaceInFront(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAtFaceInFront::~BehaviorLookAtFaceInFront()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorLookAtFaceInFront::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtFaceInFront::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtFaceInFront::GetAllDelegates(std::set<IBehavior*>& delegates) const
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtFaceInFront::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtFaceInFront::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  SmartFaceID smartID;
  if(GetFaceIDToLookAt(smartID)){
    TransitionToLookAtFace(smartID);
  }else{
    TransitionToLookUp();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorLookAtFaceInFront::GetFaceIDToLookAt(SmartFaceID& smartID) const
{
  auto& selectionComp = GetAIComp<FaceSelectionComponent>();

  const TimeStamp_t currentTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  const bool includeRecognizableOnly = false;
  std::vector<SmartFaceID> ids;
  const bool res = selectionComp.AreFacesInFrontOfRobot(ids, currentTime_ms - kSeenFaceWindow_ms, includeRecognizableOnly);

  if(res){
    smartID = ids[0];
  }

  return res;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtFaceInFront::TransitionToLookUp()
{
  const bool isPanAbsolute = false;
  const bool isTiltAbsolute = true;
  DelegateIfInControl(new PanAndTiltAction(0, DEG_TO_RAD(50), isPanAbsolute, isTiltAbsolute), [this](){
    SmartFaceID smartID;
    if(GetFaceIDToLookAt(smartID)){
      TransitionToLookAtFace(smartID);
    }
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtFaceInFront::TransitionToLookAtFace(const SmartFaceID& smartID)
{
  const Vision::TrackedFace* trackedFace =  GetBEI().GetFaceWorld().GetFace(smartID);
  if(trackedFace != nullptr){
    DelegateIfInControl(new TurnTowardsPoseAction(trackedFace->GetHeadPose()));
  }
}


}
}
