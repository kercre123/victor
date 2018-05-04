/**
 * File: behaviorPlannerTest.cpp
 *
 * Author: ross
 * Created: 2018-05-03
 *
 * Description: Upon spotting cube(s), the robot drives to one of them, then to a point far from it, then back again, forever.
 *              Point the cube lights in some direction for the robot to follow that direction. Otherwise,
 *              the planner choses the side of the cube (and the cube, if there are multiple cubes), although
 *              it will stick to that selected pose near the cube for the remainder of the behavior.
 *              To reset, pick up the robot.
 *              The robot will use TTS to convey success/fail for each step
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorPlannerTest.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/sayTextAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/block.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/components/pathComponent.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
CONSOLE_VAR_RANGED(float, kCubeDistance_mm, "BehaviorPlannerTest", 25.0f, 0.0f, 100.0f );
CONSOLE_VAR_RANGED(float, kDistance_mm, "BehaviorPlannerTest", 1000.0f, 0.0f, 2000.0f );
CONSOLE_VAR(bool, kOnlyUseOriginalGoal, "BehaviorPlannerTest", true );

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlannerTest::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlannerTest::DynamicVariables::DynamicVariables()
{
  state = State::FirstRun;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlannerTest::BehaviorPlannerTest(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlannerTest::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
  modifiers.visionModesForActiveScope->insert( {VisionMode::DetectingMarkers, EVisionUpdateFrequency::High} );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlannerTest::OnBehaviorActivated() 
{
  // DON'T reset dynamic variables when resuming, in case mandatory physical reactions interrupt
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlannerTest::BehaviorUpdate() 
{
  if( !IsActivated() || IsControlDelegated() ) {
    return;
  }
  
  // reset the behavior when up in the air or on first run
  {
    const bool isPickedUp = GetBEI().GetRobotInfo().IsPickedUp();
    if( isPickedUp || (_dVars.state == State::FirstRun) ) {
      _dVars = DynamicVariables();
      _dVars.state = State::WaitingOnCube;
    }
  }
  
  if( _dVars.state == State::WaitingOnCube ) {
    
    const bool gotCube = FindPoseFromCube();
    if( gotCube ) {
      GoToCubePose();
    }
    
  } else if( _dVars.state == State::DrivingToCube ) {
    
    // finished driving to cube
    GoToFarAwayPose();
    
  } else if( _dVars.state == State::DrivingFar ) {
    
    // finished driving far away. go back to the originally selected cube pose
    GoToCubePose();
    
  }
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlannerTest::FindPoseFromCube()
{
  BlockWorldFilter filter;
  filter.SetAllowedFamilies( {ObjectFamily::LightCube} );
  
  std::vector<const ObservableObject*> objects;
  GetBEI().GetBlockWorld().FindLocatedMatchingObjects( filter, objects );
  
  if( objects.empty() ) {
    return false;
  }
  
  _dVars.cubePoses.clear();
  
  for( const auto* object : objects ) {
    const Block* block = dynamic_cast<const Block*>(object);
    if( block == nullptr ) {
      continue;
    }
    
    const auto& markerFront  = block->GetMarker( Block::FaceName::FRONT_FACE );
    const auto& markerLeft   = block->GetMarker( Block::FaceName::LEFT_FACE );
    const auto& markerBack   = block->GetMarker( Block::FaceName::BACK_FACE );
    const auto& markerRight  = block->GetMarker( Block::FaceName::RIGHT_FACE );
    const auto& markerTop    = block->GetMarker( Block::FaceName::TOP_FACE );
    const auto& markerBottom = block->GetMarker( Block::FaceName::BOTTOM_FACE );
    
    Pose3d waste;
    Vision::KnownMarker verticalMarker = block->GetTopMarker(waste);
    
    std::vector<const Pose3d*> poses;
    if( verticalMarker.GetCode() == markerTop.GetCode() || verticalMarker.GetCode() == markerBottom.GetCode() ) {
      // lights are up or down. generate 4 poses
      poses.push_back( &markerLeft.GetPose() );
      poses.push_back( &markerRight.GetPose() );
      poses.push_back( &markerFront.GetPose() );
      poses.push_back( &markerBack.GetPose() );
    } else {
      // lights are pointing somewhere. go there
      poses.push_back( &markerTop.GetPose() );
    }
    for( const auto* pose : poses ) {
      static const float halfHeight = 0.5f * block->GetSize().z();
      _dVars.cubePoses.emplace_back( M_PI_2,
                                     Z_AXIS_3D(),
                                     Point3f{0.f, -kCubeDistance_mm, -halfHeight},
                                     *pose );
    }
  }
  
  return !_dVars.cubePoses.empty();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlannerTest::CalcFarAwayPose()
{
  _dVars.drivePose = _dVars.cubePoses.front();
  _dVars.drivePose.RotateBy( RotationVector3d( Radians{M_PI_F}, Z_AXIS_3D() ) );
  _dVars.drivePose.TranslateForward( kDistance_mm );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlannerTest::GoToCubePose()
{
  _dVars.state = State::DrivingToCube;
  auto* action = new DriveToPoseAction( _dVars.cubePoses, false );
  action->SetMustContinueToOriginalGoal( kOnlyUseOriginalGoal );
  DelegateIfInControl( action, [this](const ActionResult& res){
    // if we havent done so already, remove all but the selected goal pose by the cube
    if( _dVars.cubePoses.size() > 1 ) {
      auto indexPtr = GetBEI().GetRobotInfo().GetPathComponent().GetLastSelectedIndex();
      if( indexPtr ) {
        int idx = static_cast<int>(*indexPtr);
        if( (idx >= 0) && (idx < _dVars.cubePoses.size()) ) {
          _dVars.cubePoses = {_dVars.cubePoses[idx]};
        }
      }
    }
    if( IActionRunner::GetActionResultCategory(res) == ActionResultCategory::RETRY ) {
      DelegateIfInControl( new SayTextAction( "CUBE RETRY",   SayTextVoiceStyle::Unprocessed ), &BehaviorPlannerTest::GoToCubePose );
    } else if( res != ActionResult::SUCCESS ) {
      DelegateIfInControl( new SayTextAction( "CUBE FAIL",    SayTextVoiceStyle::Unprocessed ) );
    } else {
      DelegateIfInControl( new SayTextAction( "CUBE SUCCESS", SayTextVoiceStyle::Unprocessed ) );
    }
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlannerTest::GoToFarAwayPose()
{
  _dVars.state = State::DrivingFar;
  CalcFarAwayPose();
  DelegateIfInControl( new DriveToPoseAction( _dVars.drivePose, false ), [this](const ActionResult& res){
    if( IActionRunner::GetActionResultCategory(res) == ActionResultCategory::RETRY ) {
      DelegateIfInControl( new SayTextAction( "DRIVE RETRY",   SayTextVoiceStyle::Unprocessed ), &BehaviorPlannerTest::GoToFarAwayPose );
    } else if( res != ActionResult::SUCCESS ) {
      DelegateIfInControl( new SayTextAction( "DRIVE FAIL",    SayTextVoiceStyle::Unprocessed ) );
    } else {
      DelegateIfInControl( new SayTextAction( "DRIVE SUCCESS", SayTextVoiceStyle::Unprocessed ) );
    }
  });
}

}
}
