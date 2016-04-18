/**
 * File: behaviorWhiteboard
 *
 * Author: Raul
 * Created: 03/25/16
 *
 * Description: Whiteboard for behaviors to share information that is only relevant to them.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorWhiteboard_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorWhiteboard_H__

#include "anki/cozmo/basestation/externalInterface/externalInterface_fwd.h"
#include "anki/common/basestation/math/pose.h"
#include "clad/types/objectTypes.h"
#include "util/signals/simpleSignal_fwd.h"

#include <vector>
#include <set>
#include <list>

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
  
private:
  Pose3d _pose;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// BehaviorWhiteboard
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorWhiteboard
{
public:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // info for every marker that is a possible cube but we don't trust (based on distance or how quickly we saw it)
  struct PossibleMarker {
    PossibleMarker( const Pose3d& p, ObjectType objType ) : pose(p), type(objType) {}
    Pose3d pose;
    ObjectType type;
  };
  using PossibleMarkerList = std::list<PossibleMarker>;
  
  using BeaconList = std::vector<Beacon>;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor
  BehaviorWhiteboard(Robot& robot);
  
  // initializes the whiteboard, registers to events
  void Init();
  
  // what to do when the robot is delocalized
  void OnRobotDelocalized();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // list of possible markers
  const PossibleMarkerList& GetPossibleMarkers() const { return _possibleMarkers; }

  // beacons
  void AddBeacon( const Pose3d& beaconPos );
  const BeaconList& GetBeacons() const { return _beacons; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Tracked values
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // enable or disable the request. The id can be anything, but is intended to be used as `this` from an
  // object which is calling this function. The reaction will be disabled immediately when disable is called,
  // and will only be re-enabled when the corresponding number of called to RequestEnableCliffReaction are
  // made with the correct ids
  void DisableCliffReaction(void* id);
  void RequestEnableCliffReaction(void* id);
  bool IsCliffReactionEnabled() const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Events
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // template for all events we subscribe to
  template<typename T>
  void HandleMessage(const T& msg);

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // remove possible markers currently stored that
  void RemovePossibleMarkersMatching(ObjectType objectType, const Pose3d& pose);
  
  // update render of possible markers since they may have changed
  void UpdatePossibleMarkerRender();
  // update render of beacons
  void UpdateBeaconRender();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // the robot this whiteboard belongs to
  Robot& _robot;
  
  // signal handles for events we register to. These are currently unsubscribed when destroyed
  std::vector<Signal::SmartHandle> _signalHandles;
  
  std::multiset<void*> _disableCliffIds;  
 
  // list of markers we have not checked out yet. Using list because we make assume possible markers of same type
  // can be found at different locations
  PossibleMarkerList _possibleMarkers;
  
  // container of beacons currently defined (high level AI concept)
  BeaconList _beacons;
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
