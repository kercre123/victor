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
#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorBeacon_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorBeacon_H__

#include "anki/common/basestation/math/pose.h"

namespace Anki {
namespace Cozmo {

class Robot;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Beacon
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// info for beacons (beacons is a concept for exploration, it's a 'base/headquarters' to put cubes for localization)
class Beacon {
public:
  Beacon( const Pose3d& p ) : _pose(p) {}
  const Pose3d& GetPose() const { return _pose;}
  
  // returns true if given position is within this beacon
  bool IsLocWithinBeacon(const Vec3f& loc) const;
  
  // return radius of this beacon
  float GetRadius() const;
  
private:
  Pose3d _pose;
};

} // namespace Cozmo
} // namespace Anki

#endif //
