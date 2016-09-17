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

#include "anki/cozmo/basestation/activeCube.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/blockWorldFilter.h"
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
    class ActiveCube;
    class IExternalInterface;
    class INavMemoryMap;
    
    class BlockWorld
    {
    public:
      using ObservableObjectLibrary = Vision::ObservableObjectLibrary<ObservableObject>;
      
      BlockWorld(Robot* robot);
      
      void DefineCustomObject(ObjectType type, f32 xSize_mm, f32 ySize_mm, f32 zSize_mm, f32 markerWidth_mm, f32 markerHeight_mm);
      
      ~BlockWorld();
      
      // Update the BlockWorld's state by processing all queued ObservedMarkers
      // and updating robots' poses and blocks' poses from them.
      Result Update();
      
      // Empties the queue of all observed markers
      void ClearAllObservedMarkers();
      
      Result QueueObservedMarker(HistPoseKey& poseKey, Vision::ObservedMarker& marker);

      // Adds a proximity obstacle (like random objects detected in front of the robot with the IR sensor) at the given pose.
      Result AddProxObstacle(const Pose3d& p);
      
      // Adds a cliff (detected with cliff detector)
      Result AddCliff(const Pose3d& p);
      
      // Adds a collision-based obstacle (for when we think we bumped into something)
      Result AddCollisionObstacle(const Pose3d& p);
      
      // Processes the edges found in the given frame
      Result ProcessVisionOverheadEdges(const OverheadEdgeFrame& frameInfo);
      
      // Adds an active object of the appropriate type based on factoryID at
      // an unknown pose. To be used when the active object first comes into radio contact.
      // This function does nothing if an active object of the same type with the active ID already exists.
      // If objToCopyID is not null then the new object will have the same id as objToCopyID
      ObjectID AddActiveObject(ActiveID activeID,
                               FactoryID factoryID,
                               ActiveObjectType activeObjectType,
                               const ObservableObject* objToCopyId = nullptr);
      
      // notify the blockWorld that someone has changed the pose of an object
      void OnObjectPoseChanged(const ObjectID& objectID, ObjectFamily family,
        const Pose3d& newPose, PoseState newPoseState);
      
      // returns true if the given origin is a zombie origin. A zombie origin means that no active objects are currently
      // in that origin/frame, which would make it impossible to relocalize to any other origin. Note that current origin
      // is not a zombie even if it doesn't have any cubes yet.
      bool IsZombiePoseOrigin(const Pose3d* origin) const;
      
      //
      // Object Access
      //
      
      // Clearing objects: all, by type, by family, or by ID.
      // NOTE: Clearing does not _delete_ an object; it marks its pose as unknown.
      void ClearAllExistingObjects();
      void ClearObjectsByFamily(const ObjectFamily family);
      void ClearObjectsByType(const ObjectType type);
      bool ClearObject(const ObjectID& withID); // Returns true if object with ID is found and cleared, false otherwise.
      bool ClearObject(ObservableObject* object);
      

      // First clears the object and then actually deletes it, removing it from
      // BlockWorld entirely.
      void DeleteAllExistingObjects();
      bool DeleteObject(const ObjectID& withID);
      bool DeleteObject(ObservableObject* object);
      void DeleteObjectsByOrigin(const Pose3d* origin, bool clearFirst); // can disable clearing before deletion
      void DeleteObjectsByFamily(const ObjectFamily family);
      void DeleteObjectsByType(const ObjectType type);
      
      // Get objects that exist in the world, by family, type, ID, etc.
      // NOTE: Like IDs, object types are unique across objects so they can be
      //       used without specifying which family.
      const ObservableObjectLibrary& GetObjectLibrary(ObjectFamily whichFamily) const;
      
      // Return a pointer to an object with the specified ID (in the current world
      // coordinate frame. If that object does not exist or its pose is unknown, nullptr is returned.
      // Be sure to ALWAYS check for the return being null!
      // Optionally, specify a family to search within.
      // For more complex queries, use one of the Find methods with a BlockWorldFilter.
      ObservableObject* GetObjectByID(const ObjectID& objectID, ObjectFamily inFamily = ObjectFamily::Unknown);
      const ObservableObject* GetObjectByID(const ObjectID& objectID, ObjectFamily inFamily = ObjectFamily::Unknown) const;
      
      // Like GetObjectByID but dynamically casts the given object ID into the
      // ActiveObject type.
      ActiveObject* GetActiveObjectByID(const ObjectID& objectID, ObjectFamily inFamily = ObjectFamily::Unknown);
      const ActiveObject* GetActiveObjectByID(const ObjectID& objectID, ObjectFamily inFamily = ObjectFamily::Unknown) const;
      
      // Returns (in arguments) all objects matching a filter
      // NOTE: does not clear result (thus can be used multiple times with the same vector)
      void FindMatchingObjects(const BlockWorldFilter& filter, std::vector<const ObservableObject*>& result) const;
      void FindMatchingObjects(const BlockWorldFilter& filter, std::vector<ObservableObject*>& result);
      
      // Returns first object matching filter
      const ObservableObject* FindMatchingObject(const BlockWorldFilter& filter) const;
      ObservableObject* FindMatchingObject(const BlockWorldFilter& filter);
      
      // Applies given filter or modifier to all objects.
      // NOTE: Calling FilterObjects this way doesn't return anything, so the only
      //  way it has any effect is via the filter's FilterFcn, which presumably
      //  is doing useful work for you somehow. Otherwise what are you doing?
      using ModifierFcn = std::function<void(ObservableObject*)>;
      void FilterObjects(const BlockWorldFilter& filter) const;
      void ModifyObjects(const ModifierFcn& modiferFcn, const BlockWorldFilter& filter = BlockWorldFilter());
      
      // Finds all blocks in the world whose centers are within the specified
      // heights off the ground (z dimension, relative to world origin!) and
      // returns a vector of quads of their outlines on the ground plane (z=0).
      // Can also pad the bounding boxes by a specified amount.
      // Optionally, will filter according to given BlockWorldFilter.
      void GetObjectBoundingBoxesXY(const f32 minHeight,
                                    const f32 maxHeight,
                                    const f32 padding,
                                    std::vector<std::pair<Quad2f,ObjectID> >& boundingBoxes,
                                    const BlockWorldFilter& filter = BlockWorldFilter()) const;
      
      // Finds an object nearest the specified distance (and optionally, rotation -- not implemented yet)
      // of the given pose. Returns nullptr if no objects match. Returns closest
      // if multiple matches are found.
      const ObservableObject* FindObjectClosestTo(const Pose3d& pose,
                                                  const BlockWorldFilter& filter = BlockWorldFilter()) const;

      const ObservableObject* FindObjectClosestTo(const Pose3d& pose,
                                                  const Vec3f&  distThreshold,
                                                  const BlockWorldFilter& filter = BlockWorldFilter()) const;
      
      ObservableObject* FindObjectClosestTo(const Pose3d& pose,
                                                  const BlockWorldFilter& filter = BlockWorldFilter());
      
      ObservableObject* FindObjectClosestTo(const Pose3d& pose,
                                                  const Vec3f&  distThreshold,
                                                  const BlockWorldFilter& filter = BlockWorldFilter());
      
      // Finds a matching object (one with the same type) that is closest to the
      // given object, within the specified distance and angle thresholds.
      // Returns nullptr if none found.
      const ObservableObject* FindClosestMatchingObject(const ObservableObject& object,
                                                        const Vec3f& distThreshold,
                                                        const Radians& angleThreshold,
                                                        const BlockWorldFilter& filter = BlockWorldFilter()) const;
      
      ObservableObject* FindClosestMatchingObject(const ObservableObject& object,
                                                  const Vec3f& distThreshold,
                                                  const Radians& angleThreshold,
                                                  const BlockWorldFilter& filter = BlockWorldFilter());
      
      // Same as above, except type and pose are specified directly
      const ObservableObject* FindClosestMatchingObject(ObjectType withType,
                                                        const Pose3d& pose,
                                                        const Vec3f& distThreshold,
                                                        const Radians& angleThreshold,
                                                        const BlockWorldFilter& filter = BlockWorldFilter()) const;
      
      ObservableObject* FindClosestMatchingObject(ObjectType withType,
                                                  const Pose3d& pose,
                                                  const Vec3f& distThreshold,
                                                  const Radians& angleThreshold,
                                                  const BlockWorldFilter& filter = BlockWorldFilter());
      
      const ObservableObject* FindMostRecentlyObservedObject(const BlockWorldFilter& filter = BlockWorldFilter()) const;
      
      // Finds existing objects whose XY bounding boxes intersect with objectSeen's
      // XY bounding box, with the exception of those that are of ignoreFamilies or
      // ignoreTypes.
      void FindIntersectingObjects(const ObservableObject* objectSeen,
                                   std::vector<const ObservableObject*>& intersectingExistingObjects,
                                   f32 padding_mm,
                                   const BlockWorldFilter& filter = BlockWorldFilter()) const;
      
      void FindIntersectingObjects(const Quad2f& quad,
                                   std::vector<const ObservableObject*> &intersectingExistingObjects,
                                   f32 padding,
                                   const BlockWorldFilter& filter = BlockWorldFilter()) const;
      
      void FindIntersectingObjects(const ObservableObject* objectSeen,
                                   std::vector<ObservableObject*>& intersectingExistingObjects,
                                   f32 padding_mm,
                                   const BlockWorldFilter& filter = BlockWorldFilter());
      
      void FindIntersectingObjects(const Quad2f& quad,
                                   std::vector<ObservableObject*> &intersectingExistingObjects,
                                   f32 padding,
                                   const BlockWorldFilter& filter = BlockWorldFilter());
      
      // Returns true if there are remaining objects that the robot could potentially
      // localize to
      bool AnyRemainingLocalizableObjects() const;
      
      // returns true if there are localizable objects at the specified origin. It iterates all localizable objects
      // and returns true if any of them has the given origin as origin
      bool AnyRemainingLocalizableObjects(const Pose3d* origin) const;
      
      // Find an object on top of the given object, using a given height tolerance
      // between the top of the given object on bottom and the bottom of existing
      // candidate objects on top. Returns nullptr if no object is found.
      const ObservableObject* FindObjectOnTopOf(const ObservableObject& objectOnBottom,
                                                f32 zTolerance,
                                                const BlockWorldFilter& filter = BlockWorldFilter()) const;
      
      ObservableObject* FindObjectOnTopOf(const ObservableObject& objectOnBottom,
                                          f32 zTolerance,
                                          const BlockWorldFilter& filter = BlockWorldFilter());
      
      // Same as above, but in revers: find object directly underneath given object
      const ObservableObject* FindObjectUnderneath(const ObservableObject& objectOnTop,
                                                   f32 zTolerance,
                                                   const BlockWorldFilter& filterIn = BlockWorldFilter()) const;
      
      ObservableObject* FindObjectUnderneath(const ObservableObject& objectOnTop,
                                             f32 zTolerance,
                                             const BlockWorldFilter& filterIn = BlockWorldFilter());
      
      
      //Returns the height of the tallest stack of blocks in block world
      //and sets the bottomBlockID that was passed in
      //If there are multiple stacks of equivelent height it returns the nearest stack
      //Pass in a list of bottom blocks to ignore if you are looking to locate a specific stack
      uint8_t GetTallestStack(ObjectID& bottomBlockID) const;
      uint8_t GetTallestStack(ObjectID& bottomBlockID, const std::vector<ObjectID>& blocksToIgnore) const;
      
      // Wrapper for above that returns bounding boxes of objects that are
      // obstacles given the robot's current z height. Objects being carried
      // and the object the robot is localized to are not considered obstacles.
      void GetObstacles(std::vector<std::pair<Quad2f,ObjectID> >& boundingBoxes,
                        const f32 padding = 0.f) const;
      
      // Get objects newly-observed or re-observed objects in the last Update.
      /*
      using ObservedObjectBoundingBoxes = std::vector<std::pair<ObjectID, Rectangle<f32> > >;
      const ObservedObjectBoundingBoxes& GetProjectedObservedObjects() const;
      const std::vector<ObjectID>& GetObservedObjectIDs() const;
      */
      
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
      Result UpdateObjectOrigins(const Pose3d* oldOrigin,
                                 const Pose3d* newOrigin);
      
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
      
      //
      // Visualization
      //

      void EnableDraw(bool on);

      // Visualize markers in image display
      void DrawObsMarkers() const;
      
      // Call every existing object's Visualize() method and call the
      // VisualizePreActionPoses() on the currently-selected ActionableObject.
      void DrawAllObjects() const;

      
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Navigation memory
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      
      // template for all events we subscribe to
      template<typename T>
      void HandleMessage(const T& msg);
      
    protected:
      
      using ObjectsMapByID_t     = std::map<ObjectID, std::shared_ptr<ObservableObject> >;
      using ObjectsMapByType_t   = std::map<ObjectType, ObjectsMapByID_t >;
      using ObjectsMapByFamily_t = std::map<ObjectFamily, ObjectsMapByType_t>;
      
      // Typedefs / Aliases
      //using ObsMarkerContainer_t = std::multiset<Vision::ObservedMarker, Vision::ObservedMarker::Sorter()>;
      //using ObsMarkerList_t = std::list<Vision::ObservedMarker>;
      using PoseKeyObsMarkerMap_t = std::multimap<HistPoseKey, Vision::ObservedMarker>;
      using ObsMarkerListMap_t = std::map<TimeStamp_t, PoseKeyObsMarkerMap_t>;
      
      //
      // Member Methods
      //
      
      // Note these are marked const but return non-const pointers.
      ObservableObject* GetObjectByIdHelper(const ObjectID& objectID, ObjectFamily inFamily) const;
      ActiveObject* GetActiveObjectByIdHelper(const ObjectID& objectID, ObjectFamily inFamily) const;
      
      bool UpdateRobotPose(PoseKeyObsMarkerMap_t& obsMarkers, const TimeStamp_t atTimestamp);
      
      Result UpdateObjectPoses(PoseKeyObsMarkerMap_t& obsMarkersAtTimestamp,
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
      static void RemoveMarkersWithinMarkers(PoseKeyObsMarkerMap_t& currentObsMarkers);
      
      // 1. Looks for objects that should have been seen (markers should have been visible
      //    but something was seen through/behind their last known location) and delete
      //    them.
      // 2. Looks for objects whose markers are not visible but which still have
      //    a corner in the camera's field of view, so the _object_ is technically
      //    still visible. Return the number of these.
      u32 CheckForUnobservedObjects(TimeStamp_t atTimestamp);
      
      // Helpers for actually inserting a new object into a new family using
      // its type and ID. Object's ID will be set if it isn't already.
      void AddNewObject(const std::shared_ptr<ObservableObject>& object);
      void AddNewObject(ObjectsMapByType_t& existingFamily, const std::shared_ptr<ObservableObject>& object);
      
      // NOTE: this function takes control over the passed-in ObservableObject*'s and
      //  will either directly add them to BlockWorld's existing objects or delete them
      //  if/when they are no longer needed.
      Result AddAndUpdateObjects(const std::multimap<f32, ObservableObject*>& objectsSeen,
                                 const ObjectFamily& inFamily,
                                 const TimeStamp_t atTimestamp);
      
      // Updates poses of stacks of objects by finding the difference between old object
      // poses and applying that to the new observed poses. Used when an object's
      // pose is updated to correct anything stacked on top of it to match.
      void UpdateRotationOfObjectsStackedOn(const ObservableObject* existingObjectOnBottom,
                                            ObservableObject* objSeen);
      
      
      // Remove all posekey-marker pairs from the map if marker is marked used
      void RemoveUsedMarkers(PoseKeyObsMarkerMap_t& poseKeyObsMarkerMap);

      // adds a markerless object at the given pose
      Result AddMarkerlessObject(const Pose3d& pose, ObjectType type);
      
      ObjectID CreateFixedCustomObject(const Pose3d& p, const f32 xSize_mm, const f32 ySize_mm, const f32 zSize_mm);
      
      // Generates a list of ObservedMarker pointers that reference the actual ObservedMarkers
      // stored in poseKeyObsMarkerMap
      void GetObsMarkerList(const PoseKeyObsMarkerMap_t& poseKeyObsMarkerMap,
                            std::list<Vision::ObservedMarker*>& lst);
      
      void ClearObjectHelper(ObservableObject* object);
      
      // Delete an object when you have a direct iterator pointing to it. Returns
      // the iterator to the next object in the container.
      ObjectsMapByID_t::iterator DeleteObject(const ObjectsMapByID_t::iterator objIter,
                                              const ObjectType&    withType,
                                              const ObjectFamily&  fromFamily);
      
      Result BroadcastObjectObservation(const ObservableObject* observedObject) const;
      
      // Use inOrigin=nullptr to use objects from all coordinate frames
      void BroadcastAvailableObjects(bool connectedObjectsOnly, const Pose3d* inOrigin);
      
      // Note: these helpers return non-const pointers despite being marked const,
      // but that's because they are protected helpers wrapped by const/non-const
      // public methods:
      ObservableObject* FindObjectHelper(const BlockWorldFilter& filter = BlockWorldFilter(),
                                         const ModifierFcn& modiferFcn = nullptr,
                                         bool returnFirstFound = false) const;
      
      ObservableObject* FindObjectOnTopOrUnderneathHelper(const ObservableObject& referenceObject,
                                                f32 zTolerance,
                                                          const BlockWorldFilter& filterIn,
                                                          bool onTop) const;
      
      ObservableObject* FindObjectClosestToHelper(const Pose3d& pose,
                                                  const Vec3f&  distThreshold,
                                                  const BlockWorldFilter& filterIn) const;
      
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
      
      ObsMarkerListMap_t _obsMarkers;
      f32 _lastPlayAreaSizeEventSec;
      const f32 _playAreaSizeEventIntervalSec;
      
      // A place to keep up with all objects' IDs and bounding boxes observed
      // in a single call to Update()
      //ObservedObjectBoundingBoxes _obsProjectedObjects;
      //std::vector<ObjectID> _currentObservedObjectIDs;
      
      // Store all known observable objects (these are everything we know about,
      // separated by class of object, not necessarily what we've actually seen
      // yet, but what everything we are aware of)
      std::map<ObjectFamily, ObservableObjectLibrary> _objectLibrary;
      
      // Store all observed objects, indexed first by Type, then by ID
      // NOTE: If a new ObjectsMap_t is added here, a pointer to it needs to
      //   be stored in allExistingObjects_ (below), initialized in the
      //   BlockWorld cosntructor.
      using ObjectsByOrigin_t = std::map<const Pose3d*, ObjectsMapByFamily_t>;
      ObjectsByOrigin_t _existingObjects;
      
      bool _didObjectsChange;
      TimeStamp_t _robotMsgTimeStampAtChange; // time of the last robot msg when objects changed
      bool _canDeleteObjects;
      bool _canAddObjects;
      
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
      
      // For allowing the calling of VizManager draw functions
      bool _enableDraw;
      
      std::set<ObjectID> _unidentifiedActiveObjects;
      
      std::vector<Signal::SmartHandle> _eventHandles;
      
      // Contains the list of added/updated objects from the last Update()
      std::list<ObservableObject*> _currentObservedObjects;
      
    }; // class BlockWorld

    
    inline const BlockWorld::ObservableObjectLibrary& BlockWorld::GetObjectLibrary(ObjectFamily whichFamily) const
    {
      auto objectsWithFamilyIter = _objectLibrary.find(whichFamily);
      if(objectsWithFamilyIter != _objectLibrary.end()) {
        return objectsWithFamilyIter->second;
      } else {
        static const ObservableObjectLibrary EmptyObjectLibrary;
        return EmptyObjectLibrary;
      }
    }
    
    inline ObservableObject* BlockWorld::GetObjectByID(const ObjectID& objectID, ObjectFamily inFamily) {
      return GetObjectByIdHelper(objectID, inFamily); // returns non-const*
    }
    
    inline const ObservableObject* BlockWorld::GetObjectByID(const ObjectID& objectID, ObjectFamily inFamily) const {
      return GetObjectByIdHelper(objectID, inFamily); // returns const*
    }
    
    inline ActiveObject* BlockWorld::GetActiveObjectByID(const ObjectID& objectID, ObjectFamily inFamily) {
      return GetActiveObjectByIdHelper(objectID, inFamily); // returns non-const*
    }
    
    inline const ActiveObject* BlockWorld::GetActiveObjectByID(const ObjectID& objectID, ObjectFamily inFamily) const {
      return GetActiveObjectByIdHelper(objectID, inFamily); // returns const*
    }
    
    inline void BlockWorld::AddNewObject(const std::shared_ptr<ObservableObject>& object)
    {
      AddNewObject(_existingObjects[&object->GetPose().FindOrigin()][object->GetFamily()], object);
    }

    /*
    inline const BlockWorld::ObservedObjectBoundingBoxes& BlockWorld::GetProjectedObservedObjects() const
    {
      return _obsProjectedObjects;
    }
     */
    
    /*
    inline const std::vector<ObjectID>& BlockWorld::GetObservedObjectIDs() const {
      return _currentObservedObjectIDs;
    }
     */
    
    inline void BlockWorld::EnableObjectAddition(bool enable) {
      _canAddObjects = enable;
    }
    
    inline void BlockWorld::EnableObjectDeletion(bool enable) {
      _canDeleteObjects = enable;
    }
    
    inline void BlockWorld::ModifyObjects(const ModifierFcn& modifierFcn, const BlockWorldFilter& filter) {
      if(modifierFcn == nullptr) {
        PRINT_NAMED_WARNING("BlockWorld.ModifyObjects.NullModifierFcn",
                            "Consider just using FilterObjects?");
      }
      FindObjectHelper(filter, modifierFcn, false);
    }
    
    inline void BlockWorld::FilterObjects(const BlockWorldFilter& filter) const {
      FindObjectHelper(filter, nullptr, false);
    }
    
    inline const ObservableObject* BlockWorld::FindObjectOnTopOf(const ObservableObject& objectOnBottom,
                                                                 f32 zTolerance,
                                                                 const BlockWorldFilter& filter) const
    {
      return FindObjectOnTopOrUnderneathHelper(objectOnBottom, zTolerance, filter, true); // returns const
    }
    
    inline ObservableObject* BlockWorld::FindObjectOnTopOf(const ObservableObject& objectOnBottom,
                                                           f32 zTolerance,
                                                           const BlockWorldFilter& filter)
    {
      return FindObjectOnTopOrUnderneathHelper(objectOnBottom, zTolerance, filter, true); // returns non-const
    }
    
    inline const ObservableObject* BlockWorld::FindObjectUnderneath(const ObservableObject& objectOnTop,
                                                                 f32 zTolerance,
                                                                 const BlockWorldFilter& filter) const
    {
      return FindObjectOnTopOrUnderneathHelper(objectOnTop, zTolerance, filter, false); // returns const
    }
    
    inline ObservableObject* BlockWorld::FindObjectUnderneath(const ObservableObject& objectOnTop,
                                                              f32 zTolerance,
                                                              const BlockWorldFilter& filter)
    {
      return FindObjectOnTopOrUnderneathHelper(objectOnTop, zTolerance, filter, false); // returns non-const
    }
    
    
    inline const ObservableObject* BlockWorld::FindObjectClosestTo(const Pose3d& pose,
                                                                   const BlockWorldFilter& filter) const
    {
      return FindObjectClosestTo(pose, Vec3f{FLT_MAX}, filter); // returns const
    }
    
    inline ObservableObject* BlockWorld::FindObjectClosestTo(const Pose3d& pose,
                                                             const BlockWorldFilter& filter)
    {
      return FindObjectClosestTo(pose, Vec3f{FLT_MAX}, filter); // returns non-const
    }
    
    inline const ObservableObject* BlockWorld::FindObjectClosestTo(const Pose3d& pose,
                                                                   const Vec3f&  distThreshold,
                                                                   const BlockWorldFilter& filter) const
    {
      return FindObjectClosestToHelper(pose, distThreshold, filter); // returns const
    }
    
    inline ObservableObject* BlockWorld::FindObjectClosestTo(const Pose3d& pose,
                                                             const Vec3f&  distThreshold,
                                                             const BlockWorldFilter& filter)
    {
      return FindObjectClosestToHelper(pose, distThreshold, filter); // returns non-const
    }
    
    inline const ObservableObject* BlockWorld::FindMatchingObject(const BlockWorldFilter& filter) const
    {
      return FindObjectHelper(filter, nullptr, true); // returns const
    }
    
    inline ObservableObject* BlockWorld::FindMatchingObject(const BlockWorldFilter& filter)
    {
      return FindObjectHelper(filter, nullptr, true); // returns non-const
    }


    
  } // namespace Cozmo
} // namespace Anki



#endif // ANKI_COZMO_BLOCKWORLD_H
