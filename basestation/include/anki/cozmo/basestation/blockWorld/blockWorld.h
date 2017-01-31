/**
 * File: blockWorld.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/1/2013
 *
 * Description: Defines a container for tracking the state of all objects in Cozmo's world.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef ANKI_COZMO_BLOCKWORLD_H
#define ANKI_COZMO_BLOCKWORLD_H

#include <queue>
#include <map>
#include <vector>

#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/cozmo/basestation/mat.h"
#include "anki/cozmo/basestation/namedColors/namedColors.h"
#include "anki/cozmo/basestation/overheadEdge.h"
#include "anki/common/types.h"
#include "anki/common/basestation/exceptions.h"

#include "anki/vision/basestation/observableObjectLibrary.h"

#include "clad/types/actionTypes.h"

#include <vector>

namespace Anki
{
  namespace Cozmo
  {
    // Forward declarations:
    class Robot;
    class RobotManager;
    class RobotMessageHandler;
    class ActiveObject;
    class IExternalInterface;
    class INavMemoryMap;
    namespace BlockConfigurations{
    class BlockConfigurationManager;
    }
    
    class BlockWorld
    {
    public:
      using ObservableObjectLibrary = Vision::ObservableObjectLibrary<ObservableObject>;
      constexpr static const float kOnCubeStackHeightTolerence = 2 * STACKED_HEIGHT_TOL_MM;
      
      BlockWorld(Robot* robot);
      
      void DefineCustomObject(ObjectType type, f32 xSize_mm, f32 ySize_mm, f32 zSize_mm, f32 markerWidth_mm, f32 markerHeight_mm);
      
      ~BlockWorld();
      
      // Update the BlockWorld's state by processing all queued ObservedMarkers
      // and updating robots' poses and blocks' poses from them.
      Result Update(std::list<Vision::ObservedMarker>& observedMarkers);
      
      // Adds a proximity obstacle (like random objects detected in front of the robot with the IR sensor) at the given pose.
      Result AddProxObstacle(const Pose3d& p);
      
      // Adds a cliff (detected with cliff detector)
      Result AddCliff(const Pose3d& p);
      
      // Adds a collision-based obstacle (for when we think we bumped into something)
      Result AddCollisionObstacle(const Pose3d& p);
      
      ObjectID CreateFixedCustomObject(const Pose3d& p, const f32 xSize_mm, const f32 ySize_mm, const f32 zSize_mm);
      
      // Processes the edges found in the given frame
      Result ProcessVisionOverheadEdges(const OverheadEdgeFrame& frameInfo);

      // Creates and adds an active object of the appropriate type based on factoryID to the connected objects container.
      // Note there is no information about pose, so no instance of this object in the current origin is updated.
      // However, if an object of the same type is found as an unconnected object, the objectID is inherited, and
      // the unconnected instances (in origins) become linked to this connected object instance.
      // It returns the new or inherited objectID on success, or invalid objectID if it fails.
      ObjectID AddConnectedActiveObject(ActiveID activeID, FactoryID factoryID, ActiveObjectType activeObjectType);
      // Removes connected object from the connected objects container. Returns matching objectID if found
      ObjectID RemoveConnectedActiveObject(ActiveID activeID);

      // Creates an object from the given active object type. Current API design prevents BlockWorld from setting
      // the pose, that's why we have AddLocatedObject(shared_ptr), that checks the pose and ID have already been
      // set. We should revisit this API where we need to expose this CreateActiveObject
      // Note memory management is responsibility of the calling code
      ActiveObject* CreateActiveObject(ActiveObjectType activeObjectType, ActiveID activeID, FactoryID factoryID);

      // Adds the given object to the BlockWorld according to its current objectID and pose. The objectID is expected
      // to be set, and not be currently in use in the BlockWorld. Otherwise it's a sign that something went
      // wrong matching the current BlockWorld objects
      void AddLocatedObject(const std::shared_ptr<ObservableObject>& object);
      
      // notify the blockWorld that someone is about to change the pose of an object
      void OnObjectPoseWillChange(const ObjectID& objectID, ObjectFamily family,
        const Pose3d& newPose, PoseState newPoseState);
      
      // notify the blockWorld that someone (poseConfirmer) has visually verified the given object at their current pose
      void OnObjectVisuallyVerified(const ObservableObject* object);
      
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Object Access
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      // Delete located instances will delete object instances of both active and passive objects. However
      // from connected objects, only the located instances are affected. The unlocated instances, that are stored
      // regardless of pose, are not affected by this. Passive objects don't have connected instances.
      void DeleteAllLocatedObjects();                              // all instances in all origins
      void DeleteLocatedObjectsByOrigin(const PoseOrigin* origin); // all instances in the given origin
      void DeleteLocatedObjectsByFamily(ObjectFamily family);      // all instances of the given family in all origins
      void DeleteLocatedObjectsByType(ObjectType type);            // all instances of the given type in all origins
      void DeleteLocatedObjectsByID(const ObjectID& withID);       // all instances of the given object in all origins
      void DeleteLocatedObjectByIDInCurOrigin(const ObjectID& withID); // the given instance in the current origin
      
      // Clear the object from shared uses, like localization, selection or carrying, etc. So that it can be removed
      // without those system lingering
      void ClearLocatedObjectByIDInCurOrigin(const ObjectID& withID);
      void ClearLocatedObject(ObservableObject* object);
      
      // Get objects that exist in the world, by family, type, ID, etc.
      // NOTE: Like IDs, object types are unique across objects so they can be
      //       used without specifying which family.
      inline const ObservableObjectLibrary& GetObjectLibrary(ObjectFamily whichFamily) const;
      
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Get objects by ID or activeID
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      
      // Return a pointer to an object with the specified ID in the current world coordinate frame.
      // If that object does not exist in the current origin, nullptr is returned.
      // If you want an object regardless of its pose, use GetConnectedActiveObjectById instead.
      // For more complex queries, use one of the Find methods with a BlockWorldFilter.
      inline const ObservableObject* GetLocatedObjectByID(const ObjectID& objectID, ObjectFamily family=ObjectFamily::Invalid) const;
      inline       ObservableObject* GetLocatedObjectByID(const ObjectID& objectID, ObjectFamily family=ObjectFamily::Invalid);
      
      // Returns a pointer to a connected object with the specified objectID without any pose information. If you need to obtain
      // the instance of this object in the current origin (if it exists), you can do so with GetLocatedObjectByID
      inline const ActiveObject* GetConnectedActiveObjectByID(const ObjectID& objectID) const;
      inline       ActiveObject* GetConnectedActiveObjectByID(const ObjectID& objectID);
      
      // Returns a pointer to a connected object with the specified objectID without any pose information. If you need to obtain
      // the instance of this object in the current origin (if it exists), you can do so with GetLocatedObjectByID
      inline const ActiveObject* GetConnectedActiveObjectByActiveID(const ActiveID& activeID) const;
      inline       ActiveObject* GetConnectedActiveObjectByActiveID(const ActiveID& activeID);

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Find connected objects by filter query
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      
      // Returns first object matching filter, among objects that are currently connected (regardless of pose)
      // Note OriginMode in the filter is ignored (TODO: provide ConnectedFilter API without origin)
      inline const ActiveObject* FindConnectedActiveMatchingObject(const BlockWorldFilter& filter) const;
      inline       ActiveObject* FindConnectedActiveMatchingObject(const BlockWorldFilter& filter);

      // Returns (in arguments) all objects matching a filter, among objects that are currently connected (regardless
      // of pose). Note OriginMode in the filter is ignored (TODO: provide ConnectedFilter API without origin)
      // NOTE: does not clear result (thus can be used multiple times with the same vector)
      void FindConnectedActiveMatchingObjects(const BlockWorldFilter& filter, std::vector<const ActiveObject*>& result) const;
      void FindConnectedActiveMatchingObjects(const BlockWorldFilter& filter, std::vector<ActiveObject*>& result);
      
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Find objects by filter query
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      
      // Applies given filter or modifier to located objects.
      // NOTE: Calling FilterLocatedObjects this way doesn't return anything, so the only
      //  way it has any effect is via the filter's FilterFcn, which presumably
      //  is doing useful work for you somehow. Otherwise what are you doing?
      using ModifierFcn = std::function<void(ObservableObject*)>;
      inline void FilterLocatedObjects(const BlockWorldFilter& filter) const;
      inline void ModifyLocatedObjects(const ModifierFcn& modiferFcn, const BlockWorldFilter& filter = BlockWorldFilter());
      
      // Returns (in arguments) all objects matching a filter, among objects that are currently located (their pose
      // is valid in the origins matching the filter)
      // NOTE: does not clear result (thus can be used multiple times with the same vector)
      void FindLocatedMatchingObjects(const BlockWorldFilter& filter, std::vector<const ObservableObject*>& result) const;
      void FindLocatedMatchingObjects(const BlockWorldFilter& filter, std::vector<ObservableObject*>& result);
      
      // Returns first object matching filter, among objects that are currently located (their pose is valid in
      // the origins matching the filter)
      inline const ObservableObject* FindLocatedMatchingObject(const BlockWorldFilter& filter) const;
      inline       ObservableObject* FindLocatedMatchingObject(const BlockWorldFilter& filter);
      
      // Finds an object closer than the given distance (optional) (rotation -- not implemented yet) with respect
      // to the given pose. Returns nullptr if no objects match, closest one if multiple matches are found.
      inline const ObservableObject* FindLocatedObjectClosestTo(const Pose3d& pose,
                                                                const BlockWorldFilter& filter = BlockWorldFilter()) const;
      inline       ObservableObject* FindLocatedObjectClosestTo(const Pose3d& pose,
                                                                const BlockWorldFilter& filter = BlockWorldFilter());
      inline const ObservableObject* FindLocatedObjectClosestTo(const Pose3d& pose,
                                                                const Vec3f& distThreshold,
                                                                const BlockWorldFilter& filter = BlockWorldFilter()) const;
      inline       ObservableObject* FindLocatedObjectClosestTo(const Pose3d& pose,
                                                                const Vec3f& distThreshold,
                                                                const BlockWorldFilter& filter = BlockWorldFilter());
      
      // Finds a matching object (one with the same family and type) that is closest to the given object, within the
      // specified distance and angle thresholds.
      // Returns nullptr if none found.
      inline const ObservableObject* FindLocatedClosestMatchingObject(const ObservableObject& object,
                                                                      const Vec3f& distThreshold,
                                                                      const Radians& angleThreshold,
                                                                      const BlockWorldFilter& filter = BlockWorldFilter()) const;
      inline       ObservableObject* FindLocatedClosestMatchingObject(const ObservableObject& object,
                                                                      const Vec3f& distThreshold,
                                                                      const Radians& angleThreshold,
                                                                      const BlockWorldFilter& filter = BlockWorldFilter());
      
      // Finds the object of the given type that is closest to the given pose, within the specified distance and
      // angle thresholds.
      // Returns nullptr if none found.
      inline const ObservableObject* FindLocatedClosestMatchingObject(ObjectType withType,
                                                                      const Pose3d& pose,
                                                                      const Vec3f& distThreshold,
                                                                      const Radians& angleThreshold,
                                                                      const BlockWorldFilter& filter = BlockWorldFilter()) const;
      inline       ObservableObject* FindLocatedClosestMatchingObject(ObjectType withType,
                                                                      const Pose3d& pose,
                                                                      const Vec3f& distThreshold,
                                                                      const Radians& angleThreshold,
                                                                      const BlockWorldFilter& filter = BlockWorldFilter());
      
      //
      const ObservableObject* FindMostRecentlyObservedObject(const BlockWorldFilter& filter = BlockWorldFilter()) const;
      
      // Finds existing objects whose XY bounding boxes intersect with objectSeen's XY bounding box, and pass
      // the given filter.
      void FindLocatedIntersectingObjects(const ObservableObject* objectSeen,
                                          std::vector<const ObservableObject*>& intersectingExistingObjects,
                                          f32 padding_mm,
                                          const BlockWorldFilter& filter = BlockWorldFilter()) const;
      void FindLocatedIntersectingObjects(const ObservableObject* objectSeen,
                                          std::vector<ObservableObject*>& intersectingExistingObjects,
                                          f32 padding_mm,
                                          const BlockWorldFilter& filter = BlockWorldFilter());
      // same as above, except it takes Quad2f instead of an object
      void FindLocatedIntersectingObjects(const Quad2f& quad,
                                          std::vector<const ObservableObject*> &intersectingExistingObjects,
                                          f32 padding_mm,
                                          const BlockWorldFilter& filter = BlockWorldFilter()) const;
      void FindLocatedIntersectingObjects(const Quad2f& quad,
                                          std::vector<ObservableObject*> &intersectingExistingObjects,
                                          f32 padding_mm,
                                          const BlockWorldFilter& filter = BlockWorldFilter());
      
      // Find an object on top of the given object, using a given height tolerance
      // between the top of the given object on bottom and the bottom of existing
      // candidate objects on top. Returns nullptr if no object is found.
      inline const ObservableObject* FindObjectOnTopOf(const ObservableObject& objectOnBottom,
                                                       f32 zTolerance,
                                                       const BlockWorldFilter& filter = BlockWorldFilter()) const;
      inline       ObservableObject* FindObjectOnTopOf(const ObservableObject& objectOnBottom,
                                                       f32 zTolerance,
                                                       const BlockWorldFilter& filter = BlockWorldFilter());
      // Similar to FindObjectOnTopOf, but in reverse: find object directly underneath given object
      inline const ObservableObject* FindObjectUnderneath(const ObservableObject& objectOnTop,
                                                          f32 zTolerance,
                                                          const BlockWorldFilter& filterIn = BlockWorldFilter()) const;
      inline       ObservableObject* FindObjectUnderneath(const ObservableObject& objectOnTop,
                                                          f32 zTolerance,
                                                          const BlockWorldFilter& filterIn = BlockWorldFilter());
      
      
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // BoundingBoxes
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      
      // Finds all blocks in the world whose centers are within the specified
      // heights off the ground (z dimension, relative to world origin!) and
      // returns a vector of quads of their outlines on the ground plane (z=0).
      // Can also pad the bounding boxes by a specified amount.
      // Optionally, will filter according to given BlockWorldFilter.
      void GetLocatedObjectBoundingBoxesXY(const f32 minHeight, const f32 maxHeight, const f32 padding,
                                           std::vector<std::pair<Quad2f,ObjectID> >& boundingBoxes,
                                           const BlockWorldFilter& filter = BlockWorldFilter()) const;

      // Wrapper for GetLocatedObjectBoundingBoxesXY that returns bounding boxes of objects that are
      // obstacles given the robot's current z height.
      void GetObstacles(std::vector<std::pair<Quad2f,ObjectID> >& boundingBoxes,
                        const f32 padding = 0.f) const;
      
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Localization
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      
      // returns true if the given origin is a zombie origin. A zombie origin means that no active objects are currently
      // in that origin/frame, which would make it impossible to relocalize to any other origin. Note that current origin
      // is not a zombie even if it doesn't have any cubes yet.
      bool IsZombiePoseOrigin(const Pose3d* origin) const;
      
      // Returns true if there are remaining objects that the robot could potentially
      // localize to
      bool AnyRemainingLocalizableObjects() const;
      
      // returns true if there are localizable objects at the specified origin. It iterates all localizable objects
      // and returns true if any of them has the given origin as origin
      bool AnyRemainingLocalizableObjects(const Pose3d* origin) const;

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Others. TODO Categorize/organize
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      
      // Returns true if any blocks were moved, added, or deleted on the
      // last call to Update().
      bool DidObjectsChange() const;
      // Gets the timestamp of the last robot msg when objects changed
      const TimeStamp_t& GetTimeOfLastChange() const;
      
      // Get/Set currently-selected object
      ObjectID GetSelectedObject() const { return _selectedObject; }
      void     CycleSelectedObject();
      
      // Try to select the object with the specified ID. Return true if that
      // object ID is found and the object is successfully selected.
      bool SelectObject(const ObjectID& objectID);
      void DeselectCurrentObject();
      
      void EnableObjectDeletion(bool enable);
      void EnableObjectAddition(bool enable);
      
      // Find all objects with the given parent and update them to have flatten
      // their objects w.r.t. the origin. Call this when the robot rejiggers
      // origins.
      Result UpdateObjectOrigins(const Pose3d* oldOrigin, const Pose3d* newOrigin);
      
      // Find the given objectID in the given origin, and update it so that it is
      // stored according to its _current_ origin. (Move from old origin to current origin.)
      // If the origin is already correct, nothing changes. If the objectID is not
      // found in the given origin, RESULT_FAIL is returned.
      Result UpdateObjectOrigin(const ObjectID& objectID, const Pose3d* oldOrigin);
      
      // checks the origins currently storing objects and if they have become zombies it deletes them
      void DeleteObjectsFromZombieOrigins();
      
      size_t GetNumAliveOrigins() const { return _locatedObjects.size(); }
      
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Navigation memory
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      // return pointer to current INavMemoryMap (it may be null if not enabled)
      const INavMemoryMap* GetNavMemoryMap() const;
      INavMemoryMap* GetNavMemoryMap();
      
      // update memory map with the current robot pose if needed (objects use other notification methods)
      void UpdateRobotPoseInMemoryMap();
      
      // flag all interesting edges in front of the robot (using ground plane ROI) as uncertain, meaning we want
      // the robot to grab new edges since we don't trust the ones we currently have in front of us
      void FlagGroundPlaneROIInterestingEdgesAsUncertain();
      
      // flags any interesting edges in the given quad as not interesting anymore. Quad should be passed wrt current origin
      void FlagQuadAsNotInterestingEdges(const Quad2f& quadWRTOrigin);
      
      // flags all current interesting edges as too small to give useful information
      void FlagInterestingEdgesAsUseless();

      // create a new memory map from current robot frame of reference. The pointer is used as an identifier
      void CreateLocalizedMemoryMap(const Pose3d* worldOriginPtr);
      
      // Visualize the navigation memory information
      void DrawNavMemoryMap() const;
      
      // Send navigation memory map (e.g. so SDK can get the data)
      void BroadcastNavMemoryMap();
      
      //
      // Visualization
      //

      // Call every existing object's Visualize() method and call the
      // VisualizePreActionPoses() on the currently-selected ActionableObject.
      void DrawAllObjects() const;
      
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Navigation memory
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      
      // template for all events we subscribe to
      template<typename T>
      void HandleMessage(const T& msg);
      
      BlockConfigurations::BlockConfigurationManager& GetBlockConfigurationManager() { assert(_blockConfigurationManager); return *_blockConfigurationManager;}
      const BlockConfigurations::BlockConfigurationManager& GetBlockConfigurationManager() const { assert(_blockConfigurationManager); return *_blockConfigurationManager;}
      void NotifyBlockConfigurationManagerObjectPoseChanged(const ObjectID& objectID) const;
      
    private:

      // active objects
      using ActiveObjectsMapByID_t     = std::map<ObjectID, std::shared_ptr<ActiveObject> >;
      using ActiveObjectsMapByType_t   = std::map<ObjectType, ActiveObjectsMapByID_t >;
      using ActiveObjectsMapByFamily_t = std::map<ObjectFamily, ActiveObjectsMapByType_t>;

      // observable objects
      using ObjectsMapByID_t     = std::map<ObjectID, std::shared_ptr<ObservableObject> >;
      using ObjectsMapByType_t   = std::map<ObjectType, ObjectsMapByID_t >;
      using ObjectsMapByFamily_t = std::map<ObjectFamily, ObjectsMapByType_t>;
      using ObjectsByOrigin_t    = std::map<const PoseOrigin*, ObjectsMapByFamily_t>;
      
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Helpers for accessors and queries
      // Note: these helpers return non-const pointers despite being marked const,
      // because they are private helpers wrapped by const/non-const public methods.
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      // Located by filter (most basic, other helpers rely on it)
      ObservableObject* FindLocatedObjectHelper(const BlockWorldFilter& filter,
                                                const ModifierFcn& modiferFcn = nullptr,
                                                bool returnFirstFound = false) const;
      
      // Connected by filter (most basic, other helpers rely on it)
      ActiveObject* FindConnectedObjectHelper(const BlockWorldFilter& filter,
                                              const ModifierFcn& modiferFcn = nullptr,
                                              bool returnFirstFound = false) const;

      // By ID or activeID
      ObservableObject* GetLocatedObjectByIdHelper(const ObjectID& objectID, ObjectFamily family) const;
      ActiveObject* GetConnectedActiveObjectByIdHelper(const ObjectID& objectID) const;
      ActiveObject* GetConnectedActiveObjectByActiveIdHelper(const ActiveID& activeID) const;
      
      // By location/pose
      ObservableObject* FindLocatedObjectClosestToHelper(const Pose3d& pose,
                                                         const Vec3f&  distThreshold,
                                                         const BlockWorldFilter& filterIn) const;

      // Matching object
      ObservableObject* FindLocatedClosestMatchingObjectHelper(const ObservableObject& object,
                                                               const Vec3f& distThreshold,
                                                               const Radians& angleThreshold,
                                                               const BlockWorldFilter& filter) const;
      
      // Matching type and location
      ObservableObject* FindLocatedClosestMatchingTypeHelper(ObjectType withType,
                                                             const Pose3d& pose,
                                                             const Vec3f& distThreshold,
                                                             const Radians& angleThreshold,
                                                             const BlockWorldFilter& filter) const;
      

      ObservableObject* FindObjectOnTopOrUnderneathHelper(const ObservableObject& referenceObject,
                                                          f32 zTolerance,
                                                          const BlockWorldFilter& filterIn,
                                                          bool onTop) const;
      
      
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      //
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      
      Result UpdateObjectPoses(std::list<Vision::ObservedMarker>& obsMarkers,
                               const ObjectFamily& inFamily,
                               const TimeStamp_t atTimestamp);
      
      Result UpdateMarkerlessObjects(TimeStamp_t atTimestamp);
      
      /*
      // Adds/Removes proxObstacles based on current sensor readings and age of existing proxObstacles
      Result UpdateProxObstaclePoses();
      */

      // Finds existing objects that overlap with and are of the same type as objectSeen,
      // where overlap is defined by the IsSameAs() function.
      // NOTE: these populate return vectors with non-const ObservableObject pointers
      //       even though the methods are marked const, but these are protected helpers
      void FindOverlappingObjects(const ObservableObject* objectSeen,
                                  const ObjectsMapByType_t& objectsExisting,
                                  std::vector<ObservableObject*>& overlappingExistingObjects) const;
      
      void FindOverlappingObjects(const ObservableObject* objectExisting,
                                  const std::vector<ObservableObject*>& objectsSeen,
                                  std::vector<ObservableObject*>& overlappingSeenObjects) const;
      
      void FindOverlappingObjects(const ObservableObject* objectExisting,
                                  const std::multimap<f32, ObservableObject*>& objectsSeen,
                                  std::vector<ObservableObject*>& overlappingSeenObjects) const;
      
      // Helper for removing markers that are inside other detected markers
      static void RemoveMarkersWithinMarkers(std::list<Vision::ObservedMarker>& currentObsMarkers);
      
      // 1. Looks for objects that should have been seen (markers should have been visible
      //    but something was seen through/behind their last known location) and delete
      //    them.
      // 2. Looks for objects whose markers are not visible but which still have
      //    a corner in the camera's field of view, so the _object_ is technically
      //    still visible. Return the number of these.
      u32 CheckForUnobservedObjects(TimeStamp_t atTimestamp);
      
      // Checks whether an object is unobserved and in collision with the robot,
      // for use in filtering objects to mark them as dirty
      bool CheckForCollisionWithRobot(const ObservableObject* object) const;
      
//      // Adds a new object based on its origin/family/type. Its ID will be assigned
//      // if it isn't already, or it will be copied from objectToCopyID if that object
//      // is not null.
//      ObjectID AddNewObject(const std::shared_ptr<ObservableObject>& object,
//                            const ObservableObject* objectToCopyID = nullptr);
//      
//      ObjectID AddNewObject(ObjectsMapByType_t& existingFamily,
//                            const std::shared_ptr<ObservableObject>& object,
//                            const ObservableObject* objectToCopyID = nullptr);
      
      // NOTE: this function takes control over the passed-in ObservableObject*'s and
      //  will either directly add them to BlockWorld's existing objects or delete them
      //  if/when they are no longer needed.
      Result AddAndUpdateObjects(const std::multimap<f32, ObservableObject*>& objectsSeen,
                                 const ObjectFamily& inFamily,
                                 const TimeStamp_t atTimestamp);
      
      // Updates poses of stacks of objects by finding the difference between old object
      // poses and applying that to the new observed poses
      void UpdatePoseOfStackedObjects();
      
      // Remove all posekey-marker pairs from the map if marker is marked used
      void RemoveUsedMarkers(std::list<Vision::ObservedMarker>& poseKeyObsMarkerMap);

      // adds a markerless object at the given pose
      Result AddMarkerlessObject(const Pose3d& pose, ObjectType type);
      
      // Clear the object from shared uses, like localization, selection or carrying, etc. So that it can be removed
      // without those system lingering
      void ClearLocatedObjectHelper(ObservableObject* object);
      
      // Delete an object when you have a direct iterator pointing to it. Returns
      // the iterator to the next object in the container.
      ObjectsMapByID_t::iterator DeleteLocatedObjectAt(const ObjectsMapByID_t::iterator objIter,
                                                       const ObjectType&    withType,
                                                       const ObjectFamily&  fromFamily);
      
      Result BroadcastObjectObservation(const ObservableObject* observedObject) const;

      // broadcast currently located objects (in current origin)
      void BroadcastLocatedObjectStates();
      // broadcast currently connected objects (regardless of pose states in any origin)
      void BroadcastConnectedObjects();
      
      void SetupEventHandlers(IExternalInterface& externalInterface);
      
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Nav memory map
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      
      // remove current renders for all maps if any
      void ClearNavMemoryMapRender() const;
      
      // enable/disable rendering of the memory maps
      void SetMemoryMapRenderEnabled(bool enabled);
      
      // updates the objects reported in curOrigin that are moving to the relocalizedOrigin by virtue of rejiggering
      void UpdateOriginsOfObjectsReportedInMemMap(const Pose3d* curOrigin, const Pose3d* relocalizedOrigin);
      
      // clear the space in the memory map between the robot and observed markers for the given object,
      // because if we saw the marker, it means there's nothing between us and the marker.
      // The observed markers are obtained querying the current marker observation time
      void ClearRobotToMarkersInMemMap(const ObservableObject* object);
      
      // add/remove the given object to/from the memory map
      void AddObjectReportToMemMap(const ObservableObject& object, const Pose3d& newPose);
      void RemoveObjectReportFromMemMap(const ObservableObject& object, const Pose3d* origin);
      
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Vision border detection
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      
      // adds edges from the given frame to the world info
      Result AddVisionOverheadEdges(const OverheadEdgeFrame& frameInfo);
      
      // review interesting edges within the given quad and decide whether they are still interesting
      void ReviewInterestingEdges(const Quad2f& withinQuad);
      
      //
      // Member Variables
      //
      
      Robot*             _robot;
      
      f32 _lastPlayAreaSizeEventSec;
      const f32 _playAreaSizeEventIntervalSec;
      
      // Store all known observable objects (these are everything we know about,
      // separated by class of object, not necessarily what we've actually seen
      // yet, but what everything we are aware of)
      std::map<ObjectFamily, ObservableObjectLibrary> _objectLibrary;
      
      // Objects that we know about because they have connected, but for which we may or may not know their location.
      // The instances of objects in this container are expected to NEVER have a valid Pose/PoseState. If they are
      // present in any origin, a copy of the object with the proper pose will be placed in the located objects container.
      // Note: by definition it stores ActiveObjects instead of ObservableObjects
      ActiveObjectsMapByFamily_t _connectedObjects;

      // Objects that we have located indexed by the origin they belong to.
      // The instances of objects in this container are expected to always have a valid Pose/PoseState. If they are
      // lost from an origin (for example by being unobserved), their master copy should be available through the
      // connected objects container.
      ObjectsByOrigin_t _locatedObjects;
      
      bool _didObjectsChange;
      TimeStamp_t _robotMsgTimeStampAtChange; // time of the last robot msg when objects changed
      
      ObjectID _selectedObject;

      // For tracking, keep track of the id of the actions we are doing
      u32 _lastTrackingActionTag = static_cast<u32>(ActionConstants::INVALID_TAG);
      
      // Map the world knows the robot has traveled
      using NavMemoryMapTable = std::map<const Pose3d*, std::unique_ptr<INavMemoryMap>>;
      NavMemoryMapTable _navMemoryMaps;
      const Pose3d* _currentNavMemoryMapOrigin;
      bool _isNavMemoryMapRenderEnabled;
      
      // poses we have sent to the memory map for objects we know, in each origin
      struct PoseInMapInfo {
        PoseInMapInfo(const Pose3d& p, bool inMap) : pose(p), isInMap(inMap) {}
        PoseInMapInfo() : pose(), isInMap(false) {}
        Pose3d pose;
        bool isInMap; // if true the pose was sent to the map, if false the pose was removed from the map
      };
      using OriginToPoseInMapInfo = std::map<const PoseOrigin*, PoseInMapInfo>;
      using ObjectIdToPosesPerOrigin = std::map<int, OriginToPoseInMapInfo>;
      ObjectIdToPosesPerOrigin _navMapReportedPoses;
      Pose3d _navMapReportedRobotPose;

      // changes in poses in the current frame to keep stacks aligned
      struct PoseChange {
        PoseChange(const ObjectID& id, const Pose3d& oldPose, const PoseState oldPoseState) :
          _id(id), _oldPose(oldPose), _oldPoseState(oldPoseState) {}
        const ObjectID _id;
        const Pose3d _oldPose;
        const PoseState _oldPoseState;
      };
      std::list<PoseChange> _objectPoseChangeList;  // changes registered (within an update tick)
      bool _trackPoseChanges;               // whether we want to register pose changes
      
      std::set<ObjectID> _unidentifiedActiveObjects;
      
      std::vector<Signal::SmartHandle> _eventHandles;
      
      float _memoryMapBroadcastRate_sec = -1.0f;      // (Negative means don't send)
      float _nextMemoryMapBroadcastTimeStamp = 0.0f;  // The next time we should broadcast
      
      TimeStamp_t _currentObservedMarkerTimestamp = 0;
      std::unique_ptr<BlockConfigurations::BlockConfigurationManager> _blockConfigurationManager;

      
    }; // class BlockWorld

    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    const BlockWorld::ObservableObjectLibrary& BlockWorld::GetObjectLibrary(ObjectFamily whichFamily) const
    {
      auto objectsWithFamilyIter = _objectLibrary.find(whichFamily);
      if(objectsWithFamilyIter != _objectLibrary.end()) {
        return objectsWithFamilyIter->second;
      } else {
        static const ObservableObjectLibrary EmptyObjectLibrary;
        return EmptyObjectLibrary;
      }
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    const ObservableObject* BlockWorld::GetLocatedObjectByID(const ObjectID& objectID, ObjectFamily family) const {
      return GetLocatedObjectByIdHelper(objectID, family); // returns const*
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ObservableObject* BlockWorld::GetLocatedObjectByID(const ObjectID& objectID, ObjectFamily family) {
      return GetLocatedObjectByIdHelper(objectID, family); // returns non-const*
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    const ActiveObject* BlockWorld::GetConnectedActiveObjectByID(const ObjectID& objectID) const {
      return GetConnectedActiveObjectByIdHelper(objectID);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ActiveObject* BlockWorld::GetConnectedActiveObjectByID(const ObjectID& objectID) {
      return GetConnectedActiveObjectByIdHelper(objectID);
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    const ActiveObject* BlockWorld::GetConnectedActiveObjectByActiveID(const ActiveID& activeID) const {
      return GetConnectedActiveObjectByActiveIdHelper(activeID);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ActiveObject* BlockWorld::GetConnectedActiveObjectByActiveID(const ActiveID& activeID)
    {
      return GetConnectedActiveObjectByActiveIdHelper(activeID);
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    const ActiveObject* BlockWorld::FindConnectedActiveMatchingObject(const BlockWorldFilter& filter) const
    {
      return FindConnectedObjectHelper(filter); // returns non-const
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ActiveObject* BlockWorld::FindConnectedActiveMatchingObject(const BlockWorldFilter& filter)
    {
      return FindConnectedObjectHelper(filter); // returns non-const
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void BlockWorld::FilterLocatedObjects(const BlockWorldFilter& filter) const
    {
      FindLocatedObjectHelper(filter, nullptr, false);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void BlockWorld::ModifyLocatedObjects(const ModifierFcn& modifierFcn, const BlockWorldFilter& filter)
    {
      if(modifierFcn == nullptr) {
        PRINT_NAMED_WARNING("BlockWorld.ModifyLocatedObjects.NullModifierFcn", "Consider just using FilterLocatedObjects?");
      }
      FindLocatedObjectHelper(filter, modifierFcn, false);
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    const ObservableObject* BlockWorld::FindLocatedMatchingObject(const BlockWorldFilter& filter) const
    {
      return FindLocatedObjectHelper(filter, nullptr, true); // returns const
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ObservableObject* BlockWorld::FindLocatedMatchingObject(const BlockWorldFilter& filter)
    {
      return FindLocatedObjectHelper(filter, nullptr, true); // returns non-const
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    const ObservableObject* BlockWorld::FindLocatedObjectClosestTo(const Pose3d& pose,
                                                                   const BlockWorldFilter& filter) const
    {
      return FindLocatedObjectClosestTo(pose, Vec3f{FLT_MAX}, filter); // returns const
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ObservableObject* BlockWorld::FindLocatedObjectClosestTo(const Pose3d& pose,
                                                             const BlockWorldFilter& filter)
    {
      return FindLocatedObjectClosestTo(pose, Vec3f{FLT_MAX}, filter); // returns non-const
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    const ObservableObject* BlockWorld::FindLocatedObjectClosestTo(const Pose3d& pose,
                                                                   const Vec3f&  distThreshold,
                                                                   const BlockWorldFilter& filter) const
    {
      return FindLocatedObjectClosestToHelper(pose, distThreshold, filter); // returns const
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ObservableObject* BlockWorld::FindLocatedObjectClosestTo(const Pose3d& pose,
                                                             const Vec3f&  distThreshold,
                                                             const BlockWorldFilter& filter)
    {
      return FindLocatedObjectClosestToHelper(pose, distThreshold, filter); // returns non-const
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    const ObservableObject* BlockWorld::FindLocatedClosestMatchingObject(const ObservableObject& object,
                                                                         const Vec3f& distThreshold,
                                                                         const Radians& angleThreshold,
                                                                        const BlockWorldFilter& filter) const
    {
      return FindLocatedClosestMatchingObjectHelper(object, distThreshold, angleThreshold, filter);
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ObservableObject* BlockWorld::FindLocatedClosestMatchingObject(const ObservableObject& object,
                                                                    const Vec3f& distThreshold,
                                                                    const Radians& angleThreshold,
                                                                    const BlockWorldFilter& filter)
    {
      return FindLocatedClosestMatchingObjectHelper(object, distThreshold, angleThreshold, filter);
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    const ObservableObject* BlockWorld::FindLocatedClosestMatchingObject(ObjectType withType,
                                                                         const Pose3d& pose,
                                                                         const Vec3f& distThreshold,
                                                                         const Radians& angleThreshold,
                                                                         const BlockWorldFilter& filter) const
    {
      return FindLocatedClosestMatchingTypeHelper(withType, pose, distThreshold, angleThreshold, filter);
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ObservableObject* BlockWorld::FindLocatedClosestMatchingObject(ObjectType withType,
                                                                   const Pose3d& pose,
                                                                   const Vec3f& distThreshold,
                                                                   const Radians& angleThreshold,
                                                                   const BlockWorldFilter& filter)
    {
      return FindLocatedClosestMatchingTypeHelper(withType, pose, distThreshold, angleThreshold, filter);
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    const ObservableObject* BlockWorld::FindObjectOnTopOf(const ObservableObject& objectOnBottom,
                                                          f32 zTolerance,
                                                          const BlockWorldFilter& filter) const
    {
      return FindObjectOnTopOrUnderneathHelper(objectOnBottom, zTolerance, filter, true); // returns const
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ObservableObject* BlockWorld::FindObjectOnTopOf(const ObservableObject& objectOnBottom,
                                                    f32 zTolerance,
                                                    const BlockWorldFilter& filter)
    {
      return FindObjectOnTopOrUnderneathHelper(objectOnBottom, zTolerance, filter, true); // returns non-const
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    const ObservableObject* BlockWorld::FindObjectUnderneath(const ObservableObject& objectOnTop,
                                                             f32 zTolerance,
                                                             const BlockWorldFilter& filter) const
    {
      return FindObjectOnTopOrUnderneathHelper(objectOnTop, zTolerance, filter, false); // returns const
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ObservableObject* BlockWorld::FindObjectUnderneath(const ObservableObject& objectOnTop,
                                                       f32 zTolerance,
                                                       const BlockWorldFilter& filter)
    {
      return FindObjectOnTopOrUnderneathHelper(objectOnTop, zTolerance, filter, false); // returns non-const
    }
    
    
    
//    inline ObjectID BlockWorld::AddNewObject(const std::shared_ptr<ObservableObject>& object,
//                                             const ObservableObject* objectToCopyID)
//
//    {
//      return AddNewObject(_locatedObjects[&object->GetPose().FindOrigin()][object->GetFamily()], object, objectToCopyID);
//    }
    
    
  } // namespace Cozmo
} // namespace Anki



#endif // ANKI_COZMO_BLOCKWORLD_H
