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

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/buildPyramid/behaviorRespondPossiblyRoll.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/behaviorHelperFactory.h"
#include "engine/blockWorld/blockWorld.h"
#include "coretech/common/engine/utils/timer.h"


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
BehaviorRespondPossiblyRoll::BehaviorRespondPossiblyRoll(const Json::Value& config)
: ICozmoBehavior(config)
{
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRespondPossiblyRoll::~BehaviorRespondPossiblyRoll()
{  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondPossiblyRoll::InitBehavior()
{
  SubscribeToTags({ExternalInterface::MessageEngineToGameTag::ObjectUpAxisChanged});
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorRespondPossiblyRoll::WantsToBeActivatedBehavior() const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondPossiblyRoll::OnBehaviorActivated()
{
  _lastActionTag = ActionConstants::INVALID_TAG;
  _upAxisChangedIDs.clear();

  DetermineNextResponse();

  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondPossiblyRoll::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  const auto& targetAxisChanged = _upAxisChangedIDs.find(_metadata.GetObjectID());
  
  if(targetAxisChanged != _upAxisChangedIDs.end()){
    CancelDelegates(false, false);
    if(targetAxisChanged->second != UpAxis::ZPositive){
      TurnAndRespondNegatively();
    }
  }
  
  _upAxisChangedIDs.clear();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondPossiblyRoll::DetermineNextResponse()
{  
  const ObservableObject* object = GetBEI().GetBlockWorld().GetLocatedObjectByID(_metadata.GetObjectID());
  if(nullptr != object){
    if (!_metadata.GetPoseUpAxisAccurate() ||
        (object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() != AxisName::Z_POS))
    {
      _metadata.SetPoseUpAxisWillBeChecked();
      TurnAndRespondNegatively();
    }else{
      TurnAndRespondPositively();
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondPossiblyRoll::TurnAndRespondPositively()
{
  DEBUG_SET_STATE(RespondingPositively);
  
  CompoundActionSequential* turnAndReact = new CompoundActionSequential();
  turnAndReact->AddAction(new TurnTowardsObjectAction(_metadata.GetObjectID(), Radians(M_PI_F), true));
  const unsigned long animIndex = _metadata.GetUprightAnimIndex() < kUprightAnims.size() ?
                                         _metadata.GetUprightAnimIndex() : kUprightAnims.size() - 1;
  turnAndReact->AddAction(new TriggerLiftSafeAnimationAction(kUprightAnims[animIndex]));
  DelegateIfInControl(turnAndReact, [this](ActionResult result){
    if((result == ActionResult::SUCCESS) ||
       (result == ActionResult::VISUAL_OBSERVATION_FAILED)){
      _metadata.SetPlayedUprightAnim();
    }
  });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondPossiblyRoll::TurnAndRespondNegatively()
{
  DEBUG_SET_STATE(RespondingNegatively);
  
  CompoundActionSequential* turnAndReact = new CompoundActionSequential();
  turnAndReact->AddAction(new TurnTowardsObjectAction(_metadata.GetObjectID(), Radians(M_PI_F), true));
  if((!_metadata.GetPlayedOnSideAnim()) &&
     (_metadata.GetOnSideAnimIndex() >= 0)){
    const unsigned long animIndex =  _metadata.GetOnSideAnimIndex() < kOnSideAnims.size() ?
                                          _metadata.GetOnSideAnimIndex() : kOnSideAnims.size() - 1;
    turnAndReact->AddAction(new TriggerLiftSafeAnimationAction(kOnSideAnims[animIndex]));
    _metadata.SetPlayedOnSideAnim();
  }
  
  DelegateIfInControl(turnAndReact, &BehaviorRespondPossiblyRoll::DelegateToRollHelper);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondPossiblyRoll::DelegateToRollHelper()
{
  DEBUG_SET_STATE(RollingObject);

  auto& factory = GetBEI().GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
  const bool upright = true;
  RollBlockParameters parameters;
  parameters.preDockCallback = [this](Robot& robot){_metadata.SetReachedPreDockRoll();};
  HelperHandle rollHelper = factory.CreateRollBlockHelper(*this,
                                                          _metadata.GetObjectID(),
                                                          upright,
                                                          parameters);
  
  SmartDelegateToHelper(rollHelper, [this](){DetermineNextResponse();}, nullptr);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondPossiblyRoll::AlwaysHandleInScope(const EngineToGameEvent& event)
{
  if(event.GetData().GetTag() ==ExternalInterface::MessageEngineToGameTag::ObjectUpAxisChanged){
    _upAxisChangedIDs.insert(std::make_pair(event.GetData().Get_ObjectUpAxisChanged().objectID,
                                            event.GetData().Get_ObjectUpAxisChanged().upAxis)
                            );
  }
}


} // namespace Cozmo
} // namespace Anki
