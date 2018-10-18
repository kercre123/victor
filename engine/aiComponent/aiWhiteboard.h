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

#include "engine/aiComponent/aiComponents_fwd.h"
#include "engine/externalInterface/externalInterface_fwd.h"

#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/objectIDs.h"
#include "coretech/vision/engine/faceIdTypes.h"

#include "clad/types/objectFamilies.h"
#include "clad/types/objectTypes.h"
#include "clad/types/behaviorComponent/postBehaviorSuggestions.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <list>
#include <queue>
#include <set>
#include <vector>
#include <unordered_map>

namespace Anki {
namespace Vector {

// Forward declarations
class ObservableObject;
class Robot;
class SayNameProbabilityTable;
class SmartFaceID;
enum class OnboardingStages : uint8_t;

namespace DefaultFailToUseParams {
constexpr static const float kTimeObjectInvalidAfterFailure_sec = 30.f;
constexpr static const float kObjectInvalidAfterFailureRadius_mm = 60.f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AIWhiteboard
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class AIWhiteboard : public IDependencyManagedComponent<AIComponentID>, 
                     private Util::noncopyable
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
  
  // object usage reason for failure
  enum class ObjectActionFailure {
    PickUpObject,   // pick up object from location
    StackOnObject,  // stack on top of object
    PlaceObjectAt,   // place object at location
    RollOrPopAWheelie // roll or pop a wheelie on a block
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IDependencyManagedComponent<AIComponentID> functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  virtual void InitDependent(Vector::Robot* robot, 
                             const AICompMap& dependentComps) override;

  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor
  AIWhiteboard(Robot& robot);

  virtual ~AIWhiteboard();

  // IDependencyManagedComponent<AIComponentID> functions
  virtual void UpdateDependent(const AICompMap& dependentComps) override;
  // end IDependencyManagedComponent<AIComponentID> functions
  
  // what to do when the robot is delocalized
  void OnRobotDelocalized();
  // what to do when the robot relocalizes to a cube
  void OnRobotRelocalized();
  // what to do when the robot wakes up (e.g. reset things)
  void OnRobotWakeUp();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Possible Objects
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // NB: BN 2018 - I'm not sure if this is still functional, it may be, but it's old Cozmo code
  
  // called when Cozmo can identify a clear quad (no borders, obstacles, etc)
  void ProcessClearQuad(const Quad2f& quad);

  // called when we've searched for a possible object at a given pose, but failed to find it
  void FinishedSearchForPossibleCubeAtPose(ObjectType objectType, const Pose3d& pose);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Cube search
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // BN: this code is not really needed anymore in it's full form but it's still used in a few places so left
  // it here for now. It was really designed for a multi-cube world where it was important to figure out which
  // cubes were working (from which angles)
  
  // notify the whiteboard that we just failed to use this object.
  // uses object's current location
  void SetFailedToUse(const ObservableObject& object, ObjectActionFailure actionFailure);
  // specify location manually - should only be used for PlaceObjectAt
  void SetFailedToUse(const ObservableObject& object, ObjectActionFailure actionFailure, const Pose3d& atLocation);

  // returns true if someone reported a failure to use the given object (by ID), less than the specified seconds ago
  // close to the given location with the given reason(s).
  // recentSecs: use negative for any time at all, 0 for failed this tick, positive for failed less than X secs ago
  // atPose: where to compare
  // distThreshold_mm: set to negative to not compare poses, set to 0 or positive for atPose or around by X distance
  // angleThreshold: set to M_PI for any rotation, set to anything else for rotation difference with atPose's rotation
  // see versions:
  // if passed onto DidFailToUse, it will try to find a match for any object
  static constexpr const int ANY_OBJECT = -1;
  // any failure for given object
  bool DidFailToUse(const int objectID, ObjectActionFailure reason) const;
  // any recent failure for given object
  bool DidFailToUse(const int objectID, ObjectActionFailure reason, float recentSecs) const;
  // any failure for given object at given pose
  bool DidFailToUse(const int objectID, ObjectActionFailure reason,
                    const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const;
  // any recent failure for given object at given pose
  bool DidFailToUse(const int objectID, ObjectActionFailure reason, float recentSecs,
                    const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const;

  // same as above, with multiple reasons considered at the same time. Returns true if there is a failure for
  // _any_ of the specified reasons
  using FailureReasonsContainer = std::set< ObjectActionFailure >;
  bool DidFailToUse(const int objectID, FailureReasonsContainer reasons) const;
  bool DidFailToUse(const int objectID, FailureReasonsContainer reasons, float recentSecs) const;
  bool DidFailToUse(const int objectID, FailureReasonsContainer reasons,
                    const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const;
  bool DidFailToUse(const int objectID, FailureReasonsContainer reasons,
                    float recentSecs, const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const;
  
  

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // This getter iterates the list of possible objects currently stored and retrieves only those that can be located in
  // current origin. Note this causes a calculation WRT origin (consider caching in the future if need to optimize)
  void GetPossibleObjectsWRTOrigin(PossibleObjectVector& possibleObjects) const;
  
  // set/return time at which engine processed information regarding edges
  void SetLastEdgeInformation(const float closestEdgeDist_mm);
  float GetLastEdgeInformationTime() const { return _edgeInfoTime_sec; }
  float GetLastEdgeClosestDistance() const { return _edgeInfoClosestEdge_mm; }

  // decide whether or not to say a name
  inline std::shared_ptr<SayNameProbabilityTable>& GetSayNameProbabilityTable() { return _sayNameProbTable; }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Victor observing demo state (may eventually become part of victor freeplay)
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // feeding state
  bool Victor_HasCubeToEat() const { return _victor_cubeToEat.IsSet(); }
  const ObjectID& Victor_GetCubeToEat() const { return _victor_cubeToEat; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Post behavior suggestions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // write a post-behavior suggestion
  void OfferPostBehaviorSuggestion( const PostBehaviorSuggestions& suggestion );
  
  // returns true if suggestion has been offered, and sets the of the last tick it was offered if so
  bool GetPostBehaviorSuggestion( const PostBehaviorSuggestions& suggestion, size_t& tick ) const;
  
  // clears the post behavior suggestions
  void ClearPostBehaviorSuggestions();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Events
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // template for all events we subscribe to
  template<typename T>
  void HandleMessage(const T& msg);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Onboarding
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  OnboardingStages GetCurrentOnboardingStage() { return _onboardingStage; }
  void SetMostRecentOnboardingStage(OnboardingStages stage) { _onboardingStage = stage; } // doesn't save or broadcast

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
  const ObjectFailureTable& GetObjectFailureTable(ObjectActionFailure action) const;
  ObjectFailureTable& GetObjectFailureTable(ObjectActionFailure action);
  
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
    
  // time at which the engine processed edge information coming from vision
  float _edgeInfoTime_sec;
  float _edgeInfoClosestEdge_mm;
 
  // list of markers/objects we have not checked out yet
  PossibleObjectList _possibleObjects;
  
  ObjectID _victor_cubeToEat;
  
  std::unordered_map<PostBehaviorSuggestions, size_t> _postBehaviorSuggestions;
  
  // holds the current onboarding stage to avoid having to listen for it or read it from disk
  OnboardingStages _onboardingStage;
  
  std::shared_ptr<SayNameProbabilityTable> _sayNameProbTable;
};
  

} // namespace Vector
} // namespace Anki

#endif //
