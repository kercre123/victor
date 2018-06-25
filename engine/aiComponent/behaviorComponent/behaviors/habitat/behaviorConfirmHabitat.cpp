/**
 * File: behaviorConfirmHabitat.cpp
 *
 * Author: Arjun Menon
 * Date:   06 04 2018
 *
 * Description:
 * behavior that represents the robot's deliberate maneuvers to confirm whether
 *  it is inside the habitat after a pickup and putdown, or driving off charger
 *
 * the robot will engage in discrete actions to get to a configuration on top
 * of the white line so that its front cliff sensors are triggered. At this
 * position, the behavior will take no more actions since we are in the pose
 * to confirm the habitat in.
 *
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/habitat/behaviorConfirmHabitat.h"

#include "clad/types/habitatDetectionTypes.h"
#include "clad/types/behaviorComponent/behaviorIDs.h"
#include "clad/externalInterface/messageEngineToGameTag.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/animActions.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/cozmoContext.h"
#include "engine/components/habitatDetectorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/components/pathComponent.h"
#include "engine/navMap/mapComponent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "engine/viz/vizManager.h"

#include "util/console/consoleInterface.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/vision/engine/observableObject.h"
#include "coretech/common/engine/utils/timer.h"

#include <memory>

namespace Anki {
namespace Cozmo {

namespace
{
}

BehaviorConfirmHabitat::DynamicVariables::DynamicVariables()
{
  _phase = ConfirmHabitatPhase::Initial;
  _randomWalkTooCloseObstacle = false;
  _lookForChargerAngleSwept_rad = 0.0;
}

BehaviorConfirmHabitat::BehaviorConfirmHabitat(const Json::Value& config)
: Anki::Cozmo::ICozmoBehavior(config)
{
  SubscribeToTags({{
    EngineToGameTag::RobotStopped,
    EngineToGameTag::CliffEvent,
    EngineToGameTag::RobotOffTreadsStateChanged,
    EngineToGameTag::UnexpectedMovement
  }});
  
  BlockWorldFilter* chargerFilter = new BlockWorldFilter();
  chargerFilter->AddAllowedFamily(ObjectFamily::Charger);
  chargerFilter->AddAllowedType(ObjectType::Charger_Basic);
  _chargerFilter.reset(chargerFilter);
}

BehaviorConfirmHabitat::~BehaviorConfirmHabitat()
{
}

void BehaviorConfirmHabitat::InitBehavior()
{
  _dVars = DynamicVariables();
}

void BehaviorConfirmHabitat::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
}

bool BehaviorConfirmHabitat::WantsToBeActivatedBehavior() const
{
  const bool onCharger = GetBEI().GetRobotInfo().IsOnChargerPlatform();
  
  const auto& habitatDetector = GetBEI().GetHabitatDetectorComponent();
  HabitatBeliefState hbelief   = habitatDetector.GetHabitatBeliefState();
  
  return !onCharger &&
         hbelief == HabitatBeliefState::Unknown;
}

void BehaviorConfirmHabitat::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}

void BehaviorConfirmHabitat::OnBehaviorActivated()
{
  PRINT_NAMED_INFO("ConfirmHabitat.Activated","");
  GetBEI().GetCliffSensorComponent().EnableStopOnWhite(true);
  _dVars = DynamicVariables();
}

void BehaviorConfirmHabitat::OnBehaviorDeactivated()
{
  PRINT_NAMED_INFO("ConfirmHabitat.Deactivated","");
  GetBEI().GetCliffSensorComponent().EnableStopOnWhite(false);
  _dVars = DynamicVariables();
}

void BehaviorConfirmHabitat::BehaviorUpdate()
{
  if(!IsActivated()) {
    return;
  }
  
  if(IsControlDelegated()) {
    return;
  }
  
  const auto& habitatDetector = GetBEI().GetHabitatDetectorComponent();
  std::unique_ptr<IActionRunner> actionToExecute;
  RobotCompletedActionCallback actionCallback = nullptr;
  
  // play the reaction to discovering we are in the habitat, if we are in it
  if(habitatDetector.GetHabitatBeliefState() == HabitatBeliefState::InHabitat) {
    TransitionToReactToHabitat();
    return;
  } else if(habitatDetector.GetHabitatBeliefState() == HabitatBeliefState::NotInHabitat) {
    CancelSelf();
    return;
  }
  
  switch(_dVars._phase) {
    case ConfirmHabitatPhase::Initial:
    {
      const bool isChargerObserved = GetChargerIfObserved() != nullptr;
      if(isChargerObserved) {
        _dVars._phase = ConfirmHabitatPhase::SeekLineFromCharger;
        TransitionToSeekLineFromCharger();
      } else {
        _dVars._phase = ConfirmHabitatPhase::LocalizeCharger;
        TransitionToLocalizeCharger();
      }
      break;
    }
    case ConfirmHabitatPhase::LocalizeCharger:
    {
      // check if we've seen the charger yet, or exceeded search turn angle
      const bool chargerSeen = GetChargerIfObserved() != nullptr;
      if(chargerSeen) {
        PRINT_NAMED_INFO("ConfirmHabitat.BehaviorUpdate.LocalizeCharger.FoundCharger", "");
        _dVars._phase = ConfirmHabitatPhase::SeekLineFromCharger;
        TransitionToSeekLineFromCharger();
      } else if(abs(_dVars._lookForChargerAngleSwept_rad) > (M_PI*2)) {
        PRINT_NAMED_INFO("ConfirmHabitat.BehaviorUpdate.LocalizeCharger.ExceededAngle", "");
        _dVars._phase = ConfirmHabitatPhase::RandomWalk;
        TransitionToRandomWalk();
      } else {
        TransitionToLocalizeCharger();
      }
      break;
    }
    case ConfirmHabitatPhase::SeekLineFromCharger:
    {
      // only try this state once, when invoked
      // if we aren't LineDocking, then RandomWalk
      // note: the action-completion callback will
      //        perform the transition check
      if(habitatDetector.GetHabitatLineRelPose() != HabitatLineRelPose::Invalid &&
         habitatDetector.GetHabitatLineRelPose() != HabitatLineRelPose::AllGrey) {
        _dVars._phase = ConfirmHabitatPhase::LineDocking;
      } else {
        PRINT_NAMED_INFO("ConfirmHabitat.BehaviorUpdate.SeekLine.NotStoppedOnWhite","");
        _dVars._phase = ConfirmHabitatPhase::RandomWalk;
        TransitionToRandomWalk();
      }
      break;
    }
    case ConfirmHabitatPhase::RandomWalk:
    {
      if(_dVars._randomWalkTooCloseObstacle) {
        // respond to unexpected movement
        TransitionToBackupAndTurn(M_PI_4_F);
        _dVars._randomWalkTooCloseObstacle = false;
      } else if(habitatDetector.GetHabitatLineRelPose() == HabitatLineRelPose::Invalid ||
                habitatDetector.GetHabitatLineRelPose() == HabitatLineRelPose::AllGrey) {
        TransitionToRandomWalk();
      } else {
        _dVars._phase = ConfirmHabitatPhase::LineDocking;
      }
      break;
    }
    case ConfirmHabitatPhase::LineDocking:
    {
      switch(habitatDetector.GetHabitatLineRelPose()) {
        case HabitatLineRelPose::AllGrey:
        {
          PRINT_NAMED_INFO("ConfirmHabitat.BehaviorUpdate.LineDocking.LineLost","");
          _dVars._phase = ConfirmHabitatPhase::RandomWalk;
          TransitionToRandomWalk();
          break;
        }
        case HabitatLineRelPose::WhiteFL: // deliberate fallthrough
        case HabitatLineRelPose::WhiteFR: // deliberate fallthrough
        case HabitatLineRelPose::WhiteBL: // deliberate fallthrough
        case HabitatLineRelPose::WhiteBR:
        {
          TransitionToCliffAlignWhite();
          break;
        }
          
        case HabitatLineRelPose::WhiteFLFR:
        {
          // desired state: do nothing
          break;
        }
        case HabitatLineRelPose::WhiteFRBR:
        {
          TransitionToTurnBackupForward(-M_PI_2_F, -10, 20);
          break;
        }
        case HabitatLineRelPose::WhiteFLBL:
        {
          TransitionToTurnBackupForward(M_PI_2_F, -10, 20);
          break;
        }
        case HabitatLineRelPose::WhiteBLBR:
        {
          TransitionToTurnBackupForward(M_PI_F, -10, 20);
          break;
        }
        case HabitatLineRelPose::Invalid:
        {
          break;
        }
      } // end switch
      break;
    }
  }
}

void BehaviorConfirmHabitat::DelegateActionHelper(IActionRunner* action, RobotCompletedActionCallback&& callback)
{
  if(action != nullptr) {
    PathMotionProfile slowProfile;
    slowProfile.speed_mmps = 40;
    slowProfile.pointTurnSpeed_rad_per_sec = 1.0;
    GetBEI().GetRobotInfo().GetPathComponent().SetCustomMotionProfileForAction(slowProfile, action);
    
    // add an action callback to print the result if we didn't assign one
    if(callback == nullptr) {
      callback = [](const ExternalInterface::RobotCompletedAction& msg)->void {
        switch(msg.result) {
          case ActionResult::SUCCESS: { break; }
          default:
          {
            PRINT_NAMED_INFO("ConfirmHabitat.DelegateActionHelper.UnhandledActionResult","%s",EnumToString(msg.result));
          }
        }
      };
    }
    DelegateNow(action, callback);
  } else {
    PRINT_NAMED_WARNING("ConfirmHabitat.BehaviorUpdate.NoActionChosen","");
  }
}

void BehaviorConfirmHabitat::TransitionToReactToHabitat()
{
  PRINT_NAMED_INFO("ConfirmHabitat.TransitionToReactToHabitat","");
  TriggerAnimationAction* action = new TriggerAnimationAction(AnimationTrigger::ReactToHabitat, 1, true);
  RobotCompletedActionCallback callback = [this](const ExternalInterface::RobotCompletedAction& msg)->void {
    this->CancelSelf();
  };
  DelegateActionHelper(action, std::move(callback));
}

void BehaviorConfirmHabitat::TransitionToLocalizeCharger()
{
  PRINT_NAMED_INFO("ConfirmHabitat.TransitionToLocalizeCharger","");
  
  IActionRunner* action = new CompoundActionSequential(std::list<IActionRunner*>{
    new PanAndTiltAction(0, DEG_TO_RAD(5), false, true),    // viewing angle for seeing the charger
    new TurnInPlaceAction(M_PI_4,false),                    // sweep some degrees to look
    new WaitForLambdaAction([this](Robot& robot)->bool {        // wait to observe charger
      return GetChargerIfObserved() != nullptr;
    }, 0.5f) // max time to wait for a charger marker
  });
  
  // callback checks:
  // + if the charger was SEEN, then we can drive to a position based on the charger's pose
  // + if the charger was NOT seen, then we will keep looking, until we've swept 360 degrees
  // + if we have not seen the charger, and we swept 360 degrees => RandomWalk
  RobotCompletedActionCallback callback = [this](const ExternalInterface::RobotCompletedAction& msg)->void {
    if(msg.result == ActionResult::SUCCESS || msg.result == ActionResult::TIMEOUT) {
      _dVars._lookForChargerAngleSwept_rad += M_PI_4;
    } else {
      PRINT_NAMED_WARNING("ConfirmHabitat.TransitionToLocalizeCharger.FailedTurnAndCheck","");
    }
  };
  
  DelegateActionHelper(action, std::move(callback));
}

void BehaviorConfirmHabitat::TransitionToCliffAlignWhite()
{
  PRINT_NAMED_INFO("ConfirmHabitat.TransitionToCliffAlignWhite","");
  IActionRunner* action = new CliffAlignToWhiteAction();
  RobotCompletedActionCallback callback = [this](const ExternalInterface::RobotCompletedAction& msg)->void {
    switch(msg.result) {
      case ActionResult::SUCCESS: { break; }
      case ActionResult::CLIFF_ALIGN_FAILED:
      {
        PRINT_NAMED_INFO("ConfirmHabitat.TransitionToCliffAlignWhite.Failed","");
        
        const auto& habitatDetector = GetBEI().GetHabitatDetectorComponent();
        auto linePose = habitatDetector.GetHabitatLineRelPose();
        switch(linePose) {
          case HabitatLineRelPose::AllGrey:
          {
            TransitionToTurnBackupForward(0.0f, -20, 30);
            break;
          }
          case HabitatLineRelPose::WhiteFL:
          case HabitatLineRelPose::WhiteFR:
          case HabitatLineRelPose::WhiteBL:
          case HabitatLineRelPose::WhiteBR:
          {
            TransitionToBackupAndTurn(0); // no turn requested here
            break;
          }
          default:
          {
            PRINT_NAMED_WARNING("ConfirmHabitat.TransitionToCliffAlignWhite.UnhandledLinePose",
                                  "%s", EnumToString(linePose));
            break;
          }
        }
        break;
      }
      default:
      {
        PRINT_NAMED_INFO("ConfirmHabitat.TransitionToCliffAlignWhite.UnhandledActionResult",
                         "%s",EnumToString(msg.result));
      }
    }
  };
  DelegateActionHelper(action, std::move(callback));
}

void BehaviorConfirmHabitat::TransitionToBackupAndTurn(f32 angle)
{
  PRINT_NAMED_INFO("ConfirmHabitat.TransitionToBackupAndTurn","");
  IActionRunner* action = new CompoundActionSequential(std::list<IActionRunner*>{
    // note: temporarily disable stop on white
    // this allows the robot to get out of tight spots
    // by allowing it to drive a little over the white
    new WaitForLambdaAction([](Robot& robot)->bool {
      robot.GetComponentPtr<CliffSensorComponent>()->EnableStopOnWhite(false);
      return true;
    },0.1f),
    new DriveStraightAction(-20,  75),
    new WaitForLambdaAction([](Robot& robot)->bool {
      robot.GetComponentPtr<CliffSensorComponent>()->EnableStopOnWhite(true);
      return true;
    },0.1f),
    new TurnInPlaceAction(angle, false),
  });
  DelegateActionHelper(action, nullptr);
}

void BehaviorConfirmHabitat::TransitionToTurnBackupForward(f32 angle, int backDist_mm, int forwardDist_mm)
{
  PRINT_NAMED_INFO("ConfirmHabitat.TransitionToTurnBackupForward","");
  CompoundActionSequential* action = new CompoundActionSequential(std::list<IActionRunner*>{
    // note: temporarily disable stop on white
    // this allows the robot to get out of tight spots
    // by allowing it to drive a little over the white
    new WaitForLambdaAction([](Robot& robot)->bool {
      robot.GetComponentPtr<CliffSensorComponent>()->EnableStopOnWhite(false);
      return true;
    })
  });
  
  if(!FLT_NEAR(angle, 0.0f)) {
    action->AddAction(new TurnInPlaceAction(angle, false));
  }
  
  if(backDist_mm != 0) {
    action->AddAction(new DriveStraightAction(backDist_mm, 100));
  }
  
  action->AddAction(new WaitForLambdaAction([](Robot& robot)->bool {
      robot.GetComponentPtr<CliffSensorComponent>()->EnableStopOnWhite(true);
      return true;
    }));
  
  if(forwardDist_mm != 0) {
    action->AddAction(new DriveStraightAction(forwardDist_mm,  50));
  }
  DelegateActionHelper(action, nullptr);
}

void BehaviorConfirmHabitat::TransitionToRandomWalk()
{
  static int count = 1;
  Pose3d robotPose = GetBEI().GetRobotInfo().GetPose();
  Pose3d desiredOffsetPose;
  if(count == 0 || count >= 3) {
    desiredOffsetPose = Pose3d(Rotation3d(0, Z_AXIS_3D()), Vec3f(100,0,0));
    count = 0;
  } else if(count == 1) {
    desiredOffsetPose = Pose3d(Rotation3d(-M_PI_4, Z_AXIS_3D()), Vec3f(75,-50,0));
  } else if(count == 2) {
    desiredOffsetPose = Pose3d(Rotation3d(M_PI_4, Z_AXIS_3D()), Vec3f(75,50,0));
  }
  desiredOffsetPose.SetParent(robotPose);
  GetBEI().GetMapComponent().SetUseProxObstaclesInPlanning(false);
  IActionRunner* action = new DriveToPoseAction(desiredOffsetPose, false);
  RobotCompletedActionCallback callback = [this](const ExternalInterface::RobotCompletedAction& msg)->void{
    GetBEI().GetMapComponent().SetUseProxObstaclesInPlanning(true);
  };
  DelegateActionHelper(action, std::move(callback));
  PRINT_NAMED_INFO("ConfirmHabitat.TransitionToRandomWalk", "ActionNum=%d",count);
  count++;
}

void BehaviorConfirmHabitat::TransitionToSeekLineFromCharger()
{
  // choose a candidate pose to drive towards the charger
  // + if we get RobotStopped with the reason=StopOnWhite => LineDocking
  const ObservableObject* charger = GetChargerIfObserved();
  if(!ANKI_VERIFY(charger != nullptr,"ConfirmHabitat.TransitionToSeekLineFromCharger.ChargerIsNull","")) {
    _dVars._phase = ConfirmHabitatPhase::RandomWalk;
    return;
  }
  
  // the origin of the charger located at the tip of the ramp
  // with the x-axis oriented towards the label
  const Pose3d& chargerPose = charger->GetPose();
  PRINT_NAMED_INFO("ConfirmHabitat.SeekLineFromCharger","");
  
  // these poses are computed relative to the charger
  // the values selected define a position slightly beyond
  // the white line of the habitat, assuming the charger
  // is correctly installed in place
  static const std::vector<Pose3d> offsetList = {
    Pose3d(   M_PI_4_F, Z_AXIS_3D(), Vec3f(  53,  159, 0)),
    Pose3d(   M_PI_4_F, Z_AXIS_3D(), Vec3f(   0,  212, 0)),

    Pose3d(  -M_PI_4_F, Z_AXIS_3D(), Vec3f(  53, -159, 0)),
    Pose3d(  -M_PI_4_F, Z_AXIS_3D(), Vec3f(   0, -212, 0)),

    Pose3d( 3*M_PI_4_F, Z_AXIS_3D(), Vec3f(-141,  212, 0)),
    Pose3d( 3*M_PI_4_F, Z_AXIS_3D(), Vec3f(-194,  159, 0)),
    Pose3d( 3*M_PI_4_F, Z_AXIS_3D(), Vec3f(-247,  106, 0)),

    Pose3d(-3*M_PI_4_F, Z_AXIS_3D(), Vec3f(-141, -212, 0)),
    Pose3d(-3*M_PI_4_F, Z_AXIS_3D(), Vec3f(-194, -159, 0)),
    Pose3d(-3*M_PI_4_F, Z_AXIS_3D(), Vec3f(-247, -106, 0)),

    Pose3d(     M_PI_F, Z_AXIS_3D(), Vec3f(-325,    0, 0)),
  };
  
  int index = GetRNG().RandInt((int)offsetList.size());
  Pose3d desiredOffsetPose = offsetList[index];
  desiredOffsetPose.SetParent(chargerPose);
  IActionRunner* action = new DriveToPoseAction(desiredOffsetPose, false);
  DelegateActionHelper(action, nullptr);
  
}

const ObservableObject* BehaviorConfirmHabitat::GetChargerIfObserved() const
{
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  return GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_chargerFilter);
}

void BehaviorConfirmHabitat::TransitionToWaitForWhite()
{
  PRINT_NAMED_INFO("ConfirmHabitat.TransitionToWaitForWhite","");
  IActionRunner* action = new WaitForLambdaAction([](Robot& robot)->bool {
    return robot.GetHabitatDetectorComponent().GetHabitatLineRelPose() != HabitatLineRelPose::Invalid &&
           robot.GetHabitatDetectorComponent().GetHabitatLineRelPose() != HabitatLineRelPose::AllGrey;
  },0.2f);
  DelegateActionHelper(action, nullptr);
}

void BehaviorConfirmHabitat::HandleWhileActivated(const EngineToGameEvent& event)
{
  using namespace ExternalInterface;
  switch (event.GetData().GetTag())
  {
    case EngineToGameTag::RobotStopped:
    {
      auto reason = event.GetData().Get_RobotStopped().reason;
      if(reason == StopReason::WHITE) {
        const auto& habitatDetector = GetBEI().GetHabitatDetectorComponent();
        PRINT_NAMED_INFO("ConfirmHabitat.HandleWhileActivated.StopOnWhite",
                         "%s",EnumToString(habitatDetector.GetHabitatLineRelPose()));
        CancelDelegates();
        TransitionToWaitForWhite();
      }
      break;
    }
    case EngineToGameTag::CliffEvent:
    {
      // deliberate fallthrough
    }
    case EngineToGameTag::RobotOffTreadsStateChanged:
    {
      CancelDelegates();
      CancelSelf();
      break;
    }
    case EngineToGameTag::UnexpectedMovement:
    {
      PRINT_NAMED_INFO("ConfirmHabitat.HandleWhiteActivated.UnexpectedMovement","");
      if(_dVars._phase == ConfirmHabitatPhase::RandomWalk && IsControlDelegated() ) {
        CancelDelegates();
        // we ran into an obstacle:
        // - confirm with the prox sensor
        // - backup, turn in a direction
        // - if there is no prox obstacle within the minimum gap => continue
        // - else turn some more, and stop
        if(GetBEI().GetHabitatDetectorComponent().IsProxObservingHabitatWall()) {
          _dVars._randomWalkTooCloseObstacle = true;
        }
      }
      break;
    }
    default:
    {
      // nothing
      break;
    }
  } // end switch
}

}
}
