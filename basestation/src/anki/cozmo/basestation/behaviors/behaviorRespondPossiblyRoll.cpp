/**
 * File: behaviorRespondPossiblyRoll.cpp
 *
 * Author: Kevin M. Karol
 * Created: 01/23/17
 *
 * Description: Behavior that turns towards a block, plays an animation
 * and then rolls it if the block is on its side
 *
 * Copyright: Anki, Inc. 2017 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorRespondPossiblyRoll.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRespondPossiblyRoll.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/utils/timer.h"


namespace Anki {
namespace Cozmo {
  
namespace{
  
static const std::vector<AnimationTrigger> kUprightAnims = {
  AnimationTrigger::BuildPyramidFirstBlockUpright,
  AnimationTrigger::BuildPyramidSecondBlockUpright,
  AnimationTrigger::BuildPyramidThirdBlockUpright
};

static const std::vector<AnimationTrigger> kOnSideAnims = {
  AnimationTrigger::BuildPyramidFirstBlockOnSide,
  AnimationTrigger::BuildPyramidSecondBlockOnSide,
  AnimationTrigger::BuildPyramidThirdBlockOnSide
};
  
}
  
using namespace ExternalInterface;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRespondPossiblyRoll::BehaviorRespondPossiblyRoll(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("RespondPossiblyRoll");
  
  // Listen for up-axis changes to update response scenarios
  auto upAxisChangedCallback = [this](const EngineToGameEvent& event) {
    _upAxisChangedIDs.insert(std::make_pair(
                  event.GetData().Get_ObjectUpAxisChanged().objectID,
                  event.GetData().Get_ObjectUpAxisChanged().upAxis)
                             );
  };
  
  if(robot.HasExternalInterface()){
    using namespace ExternalInterface;
    _eventHalders.push_back(robot.GetExternalInterface()->Subscribe(
                      MessageEngineToGameTag::ObjectUpAxisChanged,
                      upAxisChangedCallback));
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRespondPossiblyRoll::~BehaviorRespondPossiblyRoll()
{  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorRespondPossiblyRoll::IsRunnableInternal(const BehaviorPreReqRespondPossiblyRoll& preReqData) const
{
  
  _metadata = RespondPossiblyRollMetadata(preReqData.GetObjectID(),
                                          preReqData.GetUprightAnimIndex(),
                                          preReqData.GetOnSideAnimIndex(),
                                          preReqData.GetPoseUpAxisAccurate());
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorRespondPossiblyRoll::InitInternal(Robot& robot)
{
  _lastActionTag = ActionConstants::INVALID_TAG;
  _upAxisChangedIDs.clear();

  DetermineNextResponse(robot);

  return Result::RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorRespondPossiblyRoll::ResumeInternal(Robot& robot)
{
  _metadata.SetPlayedOnSideAnim();
  return InitInternal(robot);
}

  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorRespondPossiblyRoll::UpdateInternal(Robot& robot)
{
  const auto& targetAxisChanged = _upAxisChangedIDs.find(_metadata.GetObjectID());
  
  if(targetAxisChanged != _upAxisChangedIDs.end()){
    StopActing(false, false);
    if(targetAxisChanged->second != UpAxis::ZPositive){
      TurnAndRespondNegatively(robot);
    }
  }
  
  _upAxisChangedIDs.clear();
  return IBehavior::UpdateInternal(robot);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondPossiblyRoll::DetermineNextResponse(Robot& robot)
{
  // Ensure behavior cube lights have been cleared
  std::vector<BehaviorStateLightInfo> basePersistantLight;
  SetBehaviorStateLights(basePersistantLight, false);
  
  ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(_metadata.GetObjectID());
  if(nullptr != object){
    if (!_metadata.GetPoseUpAxisAccurate() ||
        (object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() != AxisName::Z_POS))
    {
      _metadata.SetPoseUpAxisWillBeChecked();
      TurnAndRespondNegatively(robot);
    }else{
      TurnAndRespondPositively(robot);
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondPossiblyRoll::TurnAndRespondPositively(Robot& robot)
{
  CompoundActionSequential* turnAndReact = new CompoundActionSequential(robot);
  turnAndReact->AddAction(new TurnTowardsObjectAction(robot, _metadata.GetObjectID(), Radians(M_PI_F), true));
  const unsigned long animIndex = _metadata.GetUprightAnimIndex() < kUprightAnims.size() ?
                                         _metadata.GetUprightAnimIndex() : kUprightAnims.size() - 1;
  turnAndReact->AddAction(new TriggerLiftSafeAnimationAction(robot, kUprightAnims[animIndex]));
  StartActing(turnAndReact, [this](ActionResult result){
    if((result == ActionResult::SUCCESS) ||
       (result == ActionResult::VISUAL_OBSERVATION_FAILED)){
      _metadata.SetPlayedUprightAnim();
    }
  });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondPossiblyRoll::TurnAndRespondNegatively(Robot& robot)
{
  
  CompoundActionSequential* turnAndReact = new CompoundActionSequential(robot);
  turnAndReact->AddAction(new TurnTowardsObjectAction(robot, _metadata.GetObjectID(), Radians(M_PI_F), true));
  if((!_metadata.GetPlayedOnSideAnim()) &&
     (_metadata.GetOnSideAnimIndex() >= 0)){
    const unsigned long animIndex =  _metadata.GetOnSideAnimIndex() < kOnSideAnims.size() ?
                                          _metadata.GetOnSideAnimIndex() : kOnSideAnims.size() - 1;
    turnAndReact->AddAction(new TriggerLiftSafeAnimationAction(robot, kOnSideAnims[animIndex]));
    _metadata.SetPlayedOnSideAnim();
  }
  
  StartActing(turnAndReact, &BehaviorRespondPossiblyRoll::DelegateToRollHelper);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondPossiblyRoll::DelegateToRollHelper(Robot& robot)
{
  ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(_metadata.GetObjectID());
  if ((nullptr != object) &&
      (object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() != AxisName::Z_POS)
      )
  {
    auto& factory = robot.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
    const bool upright = true;
    RollBlockParameters parameters;
    parameters.preDockCallback = [this](Robot& robot){_metadata.SetReachedPreDockRoll();};
    HelperHandle rollHelper = factory.CreateRollBlockHelper(robot,
                                                            *this,
                                                            _metadata.GetObjectID(),
                                                            upright,
                                                            parameters);
    
    SmartDelegateToHelper(robot, rollHelper, [this](Robot& robot){DetermineNextResponse(robot);}, nullptr);
    // Set the cube lights to interacting for full behavior run time
    std::vector<BehaviorStateLightInfo> basePersistantLight;
    basePersistantLight.push_back(
      BehaviorStateLightInfo(_metadata.GetObjectID(), CubeAnimationTrigger::InteractingBehaviorLock)
    );
    SetBehaviorStateLights(basePersistantLight, false);
  }
}

} // namespace Cozmo
} // namespace Anki
