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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/components/movementComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_ProxObstacle.h"
#include "engine/navMap/memoryMap/memoryMapTypes.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/random/randomGenerator.h"

// todo: remove when lights are removed
#include "engine/components/bodyLightComponent.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  // todo: turn these into params
  const float kDistanceForActivation_mm = 150.0f;
  const float kDistanceForConeActivation_mm = 89.0f;
  const float kHalfAngleForConeActivation_rad = M_PI_4_F;
  const float kDistanceForStopTurn_mm = 300.0f;
  const float kDistanceForStartApproach_mm = 120.0f;
  const float kDistanceForStopApproach_mm = 80.0f;
  
  const float kMaxTurnAngle_deg = 135.0f; // 90 + 45 looks good for coming at a wall at an acute angle
  
  // [1-100]:JPEG quality, -1: use PNG
  int8_t kImageQuality = 100;
  const bool kTakePhoto = false;
  
  float kReturnToCenterSpeed_deg_s = 300.0f;
  float kTurnSpeed_deg_s = 30.0f;
  
  const unsigned int kNumFloodFillSteps = 10;
  
  
  // if discovered an obstacle within this long, and an unrelated path brings you nearby turn toward it
  int kTimeForTurnToPassingObstacles_ms = 5000;
  
  // backpack lights so I know when this behavior is active
  static const BackpackLights kLightsOff =
  {
    .onColors               = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
    .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
    .onPeriod_ms            = {{100,0,0}},
    .offPeriod_ms           = {{100,0,0}},
    .transitionOnPeriod_ms  = {{0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0}},
    .offset                 = {{0,0,0}}
  };
  
  static const BackpackLights kLightsActiveFront =
  {
    .onColors               = {{NamedColors::RED,NamedColors::BLACK,NamedColors::BLACK}},
    .offColors              = {{NamedColors::RED,NamedColors::BLACK,NamedColors::BLACK}},
    .onPeriod_ms            = {{100,0,0}},
    .offPeriod_ms           = {{100,0,0}},
    .transitionOnPeriod_ms  = {{0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0}},
    .offset                 = {{0,0,0}}
  };
  static const BackpackLights kLightsActiveSide =
  {
    .onColors               = {{NamedColors::BLUE,NamedColors::BLACK,NamedColors::BLACK}},
    .offColors              = {{NamedColors::BLUE,NamedColors::BLACK,NamedColors::BLACK}},
    .onPeriod_ms            = {{100,0,0}},
    .offPeriod_ms           = {{100,0,0}},
    .transitionOnPeriod_ms  = {{0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0}},
    .offset                 = {{0,0,0}}
  };
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
  persistent.canVisitObstacle = false;
  persistent.canSeeSideObstacle = false;
  persistent.seesFrontObstacle = false;
  persistent.seesSideObstacle = false;
  firstTurnDirectionIsLeft = false;
  initialPoseAngle_rad = 0.0f;
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
void BehaviorExploringExamineObstacle::GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const
{
//  const char* list[] = {
//    // TODO: insert any possible root-level json keys that this class is expecting.
//    // TODO: replace this method with a simple {} in the header if this class doesn't use the ctor's "config" argument.
//  };
  //expectedKeys.insert( std::begin(list), std::end(list) );
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
    
    GetBEI().GetBodyLightComponent().SetBackpackLights(kLightsActiveSide);
    
    // turn first
    Pose2d pose(0.0f, _dVars.persistent.sideObstaclePosition );
    Pose3d pose3{ pose };
    pose3.SetParent( GetBEI().GetRobotInfo().GetWorldOrigin() );
    pose3.GetWithRespectTo( GetBEI().GetRobotInfo().GetPose(), pose3 );
    DelegateIfInControl(new TurnTowardsPoseAction( pose3 ), [this](ActionResult res){
      TransitionToNextAction();
    });
    
  } else {
    if( !_dVars.persistent.seesFrontObstacle ) {
      // can happen in tests
      PRINT_NAMED_WARNING( "BehaviorExploringExamineObstacle.OnBehaviorActivated.NoObst",
                           "Should be facing an obstacle now" );
    }
    
    GetBEI().GetBodyLightComponent().SetBackpackLights(kLightsActiveFront);
    
    // todo: should probably back up if this behavior activates and it's too close. for now, the reaction
    // animation moves it back a little bit
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ReactToObstacle), [this](ActionResult res){
      TransitionToNextAction();
    });
  }
  
  _dVars.persistent.seesFrontObstacle = false;
  _dVars.persistent.seesSideObstacle = false;
  
}
  
//
void BehaviorExploringExamineObstacle::TransitionToNextAction()
{
  if( _dVars.state == State::Initial ) {
    
    if( kTakePhoto ) {
      DevTakePhoto();
    }
  }
  
  const bool shouldVisitObstacle = _dVars.persistent.canVisitObstacle && (_dVars.state == State::Initial);
  if( shouldVisitObstacle ) {
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
    // these head motions will probably be animations or something
    action->AddAction( new MoveHeadToAngleAction( DEG_TO_RAD(15.0f) ) );
    action->AddAction( new WaitAction( 0.5f ) );
    action->AddAction( new MoveHeadToAngleAction( DEG_TO_RAD(0.0f) ) );
    const float angle = _dVars.firstTurnDirectionIsLeft ? -DEG_TO_RAD(kMaxTurnAngle_deg) : DEG_TO_RAD(kMaxTurnAngle_deg);
    const bool isAbsAngle = false;
    auto* turnAction = new TurnInPlaceAction(angle, isAbsAngle);
    turnAction->SetMaxSpeed(DEG_TO_RAD(kTurnSpeed_deg_s));
    action->AddAction( turnAction );
    
  } else if( (_dVars.state == State::FirstTurn) || (_dVars.state == State::SecondTurn) ) {
    
    _dVars.state = (_dVars.state == State::SecondTurn) ? State::ReturnToCenterEnd : State::ReturnToCenter;
    
    if( _dVars.state == State::ReturnToCenterEnd ) {
      GetBEI().GetBodyLightComponent().SetBackpackLights(kLightsOff);
      CancelSelf();
      return;
    }
    
    // these head motions will probably be animations or something
    action->AddAction( new MoveHeadToAngleAction( DEG_TO_RAD(15.0f) ) );
    action->AddAction( new WaitAction( 0.5f ) );
    action->AddAction( new MoveHeadToAngleAction( DEG_TO_RAD(0.0f) ) );
    const float angle = _dVars.initialPoseAngle_rad;
    const bool isAbsAngle = true;
    auto* turnToCenterAction = new TurnInPlaceAction(angle, isAbsAngle);
    turnToCenterAction->SetMaxSpeed( DEG_TO_RAD(kReturnToCenterSpeed_deg_s) );
    action->AddAction( turnToCenterAction );
   
  } else if( _dVars.state == State::ReturnToCenter ) {
    
    SET_STATE( SecondTurn );
    
    // now turn the other way
    const float angle = _dVars.firstTurnDirectionIsLeft ? DEG_TO_RAD(kMaxTurnAngle_deg) : -DEG_TO_RAD(kMaxTurnAngle_deg);
    const bool isAbsAngle = false;
    auto* turnAction = new TurnInPlaceAction(angle, isAbsAngle);
    turnAction->SetMaxSpeed(DEG_TO_RAD(kTurnSpeed_deg_s));
    action->AddAction( turnAction );
    
  } else if( _dVars.state == State::ReturnToCenterEnd ) {
    GetBEI().GetBodyLightComponent().SetBackpackLights(kLightsOff);
    CancelSelf();
    return;
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
  
  const bool foundNewObstacle = memoryMap->Eval( {{Point2f{currRobotPose.GetTranslation()},
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
  
  const bool foundNewObstacle = memoryMap->Eval( {{ t1, t2, t3 }}, evalFunc );
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
  const std::string path = Util::FileUtils::FullFilePath({ "exploringObstacles" });
  auto& visionComponent = GetBEI().GetComponentWrapper(BEIComponentID::Vision).GetValue<VisionComponent>();
  visionComponent.SetSaveImageParameters(ImageSendMode::SingleShot,
                                         path,
                                         kImageQuality);

}


#undef SET_STATE
  
}
}
