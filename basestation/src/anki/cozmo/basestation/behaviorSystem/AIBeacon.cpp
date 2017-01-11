/**
 * File: AIBeacon
 *
 * Author: Raul
 * Created: 03/25/16
 *
 * Description: Beacon is a 'base or headquarters' to put cubes inside a radius for localization or to show purpose
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "anki/cozmo/basestation/behaviorSystem/AIBeacon.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/utils/timer.h"

#include "util/console/consoleInterface.h"


namespace Anki {
namespace Cozmo {

// beacon radius (it could be problem if we had too many cubes and not enough beacons to place them)
CONSOLE_VAR(float, kB_BeaconRadius_mm, "AIBeacon", 175.0f);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Beacon
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIBeacon::IsLocWithinBeacon(const Pose3d& pose, float inwardThreshold_mm) const
{
  DEV_ASSERT(inwardThreshold_mm < kB_BeaconRadius_mm, "AIBeacon.IsLocWithinBeacon.InvalidInwardTreshold");
  
  Pose3d relative;
  if ( !pose.GetWithRespectTo(_pose, relative) )
  {
    // we currently don't support beacons in arbitrary origins, so this should not happen
    DEV_ASSERT(false, "AIBeacon.IsLocWithinBeacon.NoPoseTransform");
    return false;
  }

  const float distSQ = relative.GetTranslation().LengthSq();
  const float innerRad = (kB_BeaconRadius_mm-inwardThreshold_mm);
  const float innerRadSQ = innerRad*innerRad;
  const bool inInnerRadius = FLT_LE(distSQ, innerRadSQ);
  return inInnerRadius;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float AIBeacon::GetRadius() const
{
  return kB_BeaconRadius_mm;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIBeacon::FailedToFindLocation()
{
  const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastTimeFailedToFindLocation_secs = curTime;
}

} // namespace Cozmo
} // namespace Anki
