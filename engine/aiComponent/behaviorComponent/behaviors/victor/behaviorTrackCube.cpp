/**
 * File: behaviorTrackCube.cpp
 *
 * Author: ross
 * Created: 2018-03-15
 *
 * Description: tracks a cube with eyes
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorTrackCube.h"
#include "engine/actions/trackObjectAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/visionComponent.h"
#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  static constexpr float kMaxNormalAngle = DEG_TO_RAD(60.0f); // how steep of an angle we can see
  static constexpr float kMinImageSizePix = 0.0f; // just check if we are looking at it
  
  const char* const kMaxTimeSinceObsKey = "maxTimeSinceObserved_ms";
  const char* const kMaxDistanceKey = "maxDistance_mm";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTrackCube::InstanceConfig::InstanceConfig()
{
  maxTimeSinceObserved_ms = 2000;
  maxDistance_mm = 1000;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTrackCube::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTrackCube::BehaviorTrackCube(const Json::Value& config)
 : ICozmoBehavior(config)
{
  _iConfig.maxTimeSinceObserved_ms = config.get( kMaxTimeSinceObsKey, _iConfig.maxTimeSinceObserved_ms ).asInt();
  _iConfig.maxDistance_mm = config.get( kMaxDistanceKey, _iConfig.maxDistance_mm ).asInt();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTrackCube::~BehaviorTrackCube()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTrackCube::WantsToBeActivatedBehavior() const
{
  return _dVars.cube.IsSet();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackCube::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActivatableScope->insert( {VisionMode::DetectingMarkers, EVisionUpdateFrequency::High} );
  modifiers.visionModesForActiveScope->insert( {VisionMode::DetectingMarkers, EVisionUpdateFrequency::High} );
  modifiers.behaviorAlwaysDelegates = false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackCube::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kMaxTimeSinceObsKey,
    kMaxDistanceKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackCube::OnBehaviorActivated()
{
  // reset dynamic variables
  const auto& cube = _dVars.cube;
  _dVars = DynamicVariables();
  _dVars.cube = cube;
  
  StartAction();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID BehaviorTrackCube::GetVisibleCube() const
{
  BlockWorldFilter filter;
  filter.AddAllowedFamily(ObjectFamily::LightCube);
  filter.SetFilterFcn(nullptr);
  
  // list of visible cubes, sorted by how close they are in asc order
  std::map<float, const ObservableObject*> objectsByDist;
  
  std::vector<const ObservableObject*> objects;
  GetBEI().GetBlockWorld().FindLocatedMatchingObjects(filter, objects);
  
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const VisionComponent& visComp = GetBEI().GetVisionComponent();
  
  // the closest moving one is "most interesting" and is always selected, even if it's farther than a stationary one
  ObjectID movingObject;
  float movingObjectDist_mm = std::numeric_limits<float>::max();
  
  const RobotTimeStamp_t currentTime_ms = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
  
  for( const auto* object : objects ) {
    
    if( object != nullptr ) {
      using NotVisibleReason = Vision::KnownMarker::NotVisibleReason;
      NotVisibleReason reason = object->IsVisibleFromWithReason(visComp.GetCamera(),
                                                                kMaxNormalAngle,
                                                                kMinImageSizePix,
                                                                false);
      
      // ignore the occluded reason because it seems broken (VIC-2699). that reason is only returned if
      // the object is otherwise visible, so treat it as visible for all intensive porpoises
      const bool isVisible = (reason == NotVisibleReason::IS_VISIBLE) || (reason == NotVisibleReason::OCCLUDED);
      // currently, cubes arent removed if they are not seen in their original location, so check the last time they were seen
      const bool recent = (_iConfig.maxTimeSinceObserved_ms < 0)
                          || (object->GetLastObservedTime() + _iConfig.maxTimeSinceObserved_ms >= currentTime_ms);
      if( isVisible && recent ) {
        const float dist = ComputeDistanceBetween( object->GetPose(), robotPose );
        if( dist <= _iConfig.maxDistance_mm ) {
          objectsByDist.emplace( dist, object );
          const bool isMoving = object->IsMoving();
          if( isMoving && (dist < movingObjectDist_mm) ) {
            movingObject = object->GetID();
            movingObjectDist_mm = dist;
          }
        }
      }
    }
  }
  
  if( movingObject.IsSet() ) {
    return movingObject;
  }
  
  if( !objectsByDist.empty() ) {
    // return closest cube
    return objectsByDist.begin()->second->GetID();
  } else {
    return ObjectID();
  }
  
}

void BehaviorTrackCube::StartAction()
{
  auto* action = new TrackObjectAction( _dVars.cube );
  action->SetMoveEyes( true );
  action->SetMode( ITrackAction::Mode::HeadOnly );
  action->SetUpdateTimeout( 0.001f * _iConfig.maxTimeSinceObserved_ms );
  
  DelegateIfInControl( action, [this](ActionResult res) {
    // if the action ended naturally, the cube was lost
    CancelSelf();
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackCube::BehaviorUpdate()
{
  ObjectID cube = GetVisibleCube();
  
  const ObjectID oldCube = _dVars.cube;
  _dVars.cube = cube;
  
  if( !IsActivated() ) {
    return;
  }
  
  
  if( (cube != oldCube) && cube.IsSet() ) {
    // found a cube, or found a more interesting cube
    CancelDelegates(false);
    StartAction();
  }
  
  if( !_dVars.cube.IsSet() ) {
    CancelSelf();
  }
}

}
}
