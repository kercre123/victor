/**
 * File: BehaviorSearchWithinBoundingBox.cpp
 *
 * Author: Lorenzo Riano
 * Created: 2018-07-06
 *
 * Description: Behavior to move the head/body whitin a confined space. The limits of the space are
 *              defined by a bounding box in normalized image coordinates.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "clad/types/salientPointTypes.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorSearchWithinBoundingBox.h"

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Cozmo {

namespace {

const char* const kHowManySearches = "howManySearches";
const char* const kWaitBeforeTurning = "waitBeforeTurning_s";

constexpr bool setRandomSeed = false; // used for testing

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSearchWithinBoundingBox::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSearchWithinBoundingBox::BehaviorSearchWithinBoundingBox(const Json::Value& config)
 : ICozmoBehavior(config)
{
  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";

  _iConfig.howManySearches = JsonTools::ParseInt8(config, kHowManySearches, debugName);
  _iConfig.waitBeforeTurning = JsonTools::ParseFloat(config, kWaitBeforeTurning, debugName);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchWithinBoundingBox::InitBehavior()
{
  if (setRandomSeed) {
    LOG_DEBUG("BehaviorSearchWithinBoundingBox.BehaviorSearchWithinBoundingBox.SettingRandomSeed","");
    GetBEI().GetRNG().SetSeed("BehaviorSearchWithinBoundingBox", 0);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchWithinBoundingBox::SetNewBoundingBox(float minX, float maxX, float minY, float maxY)
{
  _iConfig.minX = minX;
  _iConfig.maxX = maxX;
  _iConfig.minY = minY;
  _iConfig.maxY = maxY;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSearchWithinBoundingBox::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchWithinBoundingBox::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = false;
  modifiers.wantsToBeActivatedWhenOffTreads = false;
  modifiers.wantsToBeActivatedWhenOnCharger = false;
  modifiers.behaviorAlwaysDelegates = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchWithinBoundingBox::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchWithinBoundingBox::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
      kHowManySearches,
      kWaitBeforeTurning,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchWithinBoundingBox::OnBehaviorActivated()
{
  LOG_DEBUG("BehaviorSearchWithinBoundingBox.OnBehaviorActivated.Activated","I am active");

  // reset dynamic variables
  _dVars = DynamicVariables();
  _dVars.numberOfRemainingSearches = _iConfig.howManySearches;

  _dVars.initialTilt = GetBEI().GetRobotInfo().GetHeadAngle();
  _dVars.initialTheta = GetBEI().GetRobotInfo().GetPose().GetRotationAngle();
  
  // Action!
  TransitionToGenerateNewPose();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchWithinBoundingBox::TransitionToGenerateNewPose()
{
  DEBUG_SET_STATE(GenerateNewPose);

  if (_dVars.numberOfRemainingSearches <= 0) {
    LOG_DEBUG("BehaviorSearchWithinBoundingBox::TransitionToGenerateNewPose.Finished",
              "Number of searches reached 0");
    // end of the behavior
    return;
  }
  else {
    LOG_DEBUG("BehaviorSearchWithinBoundingBox::TransitionToGenerateNewPose.RemainingSearches",
              "Number of remaining searches: %d", _dVars.numberOfRemainingSearches);
  }

  // Generate random coordinates within the bounding box
  _dVars.nextX = float(GetBEI().GetRNG().RandDblInRange(_iConfig.minX, _iConfig.maxX));
  _dVars.nextY = float(GetBEI().GetRNG().RandDblInRange(_iConfig.minY, _iConfig.maxY));

  LOG_DEBUG("BehaviorSearchWithinBoundingBox::TransitionToGenerateNewPose.NewCoordinates",
           "New coordinates are (%f, %f) ", _dVars.nextX, _dVars.nextY);

  TransitionToMoveToLookAt();

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchWithinBoundingBox::TransitionToMoveToLookAt()
{
  DEBUG_SET_STATE(MoveToLookAt);
  // Save the initial position to return to later
  _dVars.initialTilt = GetBEI().GetRobotInfo().GetHeadAngle();
  _dVars.initialTheta = GetBEI().GetRobotInfo().GetPose().GetRotationAngle();

  const uint32_t timestamp = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
  // will use the SalientPoint to use the Action that takes image coordinates between 0 and 1
  const Vision::SalientPoint point(timestamp,
                                   _dVars.nextX,
                                   _dVars.nextY,
                                   0,
                                   0,
                                   Vision::SalientPointType::Unknown,
                                   "",
                                   {}
  );
  auto* turnAction =  new TurnTowardsImagePointAction(point);
  CancelDelegates(false);
  // TODO After moving, should an animation be played? Look, blink, something else?
  DelegateIfInControl(turnAction, &BehaviorSearchWithinBoundingBox::TransitionToWait);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchWithinBoundingBox::TransitionToWait() {
  DEBUG_SET_STATE(Wait);
  auto* waitAction =  new WaitAction(_iConfig.waitBeforeTurning);
  CancelDelegates(false);
  DelegateIfInControl(waitAction, &BehaviorSearchWithinBoundingBox::TransitionToMoveToOriginalPose);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSearchWithinBoundingBox::TransitionToMoveToOriginalPose()
{
  DEBUG_SET_STATE(MoveToOriginalPose);
  _dVars.numberOfRemainingSearches--;

  // move back to the original position
  auto* turnAction =  new PanAndTiltAction(_dVars.initialTheta,
                                           _dVars.initialTilt,
                                           true,
                                           true);

  CancelDelegates(false);
  DelegateIfInControl(turnAction, &BehaviorSearchWithinBoundingBox::TransitionToGenerateNewPose);

}

} // namespace Cozmo
} // namespace Anki
