/**
 * File: AIWhiteboard
 *
 * Author: Raul
 * Created: 03/25/16
 *
 * Description: Whiteboard for behaviors to share information that is only relevant to them.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_AIWhiteboard_H__
#define __Cozmo_Basestation_BehaviorSystem_AIWhiteboard_H__

#include "AIBeacon.h" 

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface_fwd.h"
#include "clad/types/objectFamilies.h"
#include "clad/types/objectTypes.h"
#include "util/signals/simpleSignal_fwd.h"

#include <vector>
#include <set>
#include <list>

namespace Anki {
namespace Cozmo {

class Robot;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AIWhiteboard
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class AIWhiteboard
{
public:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // info for every marker that is a possible cube but we don't trust (based on distance or how quickly we saw it)
  // or other information, like old cubes that have moved, etc.
  struct PossibleObject {
    PossibleObject( const Pose3d& p, ObjectType objType ) : pose(p), type(objType) {}
    Pose3d pose;
    ObjectType type;
  };
  using PossibleObjectList = std::list<PossibleObject>;
  using PossibleObjectVector = std::vector<PossibleObject>;
  
  // info for objects we search from the whiteboard and return as result of the search
  struct ObjectInfo {
    ObjectInfo(ObjectID objId, ObjectFamily fam) : id(objId), family(fam) {}
    ObjectID id;
    ObjectFamily family;
  };
  using ObjectInfoList = std::vector<ObjectInfo>;
  
  // list of beacons
  using BeaconList = std::vector<AIBeacon>;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor
  AIWhiteboard(Robot& robot);
  
  // initializes the whiteboard, registers to events
  void Init();
  
  // what to do when the robot is delocalized
  void OnRobotDelocalized();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Possible Objects
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // called when Cozmo can identify a clear quad (no borders, obstacles, etc)
  void ProcessClearQuad(const Quad2f& quad);

  // called when we've searched for a possible object at a given pose, but failed to find it
  void FinishedSearchForPossibleCubeAtPose(ObjectType objectType, const Pose3d& pose);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Cube search
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // find any usable cubes (not unknown) that are not in a beacon, and return true if any are found.
  bool FindUsableCubesOutOfBeacons(ObjectInfoList& outObjectList) const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // This getter iterates the list of possible objects currently stored and retrieves only those that can be located in
  // current origin. Note this causes a calculation WRT origin (consider caching in the future if need to optimize)
  void GetPossibleObjectsWRTOrigin(PossibleObjectVector& possibleObjects) const;

  // beacons
  void AddBeacon( const Pose3d& beaconPos );
  const BeaconList& GetBeacons() const { return _beacons; }

  // return current active beacon if any, or nullptr if none are active
  const AIBeacon* GetActiveBeacon() const;
  
  // return time at which Cozmo got off the charger by himself
  void GotOffChargerAtTime(const float time_sec) { _gotOffChargerAtTime_sec = time_sec; }
  float GetTimeAtWhichRobotGotOffCharger() const { return _gotOffChargerAtTime_sec; }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Events
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // template for all events we subscribe to
  template<typename T>
  void HandleMessage(const T& msg);

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Markers
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // consider adding an object to possible object list
  void ConsiderNewPossibleObject(ObjectType objectType, const Pose3d& pose);
  
  // remove possible objects currently stored close to the given pose and that match the object type
  void RemovePossibleObjectsMatching(ObjectType objectType, const Pose3d& pose);

  // removes markers that belong to a zombie map
  void RemovePossibleObjectsFromZombieMaps();
  
  // update render of possible markers since they may have changed
  void UpdatePossibleObjectRender();
  // update render of beacons
  void UpdateBeaconRender();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // the robot this whiteboard belongs to
  Robot& _robot;
  
  // signal handles for events we register to. These are currently unsubscribed when destroyed
  std::vector<Signal::SmartHandle> _signalHandles;
  
  // time at which the robot got off the charger by itself. Negative value means never
  float _gotOffChargerAtTime_sec;
 
  // list of markers/objects we have not checked out yet
  PossibleObjectList _possibleObjects;
  
  // container of beacons currently defined (high level AI concept)
  BeaconList _beacons;

};
  

} // namespace Cozmo
} // namespace Anki

#endif //
