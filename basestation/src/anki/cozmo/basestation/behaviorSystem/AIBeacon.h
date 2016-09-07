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
#ifndef __Cozmo_Basestation_BehaviorSystem_AIBeacon_H__
#define __Cozmo_Basestation_BehaviorSystem_AIBeacon_H__

#include "anki/common/basestation/math/pose.h"

namespace Anki {
namespace Cozmo {

class Robot;
class AIWhiteboard;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Beacon
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// info for beacons (beacons is a concept for exploration, it's a 'base/headquarters' to put cubes for localization)
class AIBeacon {
public:
  AIBeacon( const Pose3d& p ) : _pose(p), _lastTimeFailedToFindLocation_secs(0.0f) {}
  const Pose3d& GetPose() const { return _pose;}
  
  // returns true if given position is within this beacon. If inwardThreshold is set, the location has to be inside
  // the beacon radius by that additional distance
  bool IsLocWithinBeacon(const Pose3d& pose, float inwardThreshold_mm=0.0f) const;
  
  // return radius of this beacon
  float GetRadius() const;
  
  // return the timestamp of the last time we failed to find a location for a cube inside this beacon
  float GetLastTimeFailedToFindLocation() const { return _lastTimeFailedToFindLocation_secs; }
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // privilege for the whiteboard so that no one changes beacons without going through the whiteboard
  friend class AIWhiteboard;

  // flag that we failed to find a good location in this beacon
  void FailedToFindLocation();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // beacon center
  Pose3d _pose;
  
  // if we ever fail to find locations at this beacon, flag it so that we don't try to use it again
  float _lastTimeFailedToFindLocation_secs;
};

} // namespace Cozmo
} // namespace Anki

#endif //
