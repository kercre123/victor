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

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/faceSelectionComponent.h"
#include "engine/faceWorld.h"

namespace Anki {
namespace Vector {

namespace{
const TimeStamp_t kSeenFaceWindow_ms = 5000;
const int kNumFramesToWait = 25;
}  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAtFaceInFront::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAtFaceInFront::DynamicVariables::DynamicVariables()
{
  waitingForFaces = false;
  robotTimeStampAtActivation = 0;
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
void BehaviorLookAtFaceInFront::BehaviorUpdate()
{
  if( _dVars.waitingForFaces ) {
    
    SmartFaceID smartID;
    if(GetFaceIDToLookAt(smartID)){
      CancelDelegates(false);
      _dVars.waitingForFaces = false;
      TransitionToLookAtFace(smartID);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtFaceInFront::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  _dVars.robotTimeStampAtActivation = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
  
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

  const RobotTimeStamp_t currentTime_ms = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
  const RobotTimeStamp_t earliestTime = std::max( currentTime_ms - kSeenFaceWindow_ms, _dVars.robotTimeStampAtActivation );
  const bool includeRecognizableOnly = false;
  std::vector<SmartFaceID> ids;
  const bool res = selectionComp.AreFacesInFrontOfRobot(ids, earliestTime, includeRecognizableOnly);

  if(res){
    smartID = ids[0];
  }

  return res;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtFaceInFront::TransitionToLookUp()
{
  // wait for a few frames after turning to give the robot a chance to see the face
  auto* action = new CompoundActionSequential;
  action->AddAction(new MoveHeadToAngleAction(MAX_HEAD_ANGLE));
  // todo: use WaitToSeeFaceAction (VIC-4789)
  action->AddAction(new WaitForImagesAction(kNumFramesToWait,
                                            VisionMode::DetectingFaces));
  
  _dVars.waitingForFaces = true;
  DelegateIfInControl(action); // either BehaviorUpdate will break out of this, or the behavior ends
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAtFaceInFront::TransitionToLookAtFace(const SmartFaceID& smartID)
{
  auto* action = new TurnTowardsFaceAction(smartID);
  action->SetRequireFaceConfirmation(true);
  DelegateIfInControl(action);
}


}
}
