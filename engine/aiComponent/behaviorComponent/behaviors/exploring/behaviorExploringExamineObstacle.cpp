/**
 * File: BehaviorExploringExamineObstacle.cpp
 *
 * Author: ross
 * Created: 2018-03-28
 *
 * Description: Behavior to examine extents of obstacle detected by the prox sensor
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/exploring/behaviorExploringExamineObstacle.h"

#include "clad/types/animationTrigger.h"
#include "coretech/common/engine/math/fastPolygon2d.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/components/movementComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_ProxObstacle.h"
#include "engine/navMap/memoryMap/memoryMapTypes.h"
#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  // todo: turn these into params
  const float kDistanceForActivation_mm = 150.0f;
  const float kDistanceForConeActivation_mm = 89.0f;
  const float kHalfAngleForConeActivation_rad = M_PI_4_F;
  const float kDistanceForStopTurn_mm = 300.0f;
  const float kDistanceForStartApproach_mm = 80.0f;
  const float kDistanceForStopApproach_mm = 50.0f;
  
  // For bumping an object. The robot is usually around 5-8cm from the object at this point, but may
  // not be facing it perfectly, so only bump if the object seems to have an appropriate width. The
  // delegated behavior decides if it is close enough
  CONSOLE_VAR_RANGED( float, kMinObjectWidthToBump_rad, "BehaviorExploring", DEG_TO_RAD(10.0f), 0.0f, M_PI_F);
  CONSOLE_VAR_RANGED( float, kMaxObjectWidthToBump_rad, "BehaviorExploring", DEG_TO_RAD(80.0f), 0.0f, M_TWO_PI_F);
  const float kProbBumpNominalObject = 0.8f;
  const float kProbReferenceBeforeBump = 0.5f;
  
  const float kMinDistForFarReaction_mm = 80.0f;
  
  // [1-100]:JPEG quality, -1: use PNG
  int8_t kImageQuality = 100;
  const bool kTakePhoto = false;
  
  float kReturnToCenterSpeed_deg_s = 300.0f;
  
  const unsigned int kNumFloodFillSteps = 10;
  
  
  // if discovered an obstacle within this long, and an unrelated path brings you nearby turn toward it
  int kTimeForTurnToPassingObstacles_ms = 5000;
}
  
#define SET_STATE(s) do{ _dVars.state = State::s; PRINT_NAMED_INFO("BehaviorExploringExamineObstacle.State", "State = %s", #s); } while(0);
  
using MemoryMapDataConstPtr = MemoryMapTypes::MemoryMapDataConstPtr;
using EContentTypePackedType = MemoryMapTypes::EContentTypePackedType;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploringExamineObstacle::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploringExamineObstacle::DynamicVariables::DynamicVariables()
{
  state = State::Initial;
  persistent.canSeeSideObstacle = false;
  persistent.seesFrontObstacle = false;
  persistent.seesSideObstacle = false;
  firstTurnDirectionIsLeft = false;
  initialPoseAngle_rad = 0.0f;
  totalObjectAngle_rad = 0.0f;
  playingScanSound = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploringExamineObstacle::BehaviorExploringExamineObstacle( const Json::Value& config )
 : ICozmoBehavior( config )
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploringExamineObstacle::~BehaviorExploringExamineObstacle()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorExploringExamineObstacle::WantsToBeActivatedBehavior() const
{
  const bool hasMap = GetBEI().HasMapComponent();
  return (_dVars.persistent.seesFrontObstacle || _dVars.persistent.seesSideObstacle) && hasMap;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploringExamineObstacle::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  // permissive since this behavior is owned
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploringExamineObstacle::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.bumpBehavior = BC.FindBehaviorByID( BEHAVIOR_ID(ExploringBumpObject) );
  _iConfig.referenceHumanBehavior = BC.FindBehaviorByID( BEHAVIOR_ID(ExploringReferenceHuman) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploringExamineObstacle::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig.bumpBehavior.get() );
  delegates.insert( _iConfig.referenceHumanBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploringExamineObstacle::OnBehaviorActivated() 
{
  // reset dynamic variables
  const auto persistent = _dVars.persistent;
  _dVars = DynamicVariables();
  _dVars.persistent = persistent;
  _dVars.firstTurnDirectionIsLeft = GetRNG().RandDbl()>0.5;
  _dVars.initialPoseAngle_rad = GetBEI().GetRobotInfo().GetPose().GetRotationAngle<'Z'>().ToFloat();
  
  if( _dVars.persistent.seesSideObstacle ) {
    DEV_ASSERT( !_dVars.persistent.seesFrontObstacle, "Should not be facing an obstacle now" );
    
    // turn first
    Pose2d pose(0.0f, _dVars.persistent.sideObstaclePosition );
    Pose3d pose3{ pose };
    pose3.SetParent( GetBEI().GetRobotInfo().GetWorldOrigin() );
    pose3.GetWithRespectTo( GetBEI().GetRobotInfo().GetPose(), pose3 );
    auto* turnAction = new TurnTowardsPoseAction( pose3 );
    auto* huhAction = new TriggerLiftSafeAnimationAction{ AnimationTrigger::ExploringHuhFar };
    auto* turnAndHuh = new CompoundActionSequential({ turnAction, huhAction });
    DelegateIfInControl( turnAndHuh, [this](ActionResult res){
      TransitionToNextAction();
    });
    
  } else {
    if( !_dVars.persistent.seesFrontObstacle ) {
      // can happen in tests
      PRINT_NAMED_WARNING( "BehaviorExploringExamineObstacle.OnBehaviorActivated.NoObst",
                           "Should be facing an obstacle now" );
    }
    
    auto& proxSensor = GetBEI().GetComponentWrapper(BEIComponentID::ProxSensor).GetComponent<ProxSensorComponent>();
    u16 distance_mm = 0;
    // ignore return value (sensor validity). this is only used to select an animation so isnt vital
    proxSensor.GetLatestDistance_mm( distance_mm );
    
    const bool obstacleIsFar = (distance_mm >= kMinDistForFarReaction_mm);
    const AnimationTrigger huhAnim = obstacleIsFar ? AnimationTrigger::ExploringHuhFar : AnimationTrigger::ExploringHuhClose;
    auto* huhAction = new TriggerLiftSafeAnimationAction{ huhAnim };
    DelegateIfInControl(huhAction, [this](ActionResult res){
      TransitionToNextAction();
    });
  }
  
  _dVars.persistent.seesFrontObstacle = false;
  _dVars.persistent.seesSideObstacle = false;
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploringExamineObstacle::OnBehaviorDeactivated()
{
  if( _dVars.playingScanSound ) {
    // behavior was likely interrupted. make sure the scan sound stops
    const auto event = AudioMetaData::GameEvent::GenericEvent::Stop__Robot_Vic_Sfx__Planning_Loop_Stop;
    GetBEI().GetRobotAudioClient().PostEvent( event, AudioMetaData::GameObjectType::Behavior );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploringExamineObstacle::TransitionToNextAction()
{
  if( _dVars.state == State::Initial ) {
    
    if( kTakePhoto ) {
      DevTakePhoto();
    }
  }
  
  using GE = AudioMetaData::GameEvent::GenericEvent;
  using GO = AudioMetaData::GameObjectType;
  
  const bool canVisitObstacle = (_dVars.state == State::Initial);
  if( canVisitObstacle ) {
    const bool useRobotWidth = true;
    if( RobotPathFreeOfObstacle( kDistanceForStartApproach_mm, useRobotWidth ) ) {
      
      SET_STATE( DriveToObstacle );
      
      // this will get canceled when we get close
      DriveStraightAction* action = new DriveStraightAction( kDistanceForStartApproach_mm );
      DelegateNow( action, [this](ActionResult res) {
        // if we got here, it wasn't canceled because of the absence of an obstacle.
        TransitionToNextAction();
      });
      return;
    }
  }
  
  // if we're here, we either skipped or have finished the initial approach maneuver
  auto action = std::make_unique<CompoundActionSequential>();
  if( (_dVars.state == State::Initial) || (_dVars.state == State::DriveToObstacle) ) {
    
    SET_STATE( FirstTurn );
    
    AnimationTrigger turnAnim = _dVars.firstTurnDirectionIsLeft
                                ? AnimationTrigger::ExploringScanToLeft
                                : AnimationTrigger::ExploringScanToRight;
    action->AddAction( new TriggerLiftSafeAnimationAction{ turnAnim } );
    
    // we manually trigger the audio since the looping scan animation doesn't always precede a
    // followup animation that could be used to stop the audio
    const auto event = GE::Play__Robot_Vic_Sfx__Planning_Loop_Play;
    GetBEI().GetRobotAudioClient().PostEvent( event, GO::Behavior );
    _dVars.playingScanSound = true;
    
  } else if( (_dVars.state == State::FirstTurn)
             || (_dVars.state == State::SecondTurn)
             || (_dVars.state == State::ReferenceHuman) )
  {
    
    const auto event = GE::Stop__Robot_Vic_Sfx__Planning_Loop_Stop;
    GetBEI().GetRobotAudioClient().PostEvent( event, GO::Behavior );
    _dVars.playingScanSound = false;
    
    // todo: clean this up. too many cases here, not enough time to separate them
    // logic is: after the first turn it should return to center. after the second turn it should either
    // exit, or return to center and bump the object, or reference a human then return to center then
    // bump the object
    
    if( _dVars.state == State::SecondTurn ) {
      
      // decide whether to bump or not
      bool shouldBump = false;
      if( (_dVars.totalObjectAngle_rad >= kMinObjectWidthToBump_rad)
          && (_dVars.totalObjectAngle_rad <= kMaxObjectWidthToBump_rad) )
      {
        // object is of a size that we should bump
        if( GetRNG().RandDbl() <= kProbBumpNominalObject ) {
          shouldBump = true;
        }
      }
      
      if( !shouldBump ) {
        // object is too thin or wide, exit the behavior instead of re-centering. If it re-centered,
        // it would immediately have to turn again to start driving, so it looks better to leave from this pose
        CancelSelf();
        return;
      }
      
      const Radians dAngle = GetBEI().GetRobotInfo().GetPose().GetRotationAngle<'Z'>() - _dVars.initialPoseAngle_rad;
      _dVars.totalObjectAngle_rad += fabsf( dAngle.ToFloat() );
      
      const bool refBehaviorWantsToBeActivated = _iConfig.referenceHumanBehavior->WantsToBeActivated();
      if( refBehaviorWantsToBeActivated && (GetRNG().RandDbl() < kProbReferenceBeforeBump) ) {
        _dVars.state = State::ReferenceHuman;
      } else {
        _dVars.state = State::ReturnToCenterEnd;
      }
    } else if( _dVars.state == State::FirstTurn ) {
      
      const Radians dAngle = GetBEI().GetRobotInfo().GetPose().GetRotationAngle<'Z'>() - _dVars.initialPoseAngle_rad;
      _dVars.totalObjectAngle_rad += fabsf( dAngle.ToFloat() );
      
      _dVars.state = State::ReturnToCenter;
      
    } else if( _dVars.state == State::ReferenceHuman ) {
      _dVars.state = State::ReturnToCenterEnd;
    }
    
      
    if( _dVars.state != State::ReferenceHuman ) {
      auto* parallelAction = new CompoundActionParallel();
      
      AnimationTrigger turnAnim;
      if( _dVars.firstTurnDirectionIsLeft ) {
        turnAnim = (_dVars.state == State::ReturnToCenter) ? AnimationTrigger::ExploringScanCenterFromLeft : AnimationTrigger::ExploringScanCenterFromRight;
      } else {
        turnAnim = (_dVars.state == State::ReturnToCenter) ? AnimationTrigger::ExploringScanCenterFromRight : AnimationTrigger::ExploringScanCenterFromLeft;
      }
      auto* animAction = new TriggerLiftSafeAnimationAction{ turnAnim };
      
      const float angle = _dVars.initialPoseAngle_rad;
      const bool isAbsAngle = true;
      auto* turnToCenterAction = new TurnInPlaceAction(angle, isAbsAngle);
      turnToCenterAction->SetMaxSpeed( DEG_TO_RAD(kReturnToCenterSpeed_deg_s) );
      
      // keep a weakptr to the turn action, so that the whole parallel action can be aborted the moment this one
      // ends
      _dVars.scanCenterAction = parallelAction->AddAction( turnToCenterAction );
      parallelAction->AddAction( animAction );
      
      action->AddAction( parallelAction );
    } else {
      DelegateIfInControl( _iConfig.referenceHumanBehavior.get(), [this](){
        TransitionToNextAction();
      });
      return;
    }
   
  } else if( _dVars.state == State::ReturnToCenter ) {
    
    SET_STATE( SecondTurn );
    
    // now turn the other way
    AnimationTrigger turnAnim = _dVars.firstTurnDirectionIsLeft
                                ? AnimationTrigger::ExploringScanToRight
                                : AnimationTrigger::ExploringScanToLeft;
    action->AddAction( new TriggerLiftSafeAnimationAction{ turnAnim } );
    
    const auto event = GE::Play__Robot_Vic_Sfx__Planning_Loop_Play;
    GetBEI().GetRobotAudioClient().PostEvent( event, GO::Behavior );
    _dVars.playingScanSound = true;
    
  } else if( _dVars.state == State::ReturnToCenterEnd ) {
    SET_STATE( Bumping );
    if( _iConfig.bumpBehavior->WantsToBeActivated() ) {
      DelegateIfInControl( _iConfig.bumpBehavior.get(), [this](){
        CancelSelf();
      });
      return;
    } else {
      CancelSelf();
      return;
    }
  }
  
  DelegateNow(action.release(), [&](ActionResult res) {
    // if we got here, it wasn't canceled because of the absence of an obstacle.
    TransitionToNextAction();
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploringExamineObstacle::BehaviorUpdate() 
{
  
  if( !IsActivated() ) {
    
    // flood fill any explored prox obstacles into unexplored obstacles.
    // while this one could be called from elsewhere, currently we only care about the result
    // when this behavior is in scope, so it's called here
    for( int i=0; i<kNumFloodFillSteps; ++i ) {
      if( !GetBEI().GetMapComponent().FlagProxObstaclesTouchingExplored() ) {
        break;
      }
    }
    
    const static bool forActivation = true;
    _dVars.persistent.seesFrontObstacle = RobotSeesObstacleInFront( kDistanceForActivation_mm, forActivation );
    // don't try evaluating the cone if there's already one in front
    if( !_dVars.persistent.seesFrontObstacle && _dVars.persistent.canSeeSideObstacle ) {
      _dVars.persistent.seesSideObstacle = RobotSeesNewObstacleInCone( kDistanceForConeActivation_mm,
                                                                       kHalfAngleForConeActivation_rad,
                                                                       _dVars.persistent.sideObstaclePosition );
    } else {
      _dVars.persistent.seesSideObstacle = false;
    }
    
    return;
  } else {
    if( GetBEI().HasMapComponent() ) {
      // mark prox obstscles that are within a short beam from the robot as explored.
      // this is done before the flood fill step ( see above comment ) so that obstacles flagged as
      // explored this tick count as seens for filling
      GetBEI().GetMapComponent().FlagProxObstaclesUsingPose();
      for( int i=0; i<kNumFloodFillSteps; ++i ) {
        if( !GetBEI().GetMapComponent().FlagProxObstaclesTouchingExplored() ) {
          break;
        }
      }
    }
  }
  
  
  if( (_dVars.state == State::FirstTurn) || (_dVars.state == State::SecondTurn) ) {
    // while in the middle of a turn, if no prox obstacle is seen, we can stop turning
    const static bool forActivation = false;
    const bool seenObstacle = RobotSeesObstacleInFront( kDistanceForStopTurn_mm, forActivation );
    if( !seenObstacle ) {
      CancelDelegates( false );
      TransitionToNextAction();
    }
  } else if( (_dVars.state == State::ReturnToCenter) || (_dVars.state == State::ReturnToCenterEnd) ) {
    if( !_dVars.scanCenterAction.lock() && IsControlDelegated() ) {
      // finished returning to center but the parallel action hasnt finished. cancel it and begin the next action
      CancelDelegates(false);
      TransitionToNextAction();
    }
  }
  const bool useRobotWidth = false; // only care how close we are, not if driving would hit an obstacle
  if( (_dVars.state == State::DriveToObstacle)
      && !RobotPathFreeOfObstacle( kDistanceForStopApproach_mm, useRobotWidth ) )
  {
    // driving up to an object and we got too close. stop.
    CancelDelegates(false);
    
    // todo: do we need another photo?
    
    TransitionToNextAction();
  }
  
}
  
bool BehaviorExploringExamineObstacle::RobotPathFreeOfObstacle( float dist_mm, bool useRobotWidth ) const
{
  // todo: seems like this could use planner helper methods if theyre exposed
  
  const auto& robotInfo = GetBEI().GetRobotInfo();
  
  const auto* memoryMap = GetBEI().GetMapComponent().GetCurrentMemoryMap();
  DEV_ASSERT( nullptr != memoryMap, "BehaviorExploringExamineObstacle.RobotPathFreeOfObstacle.NeedMemoryMap" );
  
  const Pose3d currRobotPose = robotInfo.GetPose();
  
  bool hasCollision;
  if( !useRobotWidth ) {
    
    // line segment extending dist_mm past robot
    Vec3f offset(dist_mm, 0.0f, 0.0f);
    Rotation3d rot = Rotation3d(0.f, Z_AXIS_3D());
    const Point2f p2 = (currRobotPose.GetTransform() * Transform3d(rot, offset)).GetTranslation();
    hasCollision = memoryMap->HasCollisionWithTypes({{Point2f{currRobotPose.GetTranslation()},
                                                      p2}},
                                                    MemoryMapTypes::kTypesThatAreObstacles);
  } else {
    
    // quad the width of the robot and extending dist_mm past the robot
    const float robotHalfWidth_mm = 0.5f*ROBOT_BOUNDING_Y;
    Vec3f offset1(0.0f,  -robotHalfWidth_mm, 0);
    Vec3f offset2(0.0f,   robotHalfWidth_mm, 0);
    Vec3f offset3(dist_mm, 0, 0);
    Rotation3d rot = Rotation3d(0.f, Z_AXIS_3D());
    
    const Point2f p1 = (currRobotPose.GetTransform() * Transform3d(rot, offset1)).GetTranslation();
    const Point2f p2 = (currRobotPose.GetTransform() * Transform3d(rot, offset2)).GetTranslation();
    const Point2f p3 = (currRobotPose.GetTransform() * Transform3d(rot, offset2 + offset3)).GetTranslation();
    const Point2f p4 = (currRobotPose.GetTransform() * Transform3d(rot, offset1 + offset3)).GetTranslation();
    
    hasCollision = memoryMap->HasCollisionWithTypes({{p1, p2, p3, p4}}, MemoryMapTypes::kTypesThatAreObstacles);
    
  }
  
  return !hasCollision;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorExploringExamineObstacle::RobotSeesObstacleInFront( float dist_mm, bool forActivation ) const
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  
  const auto* memoryMap = GetBEI().GetMapComponent().GetCurrentMemoryMap();
  DEV_ASSERT( nullptr != memoryMap, "BehaviorExploringExamineObstacle.WTBAB.NeedMemoryMap" );
  
  const Pose3d currRobotPose = robotInfo.GetPose();
  
  Vec3f offset(dist_mm, 0.0f, 0.0f);
  Rotation3d rot = Rotation3d(0.f, Z_AXIS_3D());
  const Point2f p2 = (currRobotPose.GetTransform() * Transform3d(rot, offset)).GetTranslation();
  
  // this behavior activates if a new unexplored ObstacleProx is seen, and ends when no explored or unexplored ObstacleProx is seen.
  const bool onlyUnexplored = forActivation;
  auto evalFunc = [&onlyUnexplored](MemoryMapDataConstPtr data){
    bool ret = false;
    if( data->type == MemoryMapTypes::EContentType::ObstacleProx ) {
      if( onlyUnexplored ) {
        auto castPtr = MemoryMapData::MemoryMapDataCast<const MemoryMapData_ProxObstacle>( data );
        ret = !castPtr->IsExplored();
      } else {
        ret = true;
      }
    }
    return ret;
  };
  
  const bool foundNewObstacle = memoryMap->AnyOf( {{Point2f{currRobotPose.GetTranslation()},
                                                   p2}},
                                                 evalFunc );
  return foundNewObstacle;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorExploringExamineObstacle::RobotSeesNewObstacleInCone( float dist_mm, float halfAngle_rad, Point2f& position ) const
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  
  const auto* memoryMap = GetBEI().GetMapComponent().GetCurrentMemoryMap();
  DEV_ASSERT( nullptr != memoryMap, "BehaviorExploringExamineObstacle.WTBAB.NeedMemoryMap" );
  
  const Pose3d currRobotPose = robotInfo.GetPose();
  
  Pose3d poseOffset{currRobotPose};
  poseOffset.TranslateForward( dist_mm );
  
  const float sideOffset_mm = dist_mm * tanf( halfAngle_rad );
  
  Vec3f rayOffset1(0.0f, -sideOffset_mm, 0);
  Vec3f rayOffset2(0.0f, sideOffset_mm, 0);
  Rotation3d rot = Rotation3d(0.f, Z_AXIS_3D());
  
  const Point2f t1 = (poseOffset.GetTransform() * Transform3d(rot, rayOffset1)).GetTranslation();
  const Point2f t2 = (poseOffset.GetTransform() * Transform3d(rot, rayOffset2)).GetTranslation();
  
  const Point2f t3 = currRobotPose.GetTranslation();
  
  // only react to unexplored prox obstacles
  const EContentTypePackedType nodeTypeFlags = EContentTypeToFlag( MemoryMapTypes::EContentType::ObstacleProx );
  // and only if theyre recent
  TimeStamp_t earliestTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp() - kTimeForTurnToPassingObstacles_ms;
  auto evalFunc = [&](MemoryMapDataConstPtr data){
    const bool sameType = IsInEContentTypePackedType( data->type, nodeTypeFlags );
    const bool newObstacle = data->GetFirstObservedTime() >= earliestTime;
    if( sameType && newObstacle ) {
      auto castPtr = MemoryMapData::MemoryMapDataCast<const MemoryMapData_ProxObstacle>( data );
      position = Point2f{castPtr->GetObservationPose().GetTranslation()};
    }
    return sameType && newObstacle;
  };
  
  const bool foundNewObstacle = memoryMap->AnyOf( {{ t1, t2, t3 }}, evalFunc );
  return foundNewObstacle;
}
  
void BehaviorExploringExamineObstacle::DevTakePhoto() const
{
  // shutter sound
  {
    using GE = AudioMetaData::GameEvent::GenericEvent;
    using GO = AudioMetaData::GameObjectType;
    GetBEI().GetRobotAudioClient().PostEvent(GE::Play__Robot_Vic_Sfx__Camera_Flash,
                                             GO::Behavior);
  }
  
  // save
  const std::string path = Util::FileUtils::FullFilePath({
    GetBEI().GetRobotInfo().GetContext()->GetDataPlatform()->GetCachePath("camera"),
    "images",
    "exploringObstacles"
  });
  auto& visionComponent = GetBEI().GetComponentWrapper(BEIComponentID::Vision).GetComponent<VisionComponent>();
  visionComponent.SetSaveImageParameters(ImageSendMode::SingleShot,
                                         path,
                                         "", // No basename: rely on auto-numbering
                                         kImageQuality);

}


#undef SET_STATE
  
}
}
