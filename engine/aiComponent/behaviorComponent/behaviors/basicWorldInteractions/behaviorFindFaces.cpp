/**
 * File: behaviorFindFaces.h
 *
 * Author: Brad Neuman
 * Created: 2016-08-31
 *
 * Description: Originally written by Lee, rewritten by Brad to be based on the "look in place" behavior
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorFindFaces.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/faceWorld.h"
#include "coretech/common/engine/jsonTools.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "util/console/consoleInterface.h"
#include "engine/actions/basicActions.h"

namespace Anki {
namespace Cozmo {

namespace {
CONSOLE_VAR_RANGED(f32, kHeadUpBodyAngleRelativeMin_deg, "Behavior.FindFaces", 1.0f, 0.0f, 180.0f); 
CONSOLE_VAR_RANGED(f32, kHeadUpBodyAngleRelativeMax_deg, "Behavior.FindFaces", 6.0f, 0.0f, 180.0f); 
CONSOLE_VAR_RANGED(f32, kHeadUpHeadAngleMin_deg, "Behavior.FindFaces", 20.0f, 0.0f, MAX_HEAD_ANGLE);
CONSOLE_VAR_RANGED(f32, kHeadUpHeadAngleMax_deg, "Behavior.FindFaces", 40.0f, 0.0f, MAX_HEAD_ANGLE);
CONSOLE_VAR(f32, kHeadUpBodyTurnSpeed_degPerSec, "Behavior.FindFaces", 150.0f);
CONSOLE_VAR(f32, kHeadUpHeadTurnSpeed_degPerSec, "Behavior.FindFaces", 90.0f);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindFaces::BehaviorFindFaces(const Json::Value& config)
: BaseClass(config)
{

  JsonTools::GetValueOptional(config, "maxFaceAgeToLook_ms", _maxFaceAgeToLook_ms);

  if( _configParams.behavior_RecentLocationsMax != 0 ) {
    PRINT_NAMED_WARNING("BehaviorFindFaces.Config.InvalidRecentLocationsMax",
                        "Config specified a maximum recent locations of %d, but FindFaces doesn't support recent locations. Clearing",
                        _configParams.behavior_RecentLocationsMax);
    _configParams.behavior_RecentLocationsMax = 0;
  }
  
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFindFaces::WantsToBeActivatedBehavior() const
{
  // we can always search for faces (override base class restrictions)
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::BeginStateMachine()
{
  TransitionToLookUp();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::TransitionToLookUp()
{
  DEBUG_SET_STATE(FindFacesLookUp);
  
  const auto& robotInfo = GetBEI().GetRobotInfo();

  // use the base class's helper function to create a random head motion
  IAction* moveHeadAction = BaseClass::CreateHeadTurnAction(kHeadUpBodyAngleRelativeMin_deg,
                                                            kHeadUpBodyAngleRelativeMax_deg,
                                                            robotInfo.GetPose().GetRotationAngle<'Z'>().getDegrees(),
                                                            kHeadUpHeadAngleMin_deg,
                                                            kHeadUpHeadAngleMax_deg,
                                                            kHeadUpBodyTurnSpeed_degPerSec,
                                                            kHeadUpHeadTurnSpeed_degPerSec);

  DelegateIfInControl(moveHeadAction, [this]() {
      const TimeStamp_t latestTimestamp = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
      // check if we should turn towards the last face, even if it's not in the current origin
      const bool kMustBeInCurrentOrigin = false;
      Pose3d waste;
      TimeStamp_t lastFaceTime = GetBEI().GetFaceWorld().GetLastObservedFace(waste, kMustBeInCurrentOrigin);
      const bool useAnyFace = _maxFaceAgeToLook_ms == 0;
      if( lastFaceTime > 0 &&
          ( useAnyFace || latestTimestamp <= lastFaceTime + _maxFaceAgeToLook_ms ) ) {
        TransitionToLookAtLastFace();
      }
      else {
        TransitionToBaseClass();
      }
    });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::TransitionToLookAtLastFace()
{
  DEBUG_SET_STATE(FindFacesLookAtLast);
  DelegateIfInControl(new TurnTowardsLastFacePoseAction(), &BehaviorFindFaces::TransitionToBaseClass);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::TransitionToBaseClass()
{
  PRINT_CH_INFO("Behaviors", "BehaviorFindFaces.TransitionToBaseClass",
                " %s is transitioning to base class, setting initial body direction",
                GetIDStr().c_str());

  const Pose3d& robotPose = GetBEI().GetRobotInfo().GetPose();
  SetInitialBodyDirection( robotPose.GetRotationAngle<'Z'>() );
  BaseClass::BeginStateMachine();
}


  
} // namespace Cozmo
} // namespace Anki
