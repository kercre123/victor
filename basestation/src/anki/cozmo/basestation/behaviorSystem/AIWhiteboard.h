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

#include "anki/cozmo/basestation/externalInterface/externalInterface_fwd.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/vision/basestation/faceIdTypes.h"
#include "clad/types/objectFamilies.h"
#include "clad/types/objectTypes.h"
#include "util/signals/simpleSignal_fwd.h"

#include <list>
#include <queue>
#include <set>
#include <vector>

namespace Anki {
namespace Cozmo {

class ObservableObject;
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
    ObjectInfo(const ObjectID& objId, ObjectFamily fam) : id(objId), family(fam) {}
    ObjectID id;
    ObjectFamily family;
  };
  using ObjectInfoList = std::vector<ObjectInfo>;
  
  // list of beacons
  using BeaconList = std::vector<AIBeacon>;

  // object usage reason for failure
  enum class ObjectUseAction {
    PickUpObject,   // pick up object from location
    StackOnObject,  // stack on top of object
    PlaceObjectAt   // place object at location
  };
  
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
  // recentFailureTimeout_sec: objects that failed to be picked up more recently than this ago will be
  // discarded
  bool FindUsableCubesOutOfBeacons(ObjectInfoList& outObjectList) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Face tracking
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // shared logic to get a face to track which requires the least panning and tilting (based on the current
  // robot pose). If preferNamed is true, then will prefer faces with a name to those without
  Vision::FaceID_t GetBestFaceToTrack(const std::set< Vision::FaceID_t >& possibleFaces,
                                      const bool preferNamedFaces) const;
  
  // returns true if all active cubes are known to be in beacons
  bool AreAllCubesInBeacons() const;
  
  // notify the whiteboard that we just failed to use this object. If no location is specified, the
  // object's current location is used
  void SetFailedToUse(const ObservableObject& object, ObjectUseAction action);
  void SetFailedToUse(const ObservableObject& object, ObjectUseAction action, const Pose3d& atLocation);

  // returns true if someone reported a failure to use the given object (by ID), less than the specified seconds ago
  // close to the given location.
  // recentSecs: use negative for any time at all, 0 for failed this tick, positive for failed less than X ago
  // atPose: where to compare
  // distThreshold_mm: set to negative to not compare poses, set to 0 or positive for atPose or around by X distance
  // angleThreshold: set to M_PI for any rotation, set to anything else for rotation difference with atPose's rotation
  // see versions:
  // if passed onto DidFailToUse, it will try to find a match for any object
  static constexpr const int ANY_OBJECT = -1;
  // any failure for given object
  bool DidFailToUse(const int objectID, ObjectUseAction reason) const;
  // any recent failure for given object
  bool DidFailToUse(const int objectID, ObjectUseAction reason, float recentSecs) const;
  // any failure for given object at given pose
  bool DidFailToUse(const int objectID, ObjectUseAction reason, const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const;
  // any recent failure for given object at given pose
  bool DidFailToUse(const int objectID, ObjectUseAction reason, float recentSecs, const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Beacons
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // add a new beacon
  void AddBeacon( const Pose3d& beaconPos );
  
  // notify whiteboard that someone tried to find good locations for cubes in this beacon and it was not possible
  void FailedToFindLocationInBeacon(AIBeacon* beacon);

  // return current active beacon if any, or nullptr if none are active
  const AIBeacon* GetActiveBeacon() const;
  AIBeacon* GetActiveBeacon();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // This getter iterates the list of possible objects currently stored and retrieves only those that can be located in
  // current origin. Note this causes a calculation WRT origin (consider caching in the future if need to optimize)
  void GetPossibleObjectsWRTOrigin(PossibleObjectVector& possibleObjects) const;

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
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // information for object failures (failed to pick up...)
  struct FailureInfo {
    FailureInfo() : _pose(), _timestampSecs(0.0f) {}
    FailureInfo(const Pose3d& pose, float time) : _pose(pose), _timestampSecs(time) {}
    Pose3d _pose;
    float  _timestampSecs;
  };
  
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
  // Failures
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // failure container typedefs
  using FailureList = std::list<FailureInfo>; // I actually want a queue, but can't iterate queues (deque is more than queue)
  using ObjectFailureTable = std::map<int, FailureList>; // easier to limit size in inner container than multimap
  
  // helper to search for failures in the given failure table
  bool FindMatchingEntry(const ObjectFailureTable& failureTable, const int objectID, float recentSecs,
                         const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const;
  
  // helper to compare whether the given entry matches the search parameters or not
  bool EntryMatches(const FailureInfo& entry, float recentSecs, const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const;
  
  // retrieve ObjectFailureTable for the given failure reason/action
  const ObjectFailureTable& GetObjectFailureTable(ObjectUseAction action) const;
  ObjectFailureTable& GetObjectFailureTable(ObjectUseAction action);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // the robot this whiteboard belongs to
  Robot& _robot;
  
  // signal handles for events we register to. These are currently unsubscribed when destroyed
  std::vector<Signal::SmartHandle> _signalHandles;
  
  // stores when someone notifies us that the robot failed to do an action (by objectId)
  ObjectFailureTable _pickUpFailures;
  ObjectFailureTable _stackOnFailures;
  ObjectFailureTable _placeAtFailures;
  
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
