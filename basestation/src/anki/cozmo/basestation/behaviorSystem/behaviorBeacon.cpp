/**
 * File: behaviorBeacon
 *
 * Author: Raul
 * Created: 03/25/16
 *
 * Description: Beacon is a 'base or headquarters' to put cubes inside a radius for localization or to show purpose
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "anki/cozmo/basestation/behaviorSystem/behaviorBeacon.h"

#include "anki/common/basestation/math/point_impl.h"

#include "util/console/consoleInterface.h"


namespace Anki {
namespace Cozmo {

// beacon radius (it could be problem if we had too many cubes and not enough beacons to place them)
CONSOLE_VAR(float, kB_BeaconRadius_mm, "BehaviorBeacon", 150.0f);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Beacon
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Beacon::IsLocWithinBeacon(const Vec3f& loc) const
{
  const float distSQ = (_pose.GetTranslation() - loc).LengthSq();
  const bool inRadius = FLT_LE(distSQ, (kB_BeaconRadius_mm*kB_BeaconRadius_mm));
  return inRadius;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float Beacon::GetRadius() const
{
  return kB_BeaconRadius_mm;
}

} // namespace Cozmo
} // namespace Anki
