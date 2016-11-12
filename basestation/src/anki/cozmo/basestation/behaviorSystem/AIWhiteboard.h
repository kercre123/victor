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
class BlockWorldFilter;

namespace DefailtFailToUseParams {
constexpr static const float kTimeObjectInvalidAfterFailure_sec = 30.f;
constexpr static const float kObjectInvalidAfterFailureRadius_mm = 60.f;
static const Radians kAngleToleranceAfterFailure_radians = M_PI;
}

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
    PlaceObjectAt,   // place object at location
    RollOrPopAWheelie // roll or pop a wheelie on a block
  };

  // intention of what to do with an object. Add things here to centralize logic for the best object to use
  enum class ObjectUseIntention {
    // any object which can be picked up
    PickUpAnyObject,

    // only pick up upright objects, unless rolling is locked (in which case, pick up any object)
    PickUpObjectWithAxisCheck
   };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor
  AIWhiteboard(Robot& robot);

  virtual ~AIWhiteboard();
  
  // initializes the whiteboard, registers to events
  void Init();

  // Tick the whiteboard
  void Update();
  
  // what to do when the robot is delocalized
  void OnRobotDelocalized();
  // what to do when the robot relocalizes to a cube
  void OnRobotRelocalized();

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
  
  // finds cubes in the given beacon and returns them in the given list. Returns true if the list is not empty (=if
  // found any cubes at all)
  bool FindCubesInBeacon(const AIBeacon* beacon, ObjectInfoList& outObjectList) const;
  
  // returns true if all active cubes are known to be in beacons
  bool AreAllCubesInBeacons() const;
  
  // notify the whiteboard that we just failed to use this object.
  // uses object's current location
  void SetFailedToUse(const ObservableObject& object, ObjectUseAction action);
  // specify location manually - should only be used for PlaceObjectAt
  void SetFailedToUse(const ObservableObject& object, ObjectUseAction action, const Pose3d& atLocation);

  // returns true if someone reported a failure to use the given object (by ID), less than the specified seconds ago
  // close to the given location with the given reason(s).
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

  // same as above, with multiple reasons considered at the same time. Returns true if there is a failure for
  // _any_ of the specified reasons
  using ReasonsContainer = std::set< ObjectUseAction >;
  bool DidFailToUse(const int objectID, ReasonsContainer reasons) const;
  bool DidFailToUse(const int objectID, ReasonsContainer reasons, float recentSecs) const;
  bool DidFailToUse(const int objectID, ReasonsContainer reasons, const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const;
  bool DidFailToUse(const int objectID, ReasonsContainer reasons, float recentSecs, const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const;
  


  // Given an intent to use an object, let the whiteboard decide which objects are valid for the robot to
  // use. This will check failure to use and other sensible defaults.
  const std::set< ObjectID >& GetValidObjectsForAction(ObjectUseIntention action) const;

  // small helper to check if a given ID is valid
  bool IsObjectValidForAction(ObjectUseIntention action, const ObjectID& object) const;

  // Pick which object would be "best" for the robot to interact with, out of the objects which are
  // valid. This may use distance from the robot, or may be changed to do something else "reasonable" in the
  // future
  ObjectID GetBestObjectForAction(ObjectUseIntention action) const;

  // returns a pointer to the filter used for the given action, or nullptr if it doesn't exist
  const BlockWorldFilter* GetDefaultFilterForAction(ObjectUseIntention action) const;

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
  // Face tracking
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // shared logic to get a face to track which requires the least panning and tilting (based on the current
  // robot pose). If preferNamed is true, then will prefer faces with a name to those without
  Vision::FaceID_t GetBestFaceToTrack(const std::set< Vision::FaceID_t >& possibleFaces,
                                      const bool preferNamedFaces) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // This getter iterates the list of possible objects currently stored and retrieves only those that can be located in
  // current origin. Note this causes a calculation WRT origin (consider caching in the future if need to optimize)
  void GetPossibleObjectsWRTOrigin(PossibleObjectVector& possibleObjects) const;

  // set/return time at which Cozmo got off the charger by himself
  void GotOffChargerAtTime(const float time_sec) { _gotOffChargerAtTime_sec = time_sec; }
  float GetTimeAtWhichRobotGotOffCharger() const { return _gotOffChargerAtTime_sec; }
  
  // return time at which Cozmo got back on treads (negative in never recorded)
  float GetTimeAtWhichRobotReturnedToTreadsSecs() const { return _returnedToTreadsAtTime_sec; }
  
  // set/return time at which engine processed information regarding edges
  inline void SetLastEdgeInformation(const float time_sec, const float closestEdgeDist_mm);
  float GetLastEdgeInformationTime() const { return _edgeInfoTime_sec; }
  float GetLastEdgeClosestDistance() const { return _edgeInfoClosestEdge_mm; }
  
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
  // Helpers
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Initialize our list of filters for caching valid objects based on ObjectUseIntention 
  void CreateBlockWorldFilters();

  
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

  // update the best objects for each action type
  void UpdateValidObjects();

  // Common logic for checking validity of blocks for any pick up action
  bool CanPickupHelper(const ObservableObject* object);
  
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
  ObjectFailureTable _rollOrPopFailures;
  
  // time at which the robot got off the charger by itself. Negative value means never
  float _gotOffChargerAtTime_sec;
  // time at which the robot returned to being on treads (after being picked up)
  float _returnedToTreadsAtTime_sec;
  
  // time at which the engine processed edge information coming from vision
  float _edgeInfoTime_sec;
  float _edgeInfoClosestEdge_mm;
 
  // list of markers/objects we have not checked out yet
  PossibleObjectList _possibleObjects;

  // mapping from actions to the best object to use for that action
  std::map< ObjectUseIntention, ObjectID > _bestObjectForAction;

  // mapping to all valid objects
  std::map< ObjectUseIntention, std::set< ObjectID > > _validObjectsForAction;

  // also keep track of blockworld filters so we don't need to keep re-creating them
  std::map< ObjectUseIntention, std::unique_ptr< BlockWorldFilter > > _filtersPerAction;
  
  // container of beacons currently defined (high level AI concept)
  BeaconList _beacons;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Inline
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::SetLastEdgeInformation(const float time_sec, const float closestEdgeDist_mm)
{
  _edgeInfoTime_sec = time_sec;
  _edgeInfoClosestEdge_mm = closestEdgeDist_mm;
}
  

} // namespace Cozmo
} // namespace Anki

#endif //
