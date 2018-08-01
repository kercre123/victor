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

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/block.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/robotComponents_fwd.h"
#include "engine/mat.h"
#include "engine/namedColors/namedColors.h"
#include "engine/overheadEdge.h"
#include "coretech/common/engine/exceptions.h"
#include "coretech/common/engine/robotTimeStamp.h"

#include "coretech/vision/engine/observableObjectLibrary.h"

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
    class INavMap;
    class MapComponent;
    
    namespace ExternalInterface {
      struct RobotDeletedLocatedObject;
      struct RobotObservedObject;
    }
    
    // BlockWorld is updated at the robot component level, same as BehaviorComponent
    // Therefore BCComponents (which are managed by BehaviorComponent) can't declare dependencies on BlockWorld 
    // since when it's Init/Update relative to BehaviorComponent must be declared by BehaviorComponent explicitly, 
    // not by individual components within BehaviorComponent
    class BlockWorld : public UnreliableComponent<BCComponentID>, 
                       public IDependencyManagedComponent<RobotComponentID>
    {
    public:
      using ObservableObjectLibrary = Vision::ObservableObjectLibrary<ObservableObject>;
      constexpr static const float kOnCubeStackHeightTolerance = 2 * STACKED_HEIGHT_TOL_MM;
      
      BlockWorld();
            
      ~BlockWorld();

      //////
      // IDependencyManagedComponent functions
      //////
      virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComps) override final;
      virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
        dependencies.insert(RobotComponentID::CozmoContextWrapper);
      };
      virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
        dependencies.insert(RobotComponentID::CubeComms);
        dependencies.insert(RobotComponentID::Vision);
      };

      // Prevent hiding function warnings by exposing the (valid) unreliable component methods
      using UnreliableComponent<BCComponentID>::InitDependent;
      using UnreliableComponent<BCComponentID>::GetInitDependencies;
      using UnreliableComponent<BCComponentID>::GetUpdateDependencies;
      //////
      // end IDependencyManagedComponent functions
      //////

      
      // Update the BlockWorld's state by processing all queued ObservedMarkers
      // and updating robot's and objects' poses from them.
      Result UpdateObservedMarkers(const std::list<Vision::ObservedMarker>& observedMarkers);
      
      // Adds a collision-based obstacle (for when we think we bumped into something)
      Result AddCollisionObstacle(const Pose3d& p);
      
      ObjectID CreateFixedCustomObject(const Pose3d& p, const f32 xSize_mm, const f32 ySize_mm, const f32 zSize_mm);


      // Defines an object that could be observed later.
      // Does not add an instance of this object to the existing objects in the world.
      // Instead, provides the definition of an object that could be instantiated based on observations.
      Result DefineObject(std::unique_ptr<const ObservableObject>&& object);

      // Creates and adds an active object of the appropriate type based on factoryID to the connected objects container.
      // Note there is no information about pose, so no instance of this object in the current origin is updated.
      // However, if an object of the same type is found as an unconnected object, the objectID is inherited, and
      // the unconnected instances (in origins) become linked to this connected object instance.
      // It returns the new or inherited objectID on success, or invalid objectID if it fails.
      ObjectID AddConnectedActiveObject(ActiveID activeID, FactoryID factoryID, ObjectType objectType);
      // Removes connected object from the connected objects container. Returns matching objectID if found
      ObjectID RemoveConnectedActiveObject(ActiveID activeID);

      // Adds the given object to the BlockWorld according to its current objectID and pose. The objectID is expected
      // to be set, and not be currently in use in the BlockWorld. Otherwise it's a sign that something went
      // wrong matching the current BlockWorld objects
      void AddLocatedObject(const std::shared_ptr<ObservableObject>& object);
      
      // notify the blockWorld that someone changed the pose of an object. Note the object may have been destroyed,
      // if the poseState changes to PoseState::Invalid. In that case, the object received by parameter is a copy
      // of the object with its pose set Invalid and not accessible
      void OnObjectPoseChanged(const ObservableObject& object, const Pose3d* oldPose, PoseState oldPoseState);

      // Called when robot gets delocalized in order to do internal bookkeeping and broadcast updated object states
      void OnRobotDelocalized(PoseOriginID_t newWorldOriginID);
         
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Object Access
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      // Delete located instances will delete object instances of both active and passive objects. However
      // from connected objects, only the located instances are affected. The unlocated instances, that are stored
      // regardless of pose, are not affected by this. Passive objects don't have connected instances.
      void DeleteLocatedObjects(const BlockWorldFilter& filter);   // objects that pass the filter will be deleted
      
      // Delete all objects that intersect with the provided quad
      void DeleteIntersectingObjects(const Quad2f& quad,
                                     f32 padding_mm,
                                     const BlockWorldFilter& filter = BlockWorldFilter());
                                     
      void DeleteIntersectingObjects(const std::shared_ptr<ObservableObject>& object,
                                     f32 padding_mm,
                                     const BlockWorldFilter& filter = BlockWorldFilter());
      
      // Clear the object from shared uses, like localization, selection or carrying, etc. So that it can be removed
      // without those system lingering
      void ClearLocatedObjectByIDInCurOrigin(const ObjectID& withID);
      void ClearLocatedObject(ObservableObject* object);
      
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
      inline const ObservableObject* FindLocatedObjectOnTopOf(const ObservableObject& objectOnBottom,
                                                              f32 zTolerance,
                                                              const BlockWorldFilter& filter = BlockWorldFilter()) const;
      inline       ObservableObject* FindLocatedObjectOnTopOf(const ObservableObject& objectOnBottom,
                                                              f32 zTolerance,
                                                              const BlockWorldFilter& filter = BlockWorldFilter());
      // Similar to FindLocatedObjectOnTopOf, but in reverse: find object directly underneath given object
      inline const ObservableObject* FindLocatedObjectUnderneath(const ObservableObject& objectOnTop,
                                                                 f32 zTolerance,
                                                                 const BlockWorldFilter& filterIn = BlockWorldFilter()) const;
      inline       ObservableObject* FindLocatedObjectUnderneath(const ObservableObject& objectOnTop,
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
      bool IsZombiePoseOrigin(PoseOriginID_t originID) const;
      
      // Returns true if there are remaining objects that the robot could potentially
      // localize to
      bool AnyRemainingLocalizableObjects() const;
      
      // returns true if there are localizable objects at the specified origin. It iterates all localizable objects
      // and returns true if any of them has the given origin as origin
      bool AnyRemainingLocalizableObjects(PoseOriginID_t origin) const;

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Others. TODO Categorize/organize
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      
      // Returns true if any blocks were moved, added, or deleted on the
      // last call to Update().
      bool DidObjectsChange() const;
      // Gets the timestamp of the last robot msg when objects changed
      const RobotTimeStamp_t& GetTimeOfLastChange() const;
      
      // Get/Set currently-selected object
      ObjectID GetSelectedObject() const { return _selectedObjectID; }
      void     CycleSelectedObject();
      
      // Try to select the object with the specified ID. Return true if that
      // object ID is found and the object is successfully selected.
      bool SelectObject(const ObjectID& objectID);
      void DeselectCurrentObject();

      // Find all objects with the given parent and update them to have flatten
      // their objects w.r.t. the origin. Call this when the robot rejiggers
      // origins.
      Result UpdateObjectOrigins(PoseOriginID_t oldOriginID, PoseOriginID_t newOriginID);
      
      // Find the given objectID in the given origin, and update it so that it is
      // stored according to its _current_ origin. (Move from old origin to current origin.)
      // If the origin is already correct, nothing changes. If the objectID is not
      // found in the given origin, RESULT_FAIL is returned.
      Result UpdateObjectOrigin(const ObjectID& objectID, PoseOriginID_t oldOriginID);
      
      // checks the origins currently storing objects and if they have become zombies it deletes them
      void DeleteObjectsFromZombieOrigins();
      
      size_t GetNumAliveOrigins() const { return _locatedObjects.size(); }
      
      // Returns the number of defined ObjectTypes for a given ObjectFamily
      size_t GetNumDefinedObjects(const ObjectFamily& objectFamily) const {
        auto objectTypeCount = _definedObjectTypeCount.find(objectFamily);
        if ( objectTypeCount != _definedObjectTypeCount.end())
        {
          return objectTypeCount->second;
        }
        return 0;
      }
      
      //
      // Visualization
      //

      // Call every existing object's Visualize() method and call the
      // VisualizePreActionPoses() on the currently-selected ActionableObject.
      void DrawAllObjects() const;
      
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Messages
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      
      // template for all events we subscribe to
      template<typename T>
      void HandleMessage(const T& msg);

    private:

      // active objects
      using ActiveObjectsMapByID_t     = std::map<ObjectID, std::shared_ptr<ActiveObject> >;
      using ActiveObjectsMapByType_t   = std::map<ObjectType, ActiveObjectsMapByID_t >;
      using ActiveObjectsMapByFamily_t = std::map<ObjectFamily, ActiveObjectsMapByType_t>;

      // observable objects
      using ObjectsMapByID_t     = std::map<ObjectID, std::shared_ptr<ObservableObject> >;
      using ObjectsMapByType_t   = std::map<ObjectType, ObjectsMapByID_t >;
      using ObjectsMapByFamily_t = std::map<ObjectFamily, ObjectsMapByType_t>;
      using ObjectsByOrigin_t    = std::map<PoseOriginID_t, ObjectsMapByFamily_t>;
      
      // defined objects
      using DefinedObjectsMapCountByFamily_t = std::map<ObjectFamily, size_t>;
      
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
      
      Result UpdateMarkerlessObjects(RobotTimeStamp_t atTimestamp);
      
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
      
      // Looks for objects that should have been seen (markers should have been visible
      // but something was seen through/behind their last known location) and delete
      // them.
      void CheckForUnobservedObjects(RobotTimeStamp_t atTimestamp);
      
      // Checks whether an object is unobserved and in collision with the robot,
      // for use in filtering objects to mark them as dirty
      bool CheckForCollisionWithRobot(const ObservableObject* object) const;
      
      // NOTE: this function takes control over the passed-in ObservableObject*'s and
      //  will either directly add them to BlockWorld's existing objects or delete them
      //  if/when they are no longer needed.
      Result AddAndUpdateObjects(const std::multimap<f32, ObservableObject*>& objectsSeen,
                                 const RobotTimeStamp_t atTimestamp);
      
      // Updates poses of stacks of objects by finding the difference between old object
      // poses and applying that to the new observed poses
      void UpdatePoseOfStackedObjects();
      
      // adds a markerless object at the given pose
      Result AddMarkerlessObject(const Pose3d& pose, ObjectType type);
      
      // Clear the object from shared uses, like localization, selection or carrying, etc. So that it can be removed
      // without those system lingering
      void ClearLocatedObjectHelper(ObservableObject* object);
            
      Result BroadcastObjectObservation(const ObservableObject* observedObject) const;

      // broadcast currently located objects (in current origin)
      void BroadcastLocatedObjectStates();
      // broadcast currently connected objects (regardless of pose states in any origin)
      void BroadcastConnectedObjects();
      
      void SetupEventHandlers(IExternalInterface& externalInterface);
      
      Result SanityCheckBookkeeping() const;
      
      void SendObjectUpdateToWebViz( const ExternalInterface::RobotDeletedLocatedObject& msg ) const;
      void SendObjectUpdateToWebViz( const ExternalInterface::RobotObservedObject& msg ) const;
      
      //
      // Member Variables
      //
      
      Robot*             _robot = nullptr;
      
      f32 _lastPlayAreaSizeEventSec;
      const f32 _playAreaSizeEventIntervalSec;
      
      // Store all known observable objects (these are everything we know about,
      // separated by class of object, not necessarily what we've actually seen
      // yet, but what everything we are aware of)
      ObservableObjectLibrary _objectLibrary;
      
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
      
      // Number of object types we have defined for each family
      // We are keeping track of the number of defined ObjectTypes for each ObjectFamily here instead of in ObjectLibrary
      // because the ObjectLibrary has no concept of ObjectFamily. If we were to get this info from ObjectLibrary directly
      // we would have to iterate over the list every time, so it's faster to keep a count when (un)defining objects.
      DefinedObjectsMapCountByFamily_t _definedObjectTypeCount;
      
      bool _didObjectsChange;
      RobotTimeStamp_t _robotMsgTimeStampAtChange; // time of the last robot msg when objects changed
      
      ObjectID _selectedObjectID;

      // For tracking, keep track of the id of the actions we are doing
      u32 _lastTrackingActionTag = static_cast<u32>(ActionConstants::INVALID_TAG);

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
            
      RobotTimeStamp_t _currentObservedMarkerTimestamp = 0;
    }; // class BlockWorld


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
    const ObservableObject* BlockWorld::FindLocatedObjectOnTopOf(const ObservableObject& objectOnBottom,
                                                                 f32 zTolerance,
                                                                 const BlockWorldFilter& filter) const
    {
      return FindObjectOnTopOrUnderneathHelper(objectOnBottom, zTolerance, filter, true); // returns const
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ObservableObject* BlockWorld::FindLocatedObjectOnTopOf(const ObservableObject& objectOnBottom,
                                                           f32 zTolerance,
                                                           const BlockWorldFilter& filter)
    {
      return FindObjectOnTopOrUnderneathHelper(objectOnBottom, zTolerance, filter, true); // returns non-const
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    const ObservableObject* BlockWorld::FindLocatedObjectUnderneath(const ObservableObject& objectOnTop,
                                                             f32 zTolerance,
                                                             const BlockWorldFilter& filter) const
    {
      return FindObjectOnTopOrUnderneathHelper(objectOnTop, zTolerance, filter, false); // returns const
    }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ObservableObject* BlockWorld::FindLocatedObjectUnderneath(const ObservableObject& objectOnTop,
                                                       f32 zTolerance,
                                                       const BlockWorldFilter& filter)
    {
      return FindObjectOnTopOrUnderneathHelper(objectOnTop, zTolerance, filter, false); // returns non-const
    }
    
  } // namespace Cozmo
} // namespace Anki



#endif // ANKI_COZMO_BLOCKWORLD_H
