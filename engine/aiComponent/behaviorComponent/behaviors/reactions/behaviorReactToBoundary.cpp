/**
 * File: BehaviorReactToBoundary.cpp
 *
 * Author: Matt Michini
 * Created: 2019-01-16
 *
 * Description: React to a boundary in the nav map
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToBoundary.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorFindCube.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "engine/cozmoContext.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_Boundary.h"
#include "engine/viz/vizManager.h"

#include "coretech/common/engine/math/ball.h"
#include "coretech/common/engine/math/polygon_impl.h"

#include "clad/externalInterface/messageEngineToGame.h"


namespace Anki {
namespace Vector {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBoundary::BehaviorReactToBoundary(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBoundary::~BehaviorReactToBoundary()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToBoundary::WantsToBeActivatedBehavior() const
{
  // Want to be activated if the robot is at all touching the 'boundary'
  const float diskCheckRadius_mm = 25.f;
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const Point2f robotPt{robotPose.GetTranslation()};
  auto isBoundaryType = [] (const auto& data) { return (data->type == MemoryMapTypes::EContentType::Boundary); };
  const bool touchingBoundary = GetBEI().GetMapComponent().CheckForCollisions(Ball2f(robotPt, diskCheckRadius_mm),
                                                                              isBoundaryType);
  
  return touchingBoundary;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBoundary::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false; // TMP
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBoundary::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBoundary::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
//  const char* list[] = {
//  };
//  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBoundary::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  // Find the nearest boundary so that we can react to it
  const Point2f robotPt{GetBEI().GetRobotInfo().GetPose().GetTranslation()};
  
  std::vector<MemoryMapTypes::MemoryMapDataConstPtr> boundaries;
  auto result = GetBEI().GetMapComponent().FindBoundaries(boundaries);
  DEV_ASSERT(result == RESULT_OK, "BehaviorReactToBoundary.OnBehaviorActivated.NoBoundaries");
  
  std::shared_ptr<const MemoryMapData_Boundary> closestBoundary;;
  float minDist = std::numeric_limits<float>::max();
  for (auto& dataPtr : boundaries) {
    auto boundary = MemoryMapData::MemoryMapDataCast<MemoryMapData_Boundary>(dataPtr);
    const auto& centroid = boundary->_boundaryQuad.ComputeCentroid();
    const float dist = ComputeDistanceBetween(robotPt, centroid);
    if (dist < minDist) {
      closestBoundary = boundary.GetSharedPtr();
      minDist = dist;
    }
  }
  
  auto* vizm = GetBEI().GetRobotInfo().GetContext()->GetVizManager();
  vizm->DrawQuadAsSegments(GetDebugLabel(), closestBoundary->_boundaryQuad, 10.f, NamedColors::PINK, false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBoundary::BehaviorUpdate() 
{
  if (!IsActivated()) {
    return;
  }

}

}
}
