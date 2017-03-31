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

#include "anki/cozmo/basestation/behaviors/behaviorFindFaces.h"

#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "util/console/consoleInterface.h"
#include "anki/cozmo/basestation/actions/basicActions.h"

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
BehaviorFindFaces::BehaviorFindFaces(Robot& robot, const Json::Value& config)
  : BaseClass(robot, config)
{
  SetDefaultName("FindFaces");

  JsonTools::GetValueOptional(config, "maxFaceAgeToLook_ms", _maxFaceAgeToLook_ms);

  if( _configParams.behavior_RecentLocationsMax != 0 ) {
    PRINT_NAMED_WARNING("BehaviorFindFaces.Config.InvalidRecentLocationsMax",
                        "Config specified a maximum recent locations of %d, but FindFaces doesn't support recent locations. Clearing",
                        _configParams.behavior_RecentLocationsMax);
    _configParams.behavior_RecentLocationsMax = 0;
  }
  
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFindFaces::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  // we can always search for faces (override base class restrictions)
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::BeginStateMachine(Robot& robot)
{
  TransitionToLookUp(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::TransitionToLookUp(Robot& robot)
{
  DEBUG_SET_STATE(FindFacesLookUp);

  // use the base class's helper function to create a random head motion
  IAction* moveHeadAction = BaseClass::CreateHeadTurnAction(robot,
                                                            kHeadUpBodyAngleRelativeMin_deg,
                                                            kHeadUpBodyAngleRelativeMax_deg,
                                                            robot.GetPose().GetRotationAngle<'Z'>().getDegrees(),
                                                            kHeadUpHeadAngleMin_deg,
                                                            kHeadUpHeadAngleMax_deg,
                                                            kHeadUpBodyTurnSpeed_degPerSec,
                                                            kHeadUpHeadTurnSpeed_degPerSec);

  StartActing(moveHeadAction, [this](Robot& robot) {
      const TimeStamp_t latestTimestamp = robot.GetLastImageTimeStamp();
      // check if we should turn towards the last face, even if it's not in the current origin
      const bool kMustBeInCurrentOrigin = false;
      Pose3d waste;
      TimeStamp_t lastFaceTime = robot.GetFaceWorld().GetLastObservedFace(waste, kMustBeInCurrentOrigin);
      const bool useAnyFace = _maxFaceAgeToLook_ms == 0;
      if( lastFaceTime > 0 &&
          ( useAnyFace || latestTimestamp <= lastFaceTime + _maxFaceAgeToLook_ms ) ) {
        TransitionToLookAtLastFace(robot);
      }
      else {
        TransitionToBaseClass(robot);
      }
    });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::TransitionToLookAtLastFace(Robot& robot)
{
  DEBUG_SET_STATE(FindFacesLookAtLast);

  StartActing(new TurnTowardsLastFacePoseAction(robot), &BehaviorFindFaces::TransitionToBaseClass);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::TransitionToBaseClass(Robot& robot)
{
  PRINT_CH_INFO("Behaviors", (GetName() + ".TransitionToBaseClass").c_str(),
                "transitioning to base class, setting initial body direction");

  SetInitialBodyDirection( robot.GetPose().GetRotationAngle<'Z'>() );
  BaseClass::BeginStateMachine(robot);
}


  
} // namespace Cozmo
} // namespace Anki
