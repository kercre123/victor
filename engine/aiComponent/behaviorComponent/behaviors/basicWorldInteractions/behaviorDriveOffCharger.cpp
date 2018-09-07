/**
 * File: behaviorDriveOffCharger.cpp
 *
 * Author: Molly Jameson (or talk to ross)
 * Created: 2016-05-19
 *
 * Description: Behavior to drive to off a charger and deal with being on/off treads while on the charger.
 *              Supply a prioritized list of options to choose the direction based on the most recent face,
 *              cube, or mic direction, or simply drive randomly or straight. If a supplied direction
 *              is not applicable (e.g., no faces) or is unsuccessful, the next one is attempted.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorDriveOffCharger.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/charger.h" // for charger length
#include "engine/components/mics/micComponent.h"
#include "engine/components/mics/micDirectionHistory.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/faceWorld.h"
#include "engine/moodSystem/moodManager.h"

#include "coretech/common/engine/utils/timer.h"


namespace Anki {
namespace Vector {

namespace{
// array of directions in {face, cube, straight, left, right, randomly, micDirection, proceduralStraight}
static const char* const kDriveDirectionCuesKey = "driveDirectionCues";
static const char* const kExtraDriveDistKey = "extraDistanceToDrive_mm";
static const char* const kMaxFaceAgeKey = "maxFaceAge_s";
static const char* const kMaxCubeAgeKey = "maxCubeAge_s";
  
const float kDefaultExtraDist_mm = 60.0f;
  
const float kExtraProceduralTimeout_s = 2.5f; // buffer to account for accel/decel and bumps
  
const AnimationTrigger kAnimFarLeft = AnimationTrigger::DriveOffChargerFarLeft;
const AnimationTrigger kAnimFarRight = AnimationTrigger::DriveOffChargerFarRight;
const AnimationTrigger kAnimLeft = AnimationTrigger::DriveOffChargerLeft;
const AnimationTrigger kAnimRight = AnimationTrigger::DriveOffChargerRight;
const AnimationTrigger kAnimStraight = AnimationTrigger::DriveOffChargerStraight;
const BehaviorID kRandomDriveBehavior = BEHAVIOR_ID(DriveOffChargerRandomlyAnim);
const int kMaxAttempts = 10;
const float kFaceAngleForTurn_rad = DEG_TO_RAD(60.0f);
}
  
#define SET_STATE(s) do{ \
                          _dVars.state = State::s; \
                          PRINT_NAMED_INFO("BehaviorDriveOffCharger.State", "State = %s", #s); \
                        } while(0);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveOffCharger::InstanceConfig::InstanceConfig()
{
  proceduralDistToDrive_mm = 0.0f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveOffCharger::DynamicVariables::DynamicVariables()
{
  state = State::NotStarted;
  directionIndex = 0;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveOffCharger::BehaviorDriveOffCharger(const Json::Value& config)
: ICozmoBehavior(config)
{
  float extraDist_mm = config.get( kExtraDriveDistKey, kDefaultExtraDist_mm ).asFloat();
  _iConfig.proceduralDistToDrive_mm = Charger::GetLength() + extraDist_mm;
  
  if( ANKI_VERIFY( config[kDriveDirectionCuesKey].isArray() && (config[kDriveDirectionCuesKey].size() > 0),
                   "BehaviorDriveOffCharger.Ctor.InvalidDirections",
                   "Direction list is not a valid list") )
  {
    for( const auto& directionStr : config[kDriveDirectionCuesKey] ) {
      if( ANKI_VERIFY( directionStr.isString(), "BehaviorDriveOffCharger.Ctor.InvalidDirStr", "" ) ) {
        _iConfig.driveDirections.push_back( ParseDriveDirection( directionStr.asString() ) );
      }
    }
  }
  
  _iConfig.maxFaceAge_s = config.get( kMaxFaceAgeKey, 10*60 ).asInt();
  _iConfig.maxCubeAge_s = config.get( kMaxCubeAgeKey, 10*60 ).asInt();
  
  PRINT_CH_DEBUG("Behaviors", "BehaviorDriveOffCharger.DriveDist",
                 "Driving %fmm off the charger (%f length + %f extra)",
                 _iConfig.proceduralDistToDrive_mm,
                 Charger::GetLength(),
                 extraDist_mm);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig.driveRandomlyBehavior.get() );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kExtraDriveDistKey,
    kDriveDirectionCuesKey,
    kMaxFaceAgeKey,
    kMaxCubeAgeKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.driveRandomlyBehavior = BC.FindBehaviorByID( kRandomDriveBehavior );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveOffCharger::WantsToBeActivatedBehavior() const
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  // assumes it's not possible to be OnCharger without being OnChargerPlatform
  DEV_ASSERT(robotInfo.IsOnChargerPlatform() || !robotInfo.IsOnChargerContacts(),
             "BehaviorDriveOffCharger.WantsToBeActivatedBehavior.InconsistentChargerFlags");

  // can run any time we are on the charger platform
  
  // bn: this used to be "on charger platform", but that caused weird issues when the robot thought it drove
  // over the platform later due to mis-localization. Then this changed to "on charger" to avoid those issues,
  // but that caused other issues (if the robot was bumped during wakeup, it wouldn't drive off the
  // charger). Now, we've gone back to OnChargerPlatform but fixed it to work better
  const bool onChargerPlatform = robotInfo.IsOnChargerPlatform();
  return onChargerPlatform;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  
  auto& robotInfo = GetBEI().GetRobotInfo();
  robotInfo.GetDrivingAnimationHandler().PushDrivingAnimations(
          {AnimationTrigger::DriveStartLaunch,
          AnimationTrigger::DriveLoopLaunch,
          AnimationTrigger::DriveEndLaunch},
          GetDebugLabel());

  const bool onTreads = GetBEI().GetOffTreadsState() == OffTreadsState::OnTreads;
  if( onTreads ) {
    SelectAndDrive();
  }
  else {
    // otherwise we'll just wait in Update until we are on the treads (or removed from the charger)
    SET_STATE(WaitForOnTreads);
  }
  
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::OnBehaviorDeactivated()
{
  auto& robotInfo = GetBEI().GetRobotInfo();
  robotInfo.GetDrivingAnimationHandler().RemoveDrivingAnimations(GetDebugLabel());
}
    
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  const auto& robotInfo = GetBEI().GetRobotInfo();
  if( robotInfo.IsOnChargerPlatform() ) {
    const bool onTreads = GetBEI().GetOffTreadsState() == OffTreadsState::OnTreads;
    if( !onTreads ) {
      // if we aren't on the treads anymore, but we are on the charger, then the user must be holding us
      // inside the charger.... so just sit in this behavior doing nothing until we get put back down
      CancelDelegates(false);
      SET_STATE(WaitForOnTreads);
      // TODO:(bn) play an idle here?
    }
    else if( ! IsControlDelegated() ) {
      // if we finished the last action, but are still on the charger, queue another one
      SelectAndDrive();
    }
    
    return;
  }

  if( !IsControlDelegated() ) {
    // store in whiteboard our success
    const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    GetAIComp<AIWhiteboard>().GotOffChargerAtTime( curTime );
    BehaviorObjectiveAchieved(BehaviorObjective::DroveAsIntended);
    
    if(GetBEI().HasMoodManager()){
      auto& moodManager = GetBEI().GetMoodManager();
      moodManager.TriggerEmotionEvent("DriveOffCharger",
                                      MoodManager::GetCurrentTimeInSeconds());
    }
    CancelSelf();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::SelectAndDrive()
{
  if( !GetBEI().GetRobotInfo().IsOnChargerPlatform() ) {
    // update loop will end the behavior
    return;
  }
  
  SET_STATE(Driving);
  
  for( ; _dVars.directionIndex < kMaxAttempts; ++_dVars.directionIndex ) {
    if( IsControlDelegated() ) {
      // the (_dVars.directionIndex - 1)-th direction just started successfully, and then we incremented the index and got here
      break;
    }
    
    // if we're here, the (_dVars.directionIndex - 1)-th direction either finished or was not applicable (e.g., no faces)
    
    if( _dVars.directionIndex >= _iConfig.driveDirections.size() ) {
      // just keep trying straight procedural until we run out of attempts
      TransitionToDrivingStraightProcedural();
    } else {
      const auto& direction = _iConfig.driveDirections[_dVars.directionIndex];
      switch( direction ) {
        case DriveDirection::Straight:
          TransitionToDrivingAnim( kAnimStraight );
          break;
        case DriveDirection::Left:
          TransitionToDrivingAnim( kAnimLeft );
          break;
        case DriveDirection::Right:
          TransitionToDrivingAnim( kAnimRight );
          break;
        case DriveDirection::ProceduralStraight:
          TransitionToDrivingStraightProcedural();
          break;
        case DriveDirection::Randomly:
          TransitionToDrivingRandomly();
          break;
        case DriveDirection::Face:
          TransitionToDrivingFace();
          break;
        case DriveDirection::Cube:
          TransitionToDrivingCube();
          break;
        case DriveDirection::MicDirection:
          TransitionToDrivingMicDirection();
          break;
      }
    }
  }
  
  if( (_dVars.directionIndex >= kMaxAttempts) && !IsControlDelegated() ) {
    PRINT_NAMED_WARNING("BehaviorDriveOffCharger.SelectAndDrive.CouldntLeave",
                        "Behavior '%s' couldn't get off charger in %d attempts. onCharger=%d",
                        GetDebugLabel().c_str(), kMaxAttempts, GetBEI().GetRobotInfo().IsOnChargerPlatform());
    CancelSelf();
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::TransitionToDrivingStraightProcedural()
{
  PRINT_CH_INFO( "Behaviors", "BehaviorDriveOffCharger.TransitionToDrivingStraightProcedural", "Driving straight" );
  DriveStraightAction* action = new DriveStraightAction(_iConfig.proceduralDistToDrive_mm);
  action->SetCanMoveOnCharger(true);
  const float timeout_s = kExtraProceduralTimeout_s
                          + (static_cast<float>(_iConfig.proceduralDistToDrive_mm) / DEFAULT_PATH_MOTION_PROFILE.speed_mmps);
  action->SetTimeoutInSeconds( timeout_s );
  DelegateIfInControl( action );
  // the Update function will transition back to this or another direction, or end the behavior, as appropriate
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::TransitionToDrivingAnim( const AnimationTrigger& animTrigger )
{
  auto* action = new TriggerLiftSafeAnimationAction{ animTrigger };
  DelegateIfInControl( action );
  // the Update function will transition back to this or another direction, or end the behavior, as appropriate
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::TransitionToDrivingRandomly()
{
  if( ANKI_VERIFY( _iConfig.driveRandomlyBehavior->WantsToBeActivated(),
                   "BehaviorDriveOffCharger.TransitionToDrivingRandomly.DWTA",
                   "Random driving behavior doesn't want to activate" ) )
  {
    PRINT_CH_INFO( "Behaviors", "BehaviorDriveOffCharger.TransitionToDrivingRandomly", "Driving randomly" );
    DelegateIfInControl( _iConfig.driveRandomlyBehavior.get() );
    // the Update function will transition back to this or another direction, or end the behavior, as appropriate
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::TransitionToDrivingFace()
{
  Pose3d facePose;
  const RobotTimeStamp_t imageTimeStamp_ms = GetBEI().GetFaceWorld().GetLastObservedFace( facePose );
  const RobotTimeStamp_t lastImageTimeStamp_ms = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
  if( imageTimeStamp_ms > 0 && (imageTimeStamp_ms + 1000*_iConfig.maxFaceAge_s >= lastImageTimeStamp_ms) ) {
    Pose3d faceWrtRobot;
    const bool poseSuccess = facePose.GetWithRespectTo(GetBEI().GetRobotInfo().GetPose(), faceWrtRobot);
    if( poseSuccess ) {
      PRINT_CH_INFO( "Behaviors", "BehaviorDriveOffCharger.TransitionToDrivingFace", "Driving toward face" );
      DriveToPose( faceWrtRobot );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::TransitionToDrivingCube()
{

  BlockWorldFilter filter;
  filter.SetAllowedFamilies( {ObjectFamily::LightCube} );
  
  std::vector<const ObservableObject*> objects;
  const ObservableObject* obj = GetBEI().GetBlockWorld().FindMostRecentlyObservedObject( filter );
  if( obj != nullptr ) {
    const RobotTimeStamp_t cubeTimeStamp_ms = obj->GetLastObservedTime();
    const RobotTimeStamp_t lastImageTimeStamp_ms = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
    if( cubeTimeStamp_ms + 1000*_iConfig.maxCubeAge_s >= lastImageTimeStamp_ms ) {
      Pose3d cubeWrtRobot;
      const bool poseSuccess = obj->GetPose().GetWithRespectTo(GetBEI().GetRobotInfo().GetPose(), cubeWrtRobot);
      if( poseSuccess ) {
        PRINT_CH_INFO( "Behaviors", "BehaviorDriveOffCharger.TransitionToDrivingCube", "Driving toward cube" );
        DriveToPose( cubeWrtRobot );
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::TransitionToDrivingMicDirection()
{
  const MicDirectionIndex direction = GetDirectionFromMicHistory();
  // direction ranges from 0 (12 oclock) to 11 (11 oclock), and then invalid/unknown
  const bool directionKnown = (direction <= 11);
  PRINT_CH_INFO( "Behaviors",
                 "BehaviorDriveOffCharger.TransitionToDrivingMicDirection.Direction", "Direction = [%d] (%s)",
                 (int)direction, directionKnown ? "known" : "unknown" );
  if( directionKnown ) {
    AnimationTrigger directionAnim;
    if( direction == 2 ) { // 2
      directionAnim = kAnimRight;
    } else if( direction > 2 && direction <= 6 ) { // 3-6 (tie breaker: 6 oclock drives right)
      directionAnim = kAnimFarRight;
    } else if( direction > 6 && direction <= 9 ) { // 7-9
      directionAnim = kAnimFarLeft;
    } else if( direction == 10 ) { // 10
      directionAnim = kAnimLeft;
    } else { // 11, 12, 1
      directionAnim = kAnimStraight;
    }
    auto* action = new TriggerLiftSafeAnimationAction{ directionAnim };
    DelegateIfInControl( action );
    // the Update function will transition back to this or another direction, or end the behavior, as appropriate
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MicDirectionIndex BehaviorDriveOffCharger::GetDirectionFromMicHistory() const
{
  const MicDirectionHistory& history = GetBEI().GetMicComponent().GetMicDirectionHistory();
  const auto& recentDirection = history.GetRecentDirection();
  return recentDirection;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::DriveToPose( const Pose3d& pose )
{
  Vec2f groundTranslation{ pose.GetTranslation() };
  const float angleToFace_rad = atan2f( groundTranslation.y(), groundTranslation.x() );
  AnimationTrigger directionAnim;
  if( angleToFace_rad < -kFaceAngleForTurn_rad ) {
    directionAnim = kAnimRight;
  } else if( angleToFace_rad > kFaceAngleForTurn_rad ) {
    directionAnim = kAnimLeft;
  } else {
    directionAnim = kAnimStraight;
  }
  auto* action = new TriggerLiftSafeAnimationAction{ directionAnim };
  DelegateIfInControl( action );
  // the Update function will transition back to this or another direction, or end the behavior, as appropriate
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveOffCharger::DriveDirection BehaviorDriveOffCharger::ParseDriveDirection( const std::string& driveDirectionStr ) const
{
  if( driveDirectionStr == "straight" ) {
    return DriveDirection::Straight;
  } else if( driveDirectionStr == "proceduralStraight" ) {
    return DriveDirection::ProceduralStraight;
  } else if( driveDirectionStr == "randomly" ) {
    return DriveDirection::Randomly;
  } else if( driveDirectionStr == "face" ) {
    return DriveDirection::Face;
  } else if( driveDirectionStr == "cube" ) {
    return DriveDirection::Cube;
  } else if( driveDirectionStr == "micDirection" ) {
    return DriveDirection::MicDirection;
  } else if( driveDirectionStr == "left" ) {
    return DriveDirection::Left;
  } else if( driveDirectionStr == "right" ) {
    return DriveDirection::Right;
  }
  
  ANKI_VERIFY( false, "BehaviorDriveOffCharger.ParseDriveDirection.Unknown", "Unknown direction %s", driveDirectionStr.c_str() );
  return DriveDirection::Straight;
}

} // namespace Vector
} // namespace Anki

