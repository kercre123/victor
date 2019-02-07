/**
 * File: blockWorld.cpp
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/1/2013
 *
 * Description: Implements a container for tracking the state of all objects in Cozmo's world.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/
#include "engine/blockWorld/blockWorld.h"

// Putting engine config include first so we get coretech/common/shared/types.h instead of anki/types.h
// TODO: Fix this types.h include mess (COZMO-3752)
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "coretech/common/engine/math/poseOriginList.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/shared/math/rect_impl.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/common/shared/utilities_shared.h"
#include "engine/activeCube.h"
#include "engine/activeObjectHelpers.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/ankiEventUtil.h"
#include "engine/block.h"
#include "engine/charger.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/customObject.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/humanHead.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_Cliff.h"
#include "engine/objectPoseConfirmer.h"
#include "engine/potentialObjectsForLocalizingTo.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/viz/vizManager.h"
#include "coretech/vision/engine/observableObjectLibrary_impl.h"
#include "coretech/vision/engine/visionMarker.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/global/globalDefinitions.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/DAS.h"
#include "util/math/math.h"
#include "webServerProcess/src/webVizSender.h"

// Giving this its own local define, in case we want to control it independently of DEV_CHEATS / SHIPPING, etc.
#define ENABLE_DRAWING ANKI_DEV_CHEATS

#define LOG_CHANNEL "BlockWorld"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  BlockWorld::BlockWorld()
  : UnreliableComponent<BCComponentID>(this, BCComponentID::BlockWorld)
  , IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::BlockWorld)
  , _trackPoseChanges(false)
  {
  } // BlockWorld() Constructor

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::InitDependent(Robot* robot, const RobotCompMap& dependentComps)
  {
    _robot = robot;
    DEV_ASSERT(_robot != nullptr, "BlockWorld.Constructor.InvalidRobot");

    //////////////////////////////////////////////////////////////////////////
    // 1x1 Light Cubes
    //
    DefineObject(std::make_unique<ActiveCube>(ObjectType::Block_LIGHTCUBE1));
#ifdef SIMULATOR
    // VIC-12886 These object types are only used in Webots tests (not in the real world), so only define them if this
    // is sim. The physical robot can sometimes hallucinate these objects, which causes issues.
    DefineObject(std::make_unique<ActiveCube>(ObjectType::Block_LIGHTCUBE2));
    DefineObject(std::make_unique<ActiveCube>(ObjectType::Block_LIGHTCUBE3));
#endif

    //////////////////////////////////////////////////////////////////////////
    // Charger
    //
    DefineObject(std::make_unique<Charger>());

    if(_robot->HasExternalInterface())
    {
      SetupEventHandlers(*_robot->GetExternalInterface());
    }
  }


  void BlockWorld::SetupEventHandlers(IExternalInterface& externalInterface)
  {
    using namespace ExternalInterface;
    auto helper = MakeAnkiEventUtil(externalInterface, *this, _eventHandles);
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DeleteAllCustomObjects>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::UndefineAllCustomMarkerObjects>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DeleteCustomMarkerObjects>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DeleteFixedCustomObjects>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SelectNextObject>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::CreateFixedCustomObject>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DefineCustomBox>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DefineCustomCube>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DefineCustomWall>();
  }

  BlockWorld::~BlockWorld()
  {

  } // ~BlockWorld() Destructor

  Result BlockWorld::DefineObject(std::unique_ptr<const ObservableObject>&& object)
  {
    // Store due to std::move
    const ObjectType objType = object->GetType();

    // Find objects that already exist with this type
    BlockWorldFilter filter;
    filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
    filter.AddAllowedType(objType);
    ObservableObject* objWithType = FindLocatedMatchingObject(filter);
    const bool redefiningExistingType = (objWithType != nullptr);

    const Result addResult = _objectLibrary.AddObject(std::move(object));

    if(RESULT_OK == addResult)
    {
      PRINT_CH_DEBUG("BlockWorld", "BlockWorld.DefineObject.AddedObjectDefinition",
                     "Defined %s in Object Library", EnumToString(objType));

      if(redefiningExistingType)
      {
        PRINT_NAMED_WARNING("BlockWorld.DefineObject.RemovingObjectsWithPreviousDefinition",
                            "Type %s was already defined, removing object(s) with old definition",
                            EnumToString(objType));

        BlockWorldFilter filter;
        filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
        filter.AddAllowedType(objType);
        DeleteLocatedObjects(filter);
      }
    }
    else
    {
      PRINT_NAMED_WARNING("BlockWorld.DefineObject.FailedToDefineObject",
                          "Failed defining %s", EnumToString(objType));
    }

    return addResult;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::DeleteFixedCustomObjects& msg)
  {
    BlockWorldFilter filter;
    filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
    filter.AddFilterFcn(&BlockWorldFilter::IsCustomObjectFilter);
    filter.AddAllowedType(ObjectType::CustomFixedObstacle);
    DeleteLocatedObjects(filter);
    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::RobotDeletedFixedCustomObjects>();
  }

  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::DeleteCustomMarkerObjects& msg)
  {
    BlockWorldFilter filter;
    filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
    filter.AddFilterFcn(&BlockWorldFilter::IsCustomObjectFilter);
    filter.AddIgnoreType(ObjectType::CustomFixedObstacle); // everything custom _except_ fixed obstacles
    DeleteLocatedObjects(filter);
    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::RobotDeletedCustomMarkerObjects>();
  }

  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::DeleteAllCustomObjects& msg)
  {
    BlockWorldFilter filter;
    filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
    filter.AddFilterFcn(&BlockWorldFilter::IsCustomObjectFilter);
    DeleteLocatedObjects(filter);
    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::RobotDeletedAllCustomObjects>();
  };

  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::UndefineAllCustomMarkerObjects& msg)
  {
    // First we need to delete any custom marker objects we already have
    // Note that this does
    HandleMessage(ExternalInterface::DeleteCustomMarkerObjects());

    // Remove the definition of anything that uses any Custom marker from the ObsObjLibrary
    static_assert(Util::EnumToUnderlying(CustomObjectMarker::Circles2) == 0,
                  "Assuming first CustomObjectMarker is Circles2");

    s32 numRemoved = 0;
    for(auto customMarker = CustomObjectMarker::Circles2; customMarker < CustomObjectMarker::Count; ++customMarker)
    {
      const Vision::MarkerType markerType = CustomObject::GetVisionMarkerType(customMarker);
      const bool removed = _objectLibrary.RemoveObjectWithMarker(markerType);
      if(removed) {
        ++numRemoved;
      }
    }

    PRINT_CH_INFO("BlockWorld", "BlockWorld.HandleMessage.UndefineAllCustomObjects",
                  "%d objects removed from library", numRemoved);
  }

  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::SelectNextObject& msg)
  {
    CycleSelectedObject();
  };

  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::CreateFixedCustomObject& msg)
  {
    Pose3d newObjectPose(msg.pose, _robot->GetPoseOriginList());

    ObjectID id = BlockWorld::CreateFixedCustomObject(newObjectPose, msg.xSize_mm, msg.ySize_mm, msg.zSize_mm);

    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::CreatedFixedCustomObject>(id);
  };

  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::DefineCustomBox& msg)
  {
    bool success = false;

    CustomObject* customBox = CustomObject::CreateBox(msg.customType,
                                                      msg.markerFront,
                                                      msg.markerBack,
                                                      msg.markerTop,
                                                      msg.markerBottom,
                                                      msg.markerLeft,
                                                      msg.markerRight,
                                                      msg.xSize_mm, msg.ySize_mm, msg.zSize_mm,
                                                      msg.markerWidth_mm, msg.markerHeight_mm,
                                                      msg.isUnique);

    if(nullptr != customBox)
    {
      const Result defineResult = DefineObject(std::unique_ptr<CustomObject>(customBox));
      success = (defineResult == RESULT_OK);
    }

    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::DefinedCustomObject>(success);
  };

  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::DefineCustomCube& msg)
  {
    bool success = false;

    CustomObject* customCube = CustomObject::CreateCube(msg.customType,
                                                        msg.marker,
                                                        msg.size_mm,
                                                        msg.markerWidth_mm, msg.markerHeight_mm,
                                                        msg.isUnique);

    if(nullptr != customCube)
    {
      const Result defineResult = DefineObject(std::unique_ptr<CustomObject>(customCube));
      success = (defineResult == RESULT_OK);
    }

    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::DefinedCustomObject>(success);
  };

  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::DefineCustomWall& msg)
  {
    bool success = false;

    CustomObject* customWall = CustomObject::CreateWall(msg.customType,
                                                        msg.marker,
                                                        msg.width_mm, msg.height_mm,
                                                        msg.markerWidth_mm, msg.markerHeight_mm,
                                                        msg.isUnique);
    if(nullptr != customWall)
    {
      const Result defineResult = DefineObject(std::unique_ptr<CustomObject>(customWall));
      success = (defineResult == RESULT_OK);
    }

    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::DefinedCustomObject>(success);
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ObservableObject* BlockWorld::FindLocatedObjectHelper(const BlockWorldFilter& filterIn,
                                                        const ModifierFcn& modifierFcn,
                                                        bool returnFirstFound) const
  {
    ObservableObject* matchingObject = nullptr;

    BlockWorldFilter filter(filterIn);

    if(filter.IsOnlyConsideringLatestUpdate())
    {
      const RobotTimeStamp_t atTimestamp = _currentObservedMarkerTimestamp;

      filter.AddFilterFcn([atTimestamp](const ObservableObject* object) -> bool
                          {
                            const bool seenAtTimestamp = (object->GetLastObservedTime() == atTimestamp);
                            return seenAtTimestamp;
                          });
    }

    const auto& currRobotOriginId = _robot->GetPoseOriginList().GetCurrentOriginID();
    
    for(const auto & objectsByOrigin : _locatedObjects) {
      const auto& originID = objectsByOrigin.first;
      if (!filter.ConsiderOrigin(originID, currRobotOriginId)) {
        continue;
      }
      for (const auto& object : objectsByOrigin.second) {
        if (nullptr == object) {
          LOG_ERROR("BlockWorld.FindLocatedObjectHelper.NullObject", "origin %d", originID);
          continue;
        }
        const bool objectMatches = filter.ConsiderType(object->GetType()) &&
                                   filter.ConsiderObject(object.get());
        if (objectMatches) {
          matchingObject = object.get();
          if(nullptr != modifierFcn) {
            modifierFcn(matchingObject);
          }
          if(returnFirstFound) {
            return matchingObject;
          }
        }
      }
    }

    return matchingObject;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ActiveObject* BlockWorld::FindConnectedObjectHelper(const BlockWorldFilter& filter,
                                                      const ModifierFcn& modifierFcn,
                                                      bool returnFirstFound) const
  {
    ActiveObject* matchingObject = nullptr;

    DEV_ASSERT(!filter.IsOnlyConsideringLatestUpdate(), "BlockWorld.FindConnectedObjectHelper.InvalidFlag");

    for (auto& connectedObject : _connectedObjects) {
      if(nullptr == connectedObject) {
        LOG_ERROR("BlockWorld.FindConnectedObjectHelper.NullObject", "");
        continue;
      }
      const bool objectMatches = filter.ConsiderType(connectedObject->GetType()) &&
                                 filter.ConsiderObject(connectedObject.get());
      if (objectMatches) {
        matchingObject = connectedObject.get();
        if(nullptr != modifierFcn) {
          modifierFcn(matchingObject);
        }
        if(returnFirstFound) {
          return matchingObject;
        }
      }
    }

    return matchingObject;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ObservableObject* BlockWorld::GetLocatedObjectByIdHelper(const ObjectID& objectID) const
  {
    // Find the object with the given ID with any pose state, in the current world origin
    BlockWorldFilter filter;
    filter.AddAllowedID(objectID);

    // Find and return match
    ObservableObject* match = FindLocatedObjectHelper(filter, nullptr, true);
    return match;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ActiveObject* BlockWorld::GetConnectedActiveObjectByIdHelper(const ObjectID& objectID) const
  {
    // Find the object with the given ID
    BlockWorldFilter filter;
    filter.AddAllowedID(objectID);

    // Find and return among ConnectedObjects
    ActiveObject* object = FindConnectedObjectHelper(filter, nullptr, true);
    return object;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ActiveObject* BlockWorld::GetConnectedActiveObjectByActiveIdHelper(const ActiveID& activeID) const
  {
    // Find object that matches given activeID
    BlockWorldFilter filter;
    filter.SetFilterFcn([activeID](const ObservableObject* object) {
      return object->GetActiveID() == activeID;
    });

    // Find and return among ConnectedObjects
    ActiveObject* object = FindConnectedObjectHelper(filter, nullptr, true);
    return object;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ObservableObject* BlockWorld::FindLocatedObjectClosestToHelper(const Pose3d& pose,
                                                                 const Vec3f&  distThreshold,
                                                                 const BlockWorldFilter& filterIn) const
  {
    // TODO: Keep some kind of OctTree data structure to make these queries faster?

    // Note: This function only considers the magnitude of distThreshold, not the individual elements (see VIC-12526)
    float closestDist = distThreshold.Length();

    BlockWorldFilter filter(filterIn);
    filter.AddFilterFcn([&pose, &closestDist](const ObservableObject* current)
                        {
                          float dist = 0.f;
                          if (!ComputeDistanceBetween(pose, current->GetPose(), dist)) {
                            LOG_ERROR("BlockWorld.FindLocatedObjectClosestToHelper.FilterFcn",
                                      "Failed to compute distance between input pose and block pose");
                            return false;
                          }
                          if(dist < closestDist) {
                            closestDist = dist;
                            return true;
                          } else {
                            return false;
                          }
                        });

    return FindLocatedObjectHelper(filter);
  }


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ObservableObject* BlockWorld::FindLocatedClosestMatchingObjectHelper(const ObservableObject& object,
                                                                       const Vec3f& distThreshold,
                                                                       const Radians& angleThreshold,
                                                                       const BlockWorldFilter& filterIn) const
  {
    Vec3f closestDist(distThreshold);
    Radians closestAngle(angleThreshold);

    // Don't check the object we're using as the comparison
    BlockWorldFilter filter(filterIn);
    filter.AddIgnoreID(object.GetID());
    filter.AddFilterFcn([&object,&closestDist,&closestAngle](const ObservableObject* current)
    {
      Vec3f Tdiff;
      Radians angleDiff;
      if(current->IsSameAs(object, closestDist, closestAngle, Tdiff, angleDiff)) {
        closestDist = Tdiff.GetAbs();
        closestAngle = angleDiff.getAbsoluteVal();
        return true;
      } else {
        return false;
      }
    });

    ObservableObject* closestObject = FindLocatedObjectHelper(filter);
    return closestObject;
  }


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ObservableObject* BlockWorld::FindLocatedClosestMatchingTypeHelper(ObjectType withType,
                                                                     const Pose3d& pose,
                                                                     const Vec3f& distThreshold,
                                                                     const Radians& angleThreshold,
                                                                     const BlockWorldFilter& filterIn) const
  {
    Vec3f closestDist(distThreshold);
    Radians closestAngle(angleThreshold);

    BlockWorldFilter filter(filterIn);
    filter.AddFilterFcn([withType,&pose,&closestDist,&closestAngle](const ObservableObject* current)
    {
      Vec3f Tdiff;
      Radians angleDiff;
      if(current->GetType() == withType &&
         current->GetPose().IsSameAs(pose, closestDist, closestAngle, Tdiff, angleDiff))
      {
        closestDist = Tdiff.GetAbs();
        closestAngle = angleDiff.getAbsoluteVal();
        return true;
      } else {
        return false;
      }
    });

    ObservableObject* closestObject = FindLocatedObjectHelper(filter);
    return closestObject;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  BlockWorld::ObjectsContainer_t::const_iterator BlockWorld::FindInContainerWithID(const ObjectsContainer_t& container,
                                                                                   const ObjectID& objectID)
  {
    return std::find_if(container.begin(),
                        container.end(),
                        [&objectID](const ObjectsContainer_t::value_type& existingObj){
                          return (existingObj != nullptr) && (existingObj->GetID() == objectID);
                        });
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result BlockWorld::BroadcastObjectObservation(const ObservableObject* observedObject) const
  {
    // Project the observed object into the robot's camera to get the bounding box
    // within the image
    std::vector<Point2f> projectedCorners;
    f32 observationDistance = 0;
    _robot->GetVisionComponent().GetCamera().ProjectObject(*observedObject, projectedCorners, observationDistance);
    const Rectangle<f32> boundingBox(projectedCorners);

    // Compute the orientation of the top marker
    Radians topMarkerOrientation(0);
    if(observedObject->IsActive()) {
      if (IsValidLightCube(observedObject->GetType(), false))
      {
        const ActiveCube* activeCube = dynamic_cast<const ActiveCube*>(observedObject);

        if(activeCube == nullptr) {
          PRINT_NAMED_ERROR("BlockWorld.BroadcastObjectObservation.NullActiveCube",
                            "ObservedObject %d with IsActive()==true could not be cast to ActiveCube.",
                            observedObject->GetID().GetValue());
          return RESULT_FAIL;
        }

        topMarkerOrientation = activeCube->GetTopMarkerOrientation();
      }
    }

    using namespace ExternalInterface;

    RobotObservedObject observation(observedObject->GetLastObservedTime(),
                                    observedObject->GetType(),
                                    observedObject->GetID(),
                                    CladRect(boundingBox.GetX(),
                                             boundingBox.GetY(),
                                             boundingBox.GetWidth(),
                                             boundingBox.GetHeight()),
                                    observedObject->GetPose().ToPoseStruct3d(_robot->GetPoseOriginList()),
                                    topMarkerOrientation.ToFloat(),
                                    observedObject->IsActive());

    if( ANKI_DEV_CHEATS ) {
      SendObjectUpdateToWebViz( observation );
    }

    _robot->Broadcast(MessageEngineToGame(std::move(observation)));

    return RESULT_OK;

  } // BroadcastObjectObservation()

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::BroadcastLocatedObjectStates()
  {
    using namespace ExternalInterface;

    // Create default filter: current origin, any object, any poseState
    BlockWorldFilter filter;

    LocatedObjectStates objectStates;
    filter.SetFilterFcn([this,&objectStates](const ObservableObject* obj)
                        {
                          const bool isConnected = obj->GetActiveID() >= 0;
                          LocatedObjectState objectState(obj->GetID(),
                                                  obj->GetLastObservedTime(),
                                                  obj->GetType(),
                                                  obj->GetPose().ToPoseStruct3d(_robot->GetPoseOriginList()),
                                                  obj->GetPoseState(),
                                                  isConnected);

                          objectStates.objects.push_back(std::move(objectState));
                          return true;
                        });

    // Iterate over all objects and add them to the available objects list if they pass the filter
    FindLocatedObjectHelper(filter, nullptr, false);

    _robot->Broadcast(MessageEngineToGame(std::move(objectStates)));
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::BroadcastConnectedObjects()
  {
    using namespace ExternalInterface;

    // Create default filter: any object
    BlockWorldFilter filter;

    ConnectedObjectStates objectStates;
    filter.SetFilterFcn([&objectStates](const ObservableObject* obj)
                        {
                          ConnectedObjectState objectState(obj->GetID(),
                                                  obj->GetType());

                          objectStates.objects.push_back(std::move(objectState));
                          return true;
                        });

    // Iterate over all objects and add them to the available objects list if they pass the filter
    FindConnectedObjectHelper(filter, nullptr, false);

    _robot->Broadcast(MessageEngineToGame(std::move(objectStates)));
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result BlockWorld::UpdateObjectOrigin(const ObjectID& objectID, const PoseOriginID_t oldOriginID)
  {
    auto originIter = _locatedObjects.find(oldOriginID);
    if(originIter == _locatedObjects.end())
    {
      PRINT_CH_INFO("BlockWorld", "BlockWorld.UpdateObjectOrigin.BadOrigin",
                    "Origin %d not found", oldOriginID);

      return RESULT_FAIL;
    }

    DEV_ASSERT_MSG(_robot->GetPoseOriginList().ContainsOriginID(oldOriginID),
                   "BlockWorld.UpdateObjectOrigin.OldOriginNotInOriginList",
                   "ID:%d", oldOriginID);

    const Pose3d& oldOrigin = _robot->GetPoseOriginList().GetOriginByID(oldOriginID);

    auto& objectsInOldOrigin = originIter->second;

    auto objectIt = FindInContainerWithID(objectsInOldOrigin, objectID);
    
    if (objectIt == objectsInOldOrigin.end()) {
      LOG_INFO("BlockWorld.UpdateObjectOrigin.ObjectNotFound",
               "Object %d not found in origin %s",
               objectID.GetValue(), oldOrigin.GetName().c_str());
      return RESULT_FAIL;
    }
    
    const auto& object = *objectIt;
    if (!object->GetPose().HasSameRootAs(oldOrigin)) {
      const Pose3d& newOrigin = object->GetPose().FindRoot();
      
      LOG_INFO("BlockWorld.UpdateObjectOrigin.ObjectFound",
               "Updating ObjectID %d from origin %s to %s",
               objectID.GetValue(),
               oldOrigin.GetName().c_str(),
               newOrigin.GetName().c_str());
      
      const PoseOriginID_t newOriginID = newOrigin.GetID();
      DEV_ASSERT_MSG(_robot->GetPoseOriginList().ContainsOriginID(newOriginID),
                     "BlockWorld.UpdateObjectOrigin.ObjectOriginNotInOriginList",
                     "Name:%s", object->GetPose().FindRoot().GetName().c_str());
      
      // Add to object's current origin (if it's there already, issue a warning and remove the duplicate first)
      auto& objectsInNewOrigin = _locatedObjects[newOriginID];
      auto existingIt = FindInContainerWithID(objectsInNewOrigin, object->GetID());
      if (existingIt != objectsInNewOrigin.end()) {
        LOG_WARNING("BlockWorld.UpdateObjectOrigin.ObjectAlreadyInNewOrigin",
                    "Removing existing object. ObjectID %d, old origin %s, new origin %s",
                    objectID.GetValue(),
                    oldOrigin.GetName().c_str(),
                    newOrigin.GetName().c_str());
        objectsInNewOrigin.erase(existingIt);
      }

      _locatedObjects[newOriginID].push_back(object);
      
      // Notify pose confirmer
      _robot->GetObjectPoseConfirmer().AddInExistingPose(object.get());
      
      // Delete from old origin
      objectsInOldOrigin.erase(objectIt);
      
      // Clean up if we just deleted the last object from this origin
      if(objectsInOldOrigin.empty())
      {
        _locatedObjects.erase(originIter);
      }
    }
    
    return RESULT_OK;
  }

  Result BlockWorld::UpdateObjectOrigins(PoseOriginID_t oldOriginID, PoseOriginID_t newOriginID)
  {
    Result result = RESULT_OK;

    if(!ANKI_VERIFY(PoseOriginList::UnknownOriginID != oldOriginID &&
                    PoseOriginList::UnknownOriginID != newOriginID,
                    "BlockWorld.UpdateObjectOrigins.OriginFail",
                    "Old and new origin IDs must not be Unknown"))
    {
      return RESULT_FAIL;
    }

    DEV_ASSERT_MSG(_robot->GetPoseOriginList().ContainsOriginID(oldOriginID),
                   "BlockWorld.UpdateObjectOrigins.BadOldOriginID", "ID:%d", oldOriginID);

    DEV_ASSERT_MSG(_robot->GetPoseOriginList().ContainsOriginID(newOriginID),
                   "BlockWorld.UpdateObjectOrigins.BadNewOriginID", "ID:%d", newOriginID);

    const Pose3d& oldOrigin = _robot->GetPoseOriginList().GetOriginByID(oldOriginID);
    const Pose3d& newOrigin = _robot->GetPoseOriginList().GetOriginByID(newOriginID);

    // COZMO-9797
    // lengthy discussion between Andrew and Raul. Ideally we would keep unconfirmed observations when
    // we rejigger. However, because we move from current to a previously known origin, the code is more
    // complicated that way. Since we plan on refactoring PoseConfirmer in the future, for now we are ok with
    // losing information. Clear PoseConfirmer here and let the originUpdater and addToPoseConfirmer modifiers
    // later re-add kosher information.
    _robot->GetObjectPoseConfirmer().Clear();

    // Look for objects in the old origin
    BlockWorldFilter filterOld;
    filterOld.SetOriginMode(BlockWorldFilter::OriginMode::Custom);
    filterOld.AddAllowedOrigin(oldOriginID);

    // Use the modifier function to update matched objects to the new origin
    ModifierFcn originUpdater = [&oldOrigin,&newOrigin,newOriginID,&result,this](ObservableObject* oldObject)
    {
      Pose3d newPose;

      if(_robot->GetCarryingComponent().IsCarryingObject(oldObject->GetID()))
      {
        // Special case: don't use the pose w.r.t. the origin b/c carried objects' parent
        // is the lift. The robot is already in the new frame by the time this called,
        // so we don't need to adjust anything
        DEV_ASSERT(_robot->GetPoseOriginList().GetCurrentOriginID() == newOriginID,
                   "BlockWorld.UpdateObjectOrigins.RobotNotInNewOrigin");
        DEV_ASSERT(oldObject->GetPose().GetRootID() == newOriginID,
                   "BlockWorld.UpdateObjectOrigins.OldCarriedObjectNotInNewOrigin");
        newPose = oldObject->GetPose();
      }
      else if(false == oldObject->GetPose().GetWithRespectTo(newOrigin, newPose))
      {
        PRINT_NAMED_ERROR("BlockWorld.UpdateObjectOrigins.OriginFail",
                          "Could not get object %d w.r.t new origin %s",
                          oldObject->GetID().GetValue(),
                          newOrigin.GetName().c_str());

        result = RESULT_FAIL;
        return;
      }

      const Vec3f& T_old = oldObject->GetPose().GetTranslation();
      const Vec3f& T_new = newPose.GetTranslation();

      // Look for a matching object in the new origin. Should have same type. If unique, should also have
      // same ID, or if not unique, the poses should match.
      BlockWorldFilter filterNew;
      filterNew.SetOriginMode(BlockWorldFilter::OriginMode::Custom);
      filterNew.AddAllowedOrigin(newOriginID);
      filterNew.AddAllowedType(oldObject->GetType());

      ObservableObject* newObject = nullptr;

      if(oldObject->IsUnique())
      {
        filterNew.AddFilterFcn(BlockWorldFilter::UniqueObjectsFilter);
        filterNew.AddAllowedID(oldObject->GetID());
        newObject = FindLocatedMatchingObject(filterNew);
      }
      else
      {
        newObject = FindLocatedObjectClosestTo(oldObject->GetPose(),
                                               oldObject->GetSameDistanceTolerance(),
                                               filterNew);
      }

      bool addNewObject = false;
      if(nullptr == newObject)
      {
        PRINT_CH_INFO("BlockWorld", "BlockWorld.UpdateObjectOrigins.NoMatchFound",
                      "No match found for %s %d, adding new at T=(%.1f,%.1f,%.1f)",
                      EnumToString(oldObject->GetType()),
                      oldObject->GetID().GetValue(),
                      T_new.x(), T_new.y(), T_new.z());

        newObject = oldObject->CloneType();

        // This call is necessary due to dependencies from CopyWithNewPose and AddNewObject
        // AddNewObject needs the correct origin which CopyWithNewPose sets
        // CopyWithNewPose needs the correct object ID which normally would be set by AddNewObject
        // Since this is a circular dependency we need to set the ID first outside of AddNewObject
        newObject->CopyID(oldObject);

        addNewObject = true;
      }
      else
      {
        PRINT_CH_INFO("BlockWorld", "BlockWorld.UpdateObjectOrigins.ObjectOriginChanged",
                      "Updating %s %d's origin from %s to %s (matched by %s to ID:%d). "
                      "T_old=(%.1f,%.1f,%.1f), T_new=(%.1f,%.1f,%.1f)",
                      EnumToString(oldObject->GetType()),
                      oldObject->GetID().GetValue(),
                      oldOrigin.GetName().c_str(),
                      newOrigin.GetName().c_str(),
                      oldObject->IsUnique() ? "type" : "pose",
                      newObject->GetID().GetValue(),
                      T_old.x(), T_old.y(), T_old.z(),
                      T_new.x(), T_new.y(), T_new.z());

        // we also want to keep the MOST recent objectID, rather than the one we used to have for this object, because
        // if clients are bookkeeping IDs, the know about the new one (for example, if an action is already going
        // to pick up that objectID, it should not change by virtue of rejiggering)
        // Note: despite the name, oldObject is the most recent instance of this match. Thanks, Andrew.
        newObject->CopyID( oldObject );
      }

      // Use all of oldObject's time bookkeeping, then update the pose and pose state
      newObject->SetObservationTimes(oldObject);
      result = _robot->GetObjectPoseConfirmer().CopyWithNewPose(newObject, newPose, oldObject);

      if(addNewObject)
      {
        // Note: need to call SetPose first because that sets the origin which
        // controls which map the object gets added to
        AddLocatedObject(std::shared_ptr<ObservableObject>(newObject));

        PRINT_CH_INFO("BlockWorld", "BlockWorld.UpdateObjectOrigins.NoMatchingObjectInNewFrame",
                      "Adding %s object with ID %d to new origin %s",
                      EnumToString(newObject->GetType()),
                      newObject->GetID().GetValue(),
                      newOrigin.GetName().c_str());
      }

    };

    // Apply the filter and modify each object that matches
    ModifyLocatedObjects(originUpdater, filterOld);

    if(RESULT_OK == result) {
      // Erase all the objects in the old frame now that their counterparts in the new
      // frame have had their poses updated
      // rsam: Note we don't have to call Delete since we don't clear or notify. There is no way that we could
      // be deleting any objects in this origin during rejigger, since we bring objects to the previously known map or
      // override their pose. For that reason, directly remove the origin rather than calling DeleteLocatedObjectsByOrigin
      // Note that we decide to not notify of objects that merge (passive matched by pose), because the old ID in the
      // old origin is not in the current one.
      _locatedObjects.erase(oldOriginID);
    }

    // Now go through all the objects already in the origin we are switching _to_
    // (the "new" origin) and make sure PoseConfirmer knows about them since we
    // have delocalized since last being in this origin (which clears the PoseConfirmer)
    BlockWorldFilter filterNew;
    filterOld.SetOriginMode(BlockWorldFilter::OriginMode::Custom);
    filterOld.AddAllowedOrigin(newOriginID);

    ModifierFcn addToPoseConfirmer = [this](ObservableObject* object){
      _robot->GetObjectPoseConfirmer().AddInExistingPose(object);
    };

    FindLocatedObjectHelper(filterNew, addToPoseConfirmer, false);

    // Notify the world about the objects in the new coordinate frame, in case
    // we added any based on rejiggering (not observation). Include unconnected
    // ones as well.
    BroadcastLocatedObjectStates();

    return result;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::DeleteObjectsFromZombieOrigins()
  {
    ObjectsByOrigin_t::iterator originIt = _locatedObjects.begin();
    while( originIt != _locatedObjects.end() )
    {
      const bool isZombie = IsZombiePoseOrigin( originIt->first );
      if ( isZombie ) {
        PRINT_CH_INFO("BlockWorld", "BlockWorld.DeleteObjectsFromZombieOrigins.DeletingOrigin",
                      "Deleting objects from Origin %d because it was zombie", originIt->first);
        originIt = _locatedObjects.erase(originIt);
      } else {
        PRINT_CH_DEBUG("BlockWorld", "BlockWorld.DeleteObjectsFromZombieOrigins.KeepingOrigin",
                       "Origin %d is still good (keeping objects)", originIt->first);
        ++originIt;
      }
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result BlockWorld::AddAndUpdateObjects(const std::vector<ObservableObject*>& objectsSeen,
                                         const RobotTimeStamp_t atTimestamp)
  {
    const Pose3d& currFrame = _robot->GetWorldOrigin();
    const PoseOriginID_t currFrameID = currFrame.GetID();

    // Construct a helper data structure for determining which objects we might
    // want to localize to
    PotentialObjectsForLocalizingTo potentialObjectsForLocalizingTo(*_robot);

    for(const auto& objSeenRawPtr : objectsSeen)
    {
      // Note that we wrap shared_ptr around the passed-in object, which was created
      // by the caller (via Cloning from the library of known objects). We will either
      // add this object to the poseConfirmer, in which case it will not get deleted,
      // or it will simply be used to update an existing object's pose and/or for
      // localization, both done by potentialObjectsForLocalizingTo. In that case,
      // once potentialObjectsForLocalizingTo is done with it, and we have exited
      // this for loop, its refcount will be zero and it'll get deleted for us.
      std::shared_ptr<ObservableObject> objSeen(objSeenRawPtr);

      // We use the distance to the observed object to decide (a) if the object is
      // close enough to do localization/identification and (b) to use only the
      // closest object in each coordinate frame for localization.
      f32 distToObjSeen = 0.f;
      if (!ComputeDistanceBetween(_robot->GetPose(), objSeen->GetPose(), distToObjSeen)) {
        LOG_ERROR("BlockWorld.AddAndUpdateObjects.ComputeDistanceFailure",
                  "Failed to compute distance between robot and object seen");
        return RESULT_FAIL;
      }

      // First thing that we have to do is ask the PoseConfirmer whether this is a confirmed object,
      // or a visual observation of an object that we want to consider for the future (unconfirmed object).
      // Pass in the object as an observation
      const ObservableObject* poseConfirmationObjectIDMatch = nullptr;
      const bool isConfirmedAtPose = _robot->GetObjectPoseConfirmer().IsObjectConfirmedAtObservedPose(objSeen,
                                                                  poseConfirmationObjectIDMatch);

      // inherit the ID of a match, or assign a new one depending if there were no matches
      DEV_ASSERT( !objSeen->GetID().IsSet(), "BlockWorld.AddAndUpdateObjects.ObservationAlreadyHasID");
      if ( nullptr != poseConfirmationObjectIDMatch ) {
        objSeen->CopyID( poseConfirmationObjectIDMatch );
      } else {
        objSeen->SetID();
      }

      /*
        Note: Andrew and Raul think that next iteration of PoseConfirmer vs PotentialObjectsToLocalizeTo should
              separate 'observations' from 'confirmations', where PoseConfirmer turns into ObservationFilter,
              and only reports accepted observations (aka confirmation) towards this method/PotentialOTLT. In
              that scenario, observations that come here would either update the robot (localizing) or update
              the object in blockworld (not localizing), but there would be no need to feed back that observation
              back into the PoseConfirmer/ObservationFilter, which is a one-way filter.

              For now, to prevent adding an observation twice or not adding it at all, flag here whether the
              observation is used by the PoseConfirmer before letting it cascade down when it confirms an object (which
              is a fix for a bug in which observations that confirm an object were not used for localization,
              which would delay rejiggering by one tick, while any other system already had access to the object from
              the BlockWorld.)
      */
      bool observationAlreadyUsed = false;

      // if the object is not confirmed add this observation to the poseConfirmer. If this observation causes
      // a confirmation, we don't need to localize to it in the current origin because we are setting its pose
      // wrt robot based on this observation. However, we could bring other origins with it.
      if ( !isConfirmedAtPose )
      {
        // even if not confirmed at this pose, it could be confirmed in this origin. AddVisualObservation needs
        // to receive any confirmed matches in the current origin, but IsObjectConfirmedAtObservedPose no longer
        // provides separate pointers for this. Note this is an optimization so that AddVisualObservation doesn't
        // need to do GetLocatedObjectByID itself when it's not necessary, but it seems to complicate logic.
        // Rethink that API in following iterations of PoseConfirmer
        ObservableObject* curMatchInOrigin = GetLocatedObjectByID(objSeen->GetID());

        // Add observation. Check if the robot was moving at all
        const bool wasRobotMoving = _robot->GetMoveComponent().WasCameraMoving(atTimestamp);
        const bool isConfirmingObservation = _robot->GetObjectPoseConfirmer().AddVisualObservation(objSeen,
                                                                                curMatchInOrigin,
                                                                                wasRobotMoving,
                                                                                distToObjSeen);
        if ( !isConfirmingObservation ) {
          // Don't print this during the factory test because it spams when seeing the
          // calibration target
          #if !FACTORY_TEST
          PRINT_CH_INFO("BlockWorld", "BlockWorld.AddAndUpdateObjects.NonConfirmingObservation",
                        "Added non-confirming visual observation for %d", objSeen->GetID().GetValue() );
          #endif

          // TODO should we broadcast RobotObservedPossibleObject here?
          continue;
        }

        // the observation was used to confirm the object. If the observation is not used to localize, do not
        // add it back again to the PoseConfirmer (see note in declaration of variable)
        observationAlreadyUsed = true;
      }

      // At this point the object is confirmed in the current origin, find matches and see if we want to localize to it
      // Note that if it just became confirmed we still want to localize to it in case that it can rejigger
      // other origins

      // Keep a list of objects matching this objSeen, by coordinate frame
      std::map<PoseOriginID_t, ObservableObject*> matchingObjects;

      // Match unique objects by type and nonunique objects by pose. Regardless,
      // only look at objects with the same type.
      BlockWorldFilter filter;
      filter.AddAllowedType(objSeen->GetType());

      if (objSeen->IsUnique())
      {
        // Can consider matches for unique objects in other coordinate frames
        filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
        filter.AddFilterFcn(&BlockWorldFilter::UniqueObjectsFilter);

        // Unique objects just match based on type (already set above)
        std::vector<ObservableObject*> objectsFound;
        FindLocatedMatchingObjects(filter, objectsFound);

        bool matchedCarryingObject = false;
        for(ObservableObject* objectFound : objectsFound)
        {
          assert(nullptr != objectFound);

          if(objectFound->GetPose().HasSameRootAs(currFrame))
          {
            // handle special case of seeing the object that we are carrying. We don't want to use it for localization
            // Note that if the object is observed at a different location than the lift, it will be moved to that
            // location, and the robot should be notified that it's no longer carried. That responsibility now
            // lies on whomever changes the position (PoseConfirmer)
            if (_robot->GetCarryingComponent().IsCarryingObject(objectFound->GetID()))
            {
              // If this is the object we're carrying observed in the carry position,
              // do nothing and continue to the next observed object.
              matchedCarryingObject = true;
              PRINT_NAMED_WARNING("Blockworld.AddAndUpdateObjects.SeeingCarriedObject",
                                  "Seeing object %s[%d] on lift at height %fmm",
                                  EnumToString(objSeen->GetType()),
                                  objSeen->GetID().GetValue(),
                                  _robot->GetLiftHeight());
              break; // break out of the current matches for the observation
            }
          }

          // Check for duplicates (same type observed in the same coordinate frame)
          // and add to our map of matches by origin
          const Pose3d& objectRoot = objectFound->GetPose().FindRoot();
          const PoseOriginID_t objectOriginID = objectRoot.GetID();
          DEV_ASSERT_MSG(_robot->GetPoseOriginList().ContainsOriginID(objectOriginID),
                         "BlockWorld.AddAndUpdateObjects.ObjectRootNotAnOrigin", "ObjectID:%d Root:%s",
                         objectFound->GetID().GetValue(), objectRoot.GetName().c_str());
          auto iter = matchingObjects.find(objectOriginID);
          if(iter != matchingObjects.end()) {
            PRINT_NAMED_WARNING("BlockWorld.AddAndUpdateObjects.MultipleMatchesForUniqueObjectInSameFrame",
                                "Observed unique object of type %s matches multiple existing objects of "
                                "same type and in the same frame (ID:%d).",
                                EnumToString(objSeen->GetType()), objectOriginID);
          } else {
            matchingObjects[objectOriginID] = objectFound;
          }
        } // for each object found

        if(matchedCarryingObject)
        {
          continue;
        }
      }
      else
      {
        // For non-unique objects, match based on pose (considering only objects in current frame)
        // Ignore objects we're carrying
        const ObjectID& carryingObjectID = _robot->GetCarryingComponent().GetCarryingObjectID();
        filter.AddFilterFcn([&carryingObjectID](const ObservableObject* candidate) {
          const bool isObjectBeingCarried = (candidate->GetID() == carryingObjectID);
          return !isObjectBeingCarried;
        });


        // the observation has matched by pose, otherwise it would not be confirmed at this pose. We don't need
        // to match by pose here, we can just used the ID we found as part of the observation confirmation
        ObservableObject* matchingObject = GetLocatedObjectByID( objSeen->GetID() );

        if (nullptr != matchingObject) {
          DEV_ASSERT(matchingObject->GetPose().HasSameRootAs(currFrame),
                     "BlockWorld.AddAndUpdateObjects.MatchedPassiveObjectInOtherCoordinateFrame");
          matchingObjects[currFrameID] = matchingObject;
        }
        else
        {
          PRINT_NAMED_ERROR("BlockWorld.UpdateObjectPoses.AddAndUpdateNoCurrentOriginMatchNonUnique",
                            "Must find match in current origin, since object is confirmed.");
        }

      }

      // We know the object was confirmed, so we must have found the instance in the current origin
      ObservableObject* observedObject = nullptr;

      auto currFrameMatchIter = matchingObjects.find(currFrameID);
      if(currFrameMatchIter != matchingObjects.end())
      {
        observedObject = currFrameMatchIter->second;
      }
      else
      {
        PRINT_NAMED_ERROR("BlockWorld.UpdateObjectPoses.AddAndUpdateNoCurrentOriginMatch",
                          "Must find match in current origin, since object is confirmed.");
        continue;
      }

      // Update lastObserved times of this object
      // (Do this before possibly attempting to localize to the object below!)
      observedObject->SetLastObservedTime(objSeen->GetLastObservedTime());
      observedObject->UpdateMarkerObservationTimes(*objSeen);

      // See if we might want to localize to the object matched in the current frame
      const bool wantedToInsertObjectInCurrentFrame = potentialObjectsForLocalizingTo.Insert(objSeen, observedObject, distToObjSeen, observationAlreadyUsed);
      if(wantedToInsertObjectInCurrentFrame)
      {
         // Add all other match pairs from other frames as potentials for localization.
         // We will decide which object(s) to localize to from this list after we've
         // processed all objects seen in this update.
        for(auto & match : matchingObjects)
        {
           // Don't try to reinsert the object in the current frame (already did this above)
           if(match.second != observedObject)
           {
             potentialObjectsForLocalizingTo.Insert(objSeen, match.second, distToObjSeen, true); // all observations used since they are matches
           }
         }
      }

      /* This is pretty verbose...
       fprintf(stdout, "Merging observation of object type=%s, with ID=%d at (%.1f, %.1f, %.1f), timestamp=%d\n",
       objSeen->GetType().GetName().c_str(),
       overlappingObjects[0]->GetID().GetValue(),
       objSeen->GetPose().GetTranslation().x(),
       objSeen->GetPose().GetTranslation().y(),
       objSeen->GetPose().GetTranslation().z(),
       overlappingObjects[0]->GetLastObservedTime());
       */

      // ObservedObject should be set by now and should be in the current frame
      assert(observedObject != nullptr); // this REALLY shouldn't happen
      DEV_ASSERT(observedObject->GetPose().HasSameRootAs(currFrame),
                 "BlockWorld.AddAndUpdateObjects.ObservedObjectNotInCurrentFrame");

      // Add all observed markers of this object as occluders
      std::vector<const Vision::KnownMarker *> observedMarkers;
      observedObject->GetObservedMarkers(observedMarkers);
      for(auto marker : observedMarkers) {
        _robot->GetVisionComponent().GetCamera().AddOccluder(*marker);
      }

      const ObjectID obsID = observedObject->GetID();
      DEV_ASSERT(obsID.IsSet(), "BlockWorld.AddAndUpdateObjects.IDNotSet");


      // Safe to remove this? Haven't seen this warning being printed...
      //
      //      // Sanity check: this should not happen, but we're seeing situations where
      //      // objects think they are being carried when the robot doesn't think it
      //      // is carrying that object
      //      // TODO: Eventually, we should be able to remove this check
      //      ActionableObject* actionObject = dynamic_cast<ActionableObject*>(observedObject);
      //      if(actionObject != nullptr) {
      //        if(actionObject->IsBeingCarried() && _robot->GetCarryingObjectID() != obsID) {
      //          PRINT_NAMED_WARNING("BlockWorld.AddAndUpdateObject.CarryStateMismatch",
      //                              "Object %d thinks it is being carried, but does not match "
      //                              "robot %d's carried object ID (%d). Setting as uncarried.",
      //                              obsID.GetValue(), _robot->GetID(),
      //                              _robot->GetCarryingObjectID().GetValue());
      //          actionObject->SetBeingCarried(false);
      //        }
      //      }

      // Tell the world about the observed object. NOTE: it is guaranteed to be in the current frame.
      BroadcastObjectObservation(observedObject);
    } // for each object seen

    // NOTE: This will be a no-op if we no objects got inserted above as being potentially
    //       useful for localization
    Result localizeResult = potentialObjectsForLocalizingTo.LocalizeRobot();

    return localizeResult;

  } // AddAndUpdateObjects()

  void BlockWorld::CheckForUnobservedObjects(RobotTimeStamp_t atTimestamp)
  {
    // Don't bother if the robot is picked up or if it was rotating too fast to
    // have been able to see the markers on the objects anyway.
    // NOTE: Just using default speed thresholds, which should be conservative.
    if(_robot->GetOffTreadsState() != OffTreadsState::OnTreads ||
       _robot->GetMoveComponent().WasMoving(atTimestamp) ||
       _robot->GetImuComponent().GetImuHistory().WasRotatingTooFast(atTimestamp))
    {
      return;
    }

    auto originIter = _locatedObjects.find(_robot->GetPoseOriginList().GetCurrentOriginID());
    if(originIter == _locatedObjects.end()) {
      // No objects relative to this origin: Nothing to do
      return;
    }

    // Create a list of unobserved object IDs (IDs since we can remove several of them while iterating)
    std::vector<ObjectID> unobservedObjectIDs;

    for (const auto& object : originIter->second) {
      if (nullptr == object) {
        LOG_ERROR("BlockWorld.CheckForUnobservedObjects.NullObject", "");
        continue;
      }
      
      // Look for "unobserved" objects not seen atTimestamp -- but skip objects:
      //    - that are currently being carried
      //    - that we are currently docking to
      const RobotTimeStamp_t lastVisuallyMatchedTime = _robot->GetObjectPoseConfirmer().GetLastVisuallyMatchedTime(object->GetID());
      const bool isUnobserved = ( (lastVisuallyMatchedTime < atTimestamp) &&
                                 (_robot->GetCarryingComponent().GetCarryingObjectID() != object->GetID()) &&
                                 (_robot->GetDockingComponent().GetDockObject() != object->GetID()) );
      if ( isUnobserved )
      {
        unobservedObjectIDs.push_back(object->GetID());
      }
    }

    // TODO: Don't bother with this if the robot is docking? (picking/placing)??
    // Now that the occlusion maps are complete, check each unobserved object's
    // visibility in each camera
    const Vision::Camera& camera = _robot->GetVisionComponent().GetCamera();
    DEV_ASSERT(camera.IsCalibrated(), "BlockWorld.CheckForUnobservedObjects.CameraNotCalibrated");
    for(const auto& objectID : unobservedObjectIDs)
    {
      // if the object doesn't exist anymore, it was deleted by another one, for example through a stack
      // or if doesn't have markers (like unexpected move objects), skip
      ObservableObject* unobservedObject = GetLocatedObjectByID(objectID);
      if ( nullptr == unobservedObject || unobservedObject->GetMarkers().empty() ) {
        continue;
      }

      // calculate padding based on distance to object pose
      u16 xBorderPad = 0;
      u16 yBorderPad = 0;
      Pose3d objectPoseWrtCamera;
      if ( unobservedObject->GetPose().GetWithRespectTo(camera.GetPose(), objectPoseWrtCamera) )
      {
        // should have markers
        const auto& markerList = unobservedObject->GetMarkers();
        if ( !markerList.empty() )
        {
          // we think localizable distance is a good distance to give the object a marker's length of padding
          // at localization distance we give a full marker of padding
          const float localizationDistance = unobservedObject->GetMaxLocalizationDistance_mm();
          const Point2f& markerSize = markerList.front().GetSize();
          const f32 focalLenX = camera.GetCalibration()->GetFocalLength_x();
          const f32 focalLenY = camera.GetCalibration()->GetFocalLength_y();
          // distFactor = (1-distNorm) + 1; 1-distNorm to invert normalization, +1 because we want 100% at distNorm=1
          const f32 distToObjInvFactor = 2-(objectPoseWrtCamera.GetTranslation().Length()/localizationDistance);
          float xPadding = focalLenX*markerSize.x()*distToObjInvFactor/localizationDistance;
          float yPadding = focalLenY*markerSize.y()*distToObjInvFactor/localizationDistance;
          xBorderPad = static_cast<u16>(xPadding);
          yBorderPad = static_cast<u16>(yPadding);
        }
        else {
          PRINT_NAMED_ERROR("BlockWorld.CheckForUnobservedObjects.NoMarkers",
                            "Object %d (Type:%s)",
                            objectID.GetValue(),
                            EnumToString(unobservedObject->GetType()) );
          continue;
        }
      }
      else
      {
        PRINT_NAMED_ERROR("BlockWorld.CheckForUnobservedObjects.ObjectNotInCameraPoseOrigin",
                          "Object %d (PosePath:%s)",
                          objectID.GetValue(),
                          unobservedObject->GetPose().GetNamedPathToRoot(false).c_str() );
        continue;
      }

      // Remove objects that should have been visible based on their last known
      // location, but which must not be there because we saw something behind
      // that location:
      bool hasNothingBehind = false;
      const bool shouldBeVisible = unobservedObject->IsVisibleFrom(camera,
                                                                   MAX_MARKER_NORMAL_ANGLE_FOR_SHOULD_BE_VISIBLE_CHECK_RAD,
                                                                   MIN_MARKER_SIZE_FOR_SHOULD_BE_VISIBLE_CHECK_PIX,
                                                                   xBorderPad, yBorderPad,
                                                                   hasNothingBehind);

      const bool isDirtyPoseState = (PoseState::Dirty == unobservedObject->GetPoseState());

      // If the object should _not_ be visible, but the reason was only that
      // it has nothing behind it to confirm that, _and_ the object has already
      // been marked dirty (e.g., by being moved), then increment the number of
      // times it has gone unobserved while dirty.
      if(!shouldBeVisible && hasNothingBehind && isDirtyPoseState)
      {
        _robot->GetObjectPoseConfirmer().MarkObjectUnobserved(unobservedObject);
      }
      else if(shouldBeVisible)
      {
        // We "should" have seen the object! Clear it.
        PRINT_CH_INFO("BlockWorld", "BlockWorld.CheckForUnobservedObjects.MarkingUnobservedObject",
                      "Marking object %d unobserved, which should have been seen, but wasn't. "
                      "(shouldBeVisible:%d hasNothingBehind:%d isDirty:%d)",
                      unobservedObject->GetID().GetValue(),
                      shouldBeVisible, hasNothingBehind, isDirtyPoseState);

        _robot->GetMapComponent().MarkObjectUnobserved(*unobservedObject);
        
        Result markResult = _robot->GetObjectPoseConfirmer().MarkObjectUnobserved(unobservedObject);
        if(RESULT_OK != markResult)
        {
          PRINT_NAMED_WARNING("BlockWorldCheckForUnobservedObjects.MarkObjectUnobservedFailed", "");
        }
      }

    } // for each unobserved object

  } // CheckForUnobservedObjects()

  ObjectID BlockWorld::CreateFixedCustomObject(const Pose3d& p, const f32 xSize_mm, const f32 ySize_mm, const f32 zSize_mm)
  {
    // Create an instance of the custom obstacle
    CustomObject* customObstacle = CustomObject::CreateFixedObstacle(xSize_mm, ySize_mm, zSize_mm);
    if(nullptr == customObstacle)
    {
      PRINT_NAMED_ERROR("BlockWorld.CreateFixedCustomObject.CreateFailed", "");
      return ObjectID{};
    }

    Pose3d obsPose(p);
    obsPose.SetParent(_robot->GetPose().GetParent());

    // Initialize with Known pose so it won't delete immediately because it isn't re-seen
    auto customObject = std::shared_ptr<CustomObject>(customObstacle);
    customObject->InitPose(obsPose, PoseState::Known);

    // set new ID before adding to the world, since this is a new object
    DEV_ASSERT( !customObject->GetID().IsSet(), "BlockWorld.CreateFixedCustomObject.NewObjectHasID" );
    customObject->SetID();

    AddLocatedObject(customObject);

    return customObject->GetID();
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ObjectID BlockWorld::AddConnectedActiveObject(const ActiveID& activeID,
                                                const FactoryID& factoryID,
                                                const ObjectType& objType)
  {
    // only connected objects should be added through this method, so a required activeID is a must
    DEV_ASSERT(activeID != ObservableObject::InvalidActiveID, "BlockWorld.AddConnectedActiveObject.CantAddInvalidActiveID");

    // Validate that ActiveID is not already referring to a connected object
    const auto* conObjWithActiveID = GetConnectedActiveObjectByActiveID( activeID );
    if ( nullptr != conObjWithActiveID)
    {
      // Verify here that factoryID and objectType match, and if they do, simply ignore the message, since we already
      // have a valid instance
      const bool isSameObject = (factoryID == conObjWithActiveID->GetFactoryID()) &&
                                (objType == conObjWithActiveID->GetType());
      if ( isSameObject ) {
        LOG_INFO("BlockWorld.AddConnectedActiveObject.FoundExistingObject",
                 "objectID %d, activeID %d, factoryID %s, type %s",
                 conObjWithActiveID->GetID().GetValue(),
                 conObjWithActiveID->GetActiveID(),
                 conObjWithActiveID->GetFactoryID().c_str(),
                 EnumToString(conObjWithActiveID->GetType()));
        return conObjWithActiveID->GetID();
      }

      // if it's not the same, then what the hell, we are currently using that activeID for other object!
      LOG_ERROR("BlockWorld.AddConnectedActiveObject.ConflictingActiveID",
                "ActiveID:%d found when we tried to add that activeID as connected object. Removing previous.",
                activeID);

      // clear the pointer and destroy it
      conObjWithActiveID = nullptr;
      RemoveConnectedActiveObject(activeID);
    }

    // Validate that factoryId is not currently a connected object
    BlockWorldFilter filter;
    filter.SetFilterFcn([factoryID](const ObservableObject* object) {
      return object->GetFactoryID() == factoryID;
    });
    const ActiveObject* const conObjectWithFactoryID = FindConnectedObjectHelper(filter, nullptr, true);
    ANKI_VERIFY( nullptr == conObjectWithFactoryID, "BlockWorld.AddConnectedActiveObject.FactoryIDAlreadyUsed", "%s", factoryID.c_str() );

    // This is the new object we are going to create. We can't insert it in _connectedObjects until
    // we know the objectID, so we create it first, and then we look for unconnected matches (we have seen the
    // object but we had not connected to it.) If we find one, we will inherit the objectID from that match; if
    // we don't find a match, we will assign it a new objectID. Then we can add to the container of _connectedObjects.
    std::shared_ptr<ActiveObject> newActiveObjectPtr;
    newActiveObjectPtr.reset(CreateActiveObjectByType(objType, activeID, factoryID));
    if ( nullptr == newActiveObjectPtr ) {
      // failed to create the object (that function should print the error, exit here with unSet ID)
      return ObjectID();
    }

    // we can't add to the _connectedObjects until the objectID has been decided

    // Is there an active object with the same activeID and type that already exists?
    BlockWorldFilter filterByActiveID;
    filterByActiveID.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
    filterByActiveID.AddFilterFcn([activeID](const ObservableObject* object) { return object->GetActiveID() == activeID;} );
    filterByActiveID.SetAllowedTypes({objType});
    std::vector<ObservableObject*> matchingObjects;
    FindLocatedMatchingObjects(filterByActiveID, matchingObjects);

    if (matchingObjects.empty())
    {
      // If no match found, find one of the same type with an invalid activeID and assume that's the one we are
      // connecting to
      BlockWorldFilter filterInAny;
      filterInAny.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
      filterInAny.SetAllowedTypes({objType});
      std::vector<ObservableObject*> objectsOfSameType;
      FindLocatedMatchingObjects(filterInAny, objectsOfSameType);

      if(!objectsOfSameType.empty())
      {
        ObjectID matchObjectID;

        // we found located instances of this object that we were not connected to
        for (auto& sameTypeObject : objectsOfSameType)
        {
          if ( matchObjectID.IsSet() ) {
            // check they all have the same objectID across frames
            DEV_ASSERT( matchObjectID == sameTypeObject->GetID(), "BlockWorld.AddConnectedActiveObject.NotSameObjectID");
          } else {
            // set once
            matchObjectID = sameTypeObject->GetID();
          }

          // COZMO-9663: Separate localizable property from poseState
          _robot->GetObjectPoseConfirmer().MarkObjectDirty(sameTypeObject);

          // check if the instance has activeID
          if (sameTypeObject->GetActiveID() == ObservableObject::InvalidActiveID)
          {
            // it doesn't have an activeID, we are connecting to it, set
            sameTypeObject->SetActiveID(activeID);
            sameTypeObject->SetFactoryID(factoryID);
            LOG_INFO("BlockWorld.AddConnectedActiveObject.FoundMatchingObjectWithNoActiveID",
                     "objectID %d, activeID %d, type %s",
                     sameTypeObject->GetID().GetValue(), sameTypeObject->GetActiveID(), EnumToString(objType));
          } else {
            // it has an activeID, we were connected. Is it the same object?
            if ( sameTypeObject->GetFactoryID() != factoryID )
            {
              // uhm, this is a different object (or factoryID was not set)
              LOG_INFO("AddActiveObject.FoundOtherActiveObjectOfSameType",
                       "ActiveID %d (factoryID %s) is same type as another existing object (objectID %d, activeID %d, factoryID %s, type %s) updating ids to match",
                       activeID,
                       factoryID.c_str(),
                       sameTypeObject->GetID().GetValue(),
                       sameTypeObject->GetActiveID(),
                       sameTypeObject->GetFactoryID().c_str(),
                       EnumToString(objType));

              // if we have a new factoryID, override the old instances with the new one we connected to
              if(!factoryID.empty())
              {
                sameTypeObject->SetActiveID(activeID);
                sameTypeObject->SetFactoryID(factoryID);
              }
            } else {
              LOG_INFO("BlockWorld.AddConnectedActiveObject.FoundIdenticalObjectOnDifferentSlot",
                       "Updating activeID of block with factoryID %s from %d to %d",
                       sameTypeObject->GetFactoryID().c_str(), sameTypeObject->GetActiveID(), activeID);
              // same object, somehow in different activeID now
              sameTypeObject->SetActiveID(activeID);
            }
          }
        }

        // inherit objectID from matches
        newActiveObjectPtr->CopyID(objectsOfSameType.front());
      }
      else
      {
        // there are no matches of the same type, set new objectID
        newActiveObjectPtr->SetID();
      }
    }
    else
    {
      // We can't find more than one object of the same type in a single origin. Otherwise something went really bad
      DEV_ASSERT(matchingObjects.size() <= 1,"BlockWorld.AddConnectedActiveObject.TooManyMatchingObjects" );

      // NOTE: [MAM] This error has not happened for some time, so I removed a lot of logic from this else block.
      // Find it again here if it is needed: https://github.com/anki/victor/blob/319a692ed2fc7cd85aa9009e415809cce827689a/engine/blockWorld/blockWorld.cpp#L1699
      // We should not find any objects in any origins that have this activeID. Otherwise that means they have
      // not disconnected properly. If there's a timing issue with connecting an object to an activeID before
      // disconnecting a previous object, we would like to know, so we can act accordingly. Add this error here
      // to detect that situation.
      LOG_ERROR("BlockWorld.AddConnectedActiveObject.ConflictingActiveID",
                "Objects with ActiveID:%d were found when we tried to add that activeID as connected object.",
                activeID);
    }

    // at this point the new active connected object has a valid objectID, we can finally add it to the world
    DEV_ASSERT( newActiveObjectPtr->GetID().IsSet(), "BlockWorld.AddConnectedActiveObject.ObjectIDWasNeverSet" );
    _connectedObjects.push_back(newActiveObjectPtr);

    // return the assigned objectID
    return newActiveObjectPtr->GetID();
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ObjectID BlockWorld::RemoveConnectedActiveObject(const ActiveID& activeID)
  {
    ObjectID removedObjectID;
    
    for (auto it = _connectedObjects.begin() ; it != _connectedObjects.end() ; ) {
      const auto& objPtr = *it;
      if (objPtr == nullptr) {
        LOG_ERROR("BlockWorld.RemoveConnectedActiveObject.NullEntryInConnectedObjects", "");
        it = _connectedObjects.erase(it);
      } else if (objPtr->GetActiveID() == activeID) {
        if (removedObjectID.IsSet()) {
          LOG_ERROR("BlockWorld.RemoveConnectedActiveObject.DuplicateEntry",
                    "Duplicate entry found in _connectedObjects for object with activeID %d. "
                    "Existing object ID %d, this object ID %d. Removing this entry as well",
                    activeID, removedObjectID.GetValue(), objPtr->GetID().GetValue());
        }
        removedObjectID = objPtr->GetID();
        it = _connectedObjects.erase(it);
      } else {
        ++it;
      }
    }

    // Clear the activeID from any located instances of the removed object
    if ( removedObjectID.IsSet() )
    {
      BlockWorldFilter matchingIDInAnyOrigin;
      matchingIDInAnyOrigin.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
      matchingIDInAnyOrigin.SetAllowedIDs({removedObjectID});
      ModifierFcn clearActiveID = [](ObservableObject* object) {
        object->SetActiveID(ObservableObject::InvalidActiveID);
        object->SetFactoryID(ObservableObject::InvalidFactoryID); // should not be needed. Should we keep it?
      };
      ModifyLocatedObjects(clearActiveID, matchingIDInAnyOrigin);
    }

    // return the objectID
    return removedObjectID;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::AddLocatedObject(const std::shared_ptr<ObservableObject>& object)
  {
    DEV_ASSERT(object->HasValidPose(), "BlockWorld.AddLocatedObject.NotAValidPoseState");
    DEV_ASSERT(object->GetID().IsSet(), "BlockWorld.AddLocatedObject.ObjectIDNotSet");

    const PoseOriginID_t objectOriginID = object->GetPose().GetRootID();

    // allow adding only in current origin
    DEV_ASSERT(objectOriginID == _robot->GetPoseOriginList().GetCurrentOriginID(),
               "BlockWorld.AddLocatedObject.NotCurrentOrigin");

    // hook activeID/factoryID if a connected object is available.
    // rsam: I would like to do this in a cleaner way, maybe just refactoring the code, but here seems fishy design-wise
    {
      // should not be connected if we are just adding to the world
      DEV_ASSERT(object->GetActiveID() == ObservableObject::InvalidActiveID,
                 "BlockWorld.AddLocatedObject.AlreadyHadActiveID");
      DEV_ASSERT(object->GetFactoryID() == ObservableObject::InvalidFactoryID,
                 "BlockWorld.AddLocatedObject.AlreadyHadFactoryID");

      // find by ObjectID. The objectID should match, since observations search for objectID even in connected
      ActiveObject* connectedObj = GetConnectedActiveObjectByID(object->GetID());
      if ( nullptr != connectedObj ) {
        object->SetActiveID( connectedObj->GetActiveID() );
        object->SetFactoryID( connectedObj->GetFactoryID() );
      }
    }

    // not asserting in case SDK tries to do this, but do not add it to the BlockWorld
    if(ObjectType::Block_LIGHTCUBE_GHOST == object->GetType())
    {
      PRINT_NAMED_ERROR("BlockWorld.AddLocatedObject.AddingGhostObject",
                        "Adding ghost objects to BlockWorld is not permitted");
      return;
    }

    // grab the current pointer and check it's empty (do not expect overwriting)
    auto& objectsInThisOrigin = _locatedObjects[objectOriginID];
    auto existingIt = FindInContainerWithID(objectsInThisOrigin, object->GetID());
    if (existingIt != objectsInThisOrigin.end()) {
      DEV_ASSERT(false, "BlockWorld.AddLocatedObject.ObjectIDInUseInOrigin");
      objectsInThisOrigin.erase(existingIt);
    }
    
    objectsInThisOrigin.push_back(object); // store the new object, this increments refcount

    // set the viz manager on this new object
    object->SetVizManager(_robot->GetContext()->GetVizManager());

    PRINT_CH_INFO("BlockWorld", "BlockWorld.AddLocatedObject",
                  "Adding new %s%s object and ID=%d ActID=%d FacID=%s at (%.1f, %.1f, %.1f), in frame %s.",
                  object->IsActive() ? "active " : "",
                  EnumToString(object->GetType()),
                  object->GetID().GetValue(),
                  object->GetActiveID(),
                  object->GetFactoryID().c_str(),
                  object->GetPose().GetTranslation().x(),
                  object->GetPose().GetTranslation().y(),
                  object->GetPose().GetTranslation().z(),
                  object->GetPose().FindRoot().GetName().c_str());

    // fire DAS event
    DASMSG(robot.object_located, "robot.object_located", "First time object has been seen in this origin");
    DASMSG_SET(s1, EnumToString(object->GetType()), "ObjectType");
    DASMSG_SET(s2, object->GetPose().FindRoot().GetName(), "Name of frame");
    DASMSG_SET(i1, object->GetID().GetValue(), "ObjectID");
    DASMSG_SEND();

    // make sure that everyone gets notified that there's a new object in town, I mean in this origin
    {
      const Pose3d* oldPosePtr = nullptr;
      const PoseState oldPoseState = PoseState::Invalid;
      _robot->GetObjectPoseConfirmer().BroadcastObjectPoseChanged(*object, oldPosePtr, oldPoseState);
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::OnObjectPoseChanged(const ObservableObject& object, const Pose3d* oldPose, PoseState oldPoseState)
  {
    const ObjectID& objectID = object.GetID();
    DEV_ASSERT(objectID.IsSet(), "BlockWorld.OnObjectPoseChanged.InvalidObjectID");

    // - - - - -
    // update the container that keeps track of changes per Update
    // - - - - -
    if ( _trackPoseChanges )
    {
      // find this object in the list of changes
      auto matchIDlambda = [&objectID](const PoseChange& a) { return a._id == objectID; };
      auto const matchIter = std::find_if(_objectPoseChangeList.begin(), _objectPoseChangeList.end(), matchIDlambda);
      const bool alreadyChanged = (matchIter != _objectPoseChangeList.end());
      if ( alreadyChanged ) {
        // this can happen if an object on top of a stack changes its pose. The bottom one can upon updating the
        // stack also try to change the top one, but that relative change will be ignored here because we
        // already knew the top object moved (that's one example of multiple pose changes that are valid.)
        PRINT_CH_INFO("BlockWorld", "BlockWorld.OnObjectPoseChanged.MultipleChanges",
                      "Object '%d' is changing its pose again this tick. Ignoring second change",
                      objectID.GetValue());
        // do not update old pose, conserve the original pose it had when it changed the first time
      }
      else
      {
        // if the old pose was valid add to the list of the changes. Otherwise this is the first time that
        // we see the object, so we don't need to update anything. Also oldPose will be nullptr in that case, useless.
        if ( ObservableObject::IsValidPoseState(oldPoseState) )
        {
          DEV_ASSERT(nullptr!=oldPose, "BlockWorld.OnObjectPoseChanged.ValidPoseStateNullPose");
          // add an entry at the end (this does not invalidate iterators or references to the current elements)
          _objectPoseChangeList.emplace_back( objectID, *oldPose, oldPoseState );
        }
        else
        {
          PRINT_CH_INFO("BlockWorld", "BlockWorld.OnObjectPoseChanged.FirstPoseForObject",
                        "Object '%d' is setting its first pose. Not queueing change.",
                        objectID.GetValue());
        }
      }
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::OnRobotDelocalized(PoseOriginID_t newWorldOriginID)
  {
    // delete objects that have become useless since we delocalized last time
    DeleteObjectsFromZombieOrigins();

    // create a new memory map for this origin
    _robot->GetMapComponent().CreateLocalizedMemoryMap(newWorldOriginID);

    // deselect blockworld's selected object, if it has one
    DeselectCurrentObject();

    // notify about updated object states
    BroadcastLocatedObjectStates();
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::SanityCheckBookkeeping() const
  {
    // Sanity check our containers to make sure each located object's properties
    // match the keys of the containers within which it is stored
    for(auto const& objectsByOrigin : _locatedObjects)
    {
      const auto& objects = objectsByOrigin.second;
      
      ANKI_VERIFY(!objects.empty(),
                  "BlockWorld.SanityCheckBookkeeping.NoObjectsInOrigin",
                  "OriginId: %d", objectsByOrigin.first);
      
      for(auto const& object : objects) {
        if (object == nullptr) {
          ANKI_VERIFY(false, "BlockWorld.SanityCheckBookkeeping.NullObject", "");
          continue;
        }
        const Pose3d& origin = object->GetPose().FindRoot();
        const PoseOriginID_t originID = origin.GetID();
        ANKI_VERIFY(PoseOriginList::UnknownOriginID != originID,
                    "BlockWorld.SanityCheckBookkeeping.ObjectWithUnknownOriginID",
                    "Origin: %s", origin.GetName().c_str());
        ANKI_VERIFY(objectsByOrigin.first == originID,
                    "BlockWorld.SanityCheckBookkeeping.MismatchedOrigin",
                    "%s Object %d is in Origin:%d but is keyed by Origin:%d",
                    EnumToString(object->GetType()), object->GetID().GetValue(),
                    originID, objectsByOrigin.first);
      }
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result BlockWorld::UpdateObservedMarkers(const std::list<Vision::ObservedMarker>& currentObsMarkers)
  {
    ANKI_CPU_PROFILE("BlockWorld::UpdateObservedMarkers");

    // clear the change list and start tracking them
    _objectPoseChangeList.clear();
    _trackPoseChanges = true;

    if(!currentObsMarkers.empty())
    {
      const RobotTimeStamp_t atTimestamp = currentObsMarkers.front().GetTimeStamp();
      _currentObservedMarkerTimestamp = atTimestamp;

      // Sanity check
      if(ANKI_DEVELOPER_CODE)
      {
        for(auto const& marker : currentObsMarkers)
        {
          if(marker.GetTimeStamp() != atTimestamp)
          {
            PRINT_NAMED_ERROR("BlockWorld.UpdateObservedMarkers.MisMatchedTimestamps", "Expected t=%u, Got t=%u",
                              (TimeStamp_t)atTimestamp, marker.GetTimeStamp());
            return RESULT_FAIL;
          }
        }
      }

      // New timestep, new set of occluders. Get rid of anything registered as
      // an occluder with the robot's camera
      _robot->GetVisionComponent().GetCamera().ClearOccluders();
      _robot->GetVisionComponent().AddLiftOccluder(atTimestamp);

      // Optional: don't allow markers seen enclosed in other markers
      // Note: If we still need this method at some future point, take a look at
      // https://github.com/anki/victor/blob/dce0730b201f1cd8f1cacdf7c101152c545c08ad/engine/blockWorld/blockWorld.cpp#L2193
      // or
      // https://github.com/anki/cozmo-one/blob/18ae758c6fd0f4f7b47356410a5612c1d46526bd/engine/blockWorld/blockWorld.cpp#L2356
      //RemoveMarkersWithinMarkers(currentObsMarkers);

      // Add, update, and/or localize the robot to any objects indicated by the
      // observed markers
      {
        // Sanity checks for robot's origin
        DEV_ASSERT(_robot->GetPose().IsChildOf(_robot->GetWorldOrigin()),
                   "BlockWorld.Update.RobotParentShouldBeOrigin");
        DEV_ASSERT(_robot->IsPoseInWorldOrigin(_robot->GetPose()),
                   "BlockWorld.Update.BadRobotOrigin");

        std::vector<ObservableObject*> objectsSeen;

        Result lastResult = _objectLibrary.CreateObjectsFromMarkers(currentObsMarkers, objectsSeen);
        if(lastResult != RESULT_OK) {
          PRINT_NAMED_ERROR("BlockWorld.UpdateObservedMarkers.CreateObjectsFromMarkersFailed", "");
          return lastResult;
        }

        lastResult = AddAndUpdateObjects(objectsSeen, atTimestamp);
        if(lastResult != RESULT_OK) {
          PRINT_NAMED_ERROR("BlockWorld.UpdateObservedMarkers.AddAndUpdateFailed", "");
          return lastResult;
        }
      }

      // Delete any objects that should have been observed but weren't,
      // visualize objects that were observed:
      CheckForUnobservedObjects(atTimestamp);
    }
    else
    {
      _currentObservedMarkerTimestamp = 0;

      const RobotTimeStamp_t lastImgTimestamp = _robot->GetLastImageTimeStamp();
      if(lastImgTimestamp > 0) // Avoid warning on first Update()
      {
        // Even if there were no markers observed, check to see if there are
        // any previously-observed objects that are partially visible (some part
        // of them projects into the image even if none of their markers fully do)
        _robot->GetVisionComponent().GetCamera().ClearOccluders();
        _robot->GetVisionComponent().AddLiftOccluder(lastImgTimestamp);
        CheckForUnobservedObjects(lastImgTimestamp);
      }
    }

    // do not track changes anymore, since we only use them to update stacks
    _trackPoseChanges = false;

#   define DISPLAY_ALL_OCCLUDERS 0
    if(DISPLAY_ALL_OCCLUDERS)
    {
      static Vision::Image dispOcc(240,320);
      dispOcc.FillWith(0);
      std::vector<Rectangle<f32>> occluders;
      _robot->GetVisionComponent().GetCamera().GetAllOccluders(occluders);
      for(auto const& rect : occluders)
      {
        std::vector<cv::Point2i> points{rect.GetTopLeft().get_CvPoint_(), rect.GetTopRight().get_CvPoint_(),
          rect.GetBottomRight().get_CvPoint_(), rect.GetBottomLeft().get_CvPoint_()};
        cv::fillConvexPoly(dispOcc.get_CvMat_(), points, 255);
      }
      dispOcc.Display("Occluders");
    }

    //PRINT_CH_INFO("BlockWorld", "BlockWorld.Update.NumBlocksObserved", "Saw %d blocks", numBlocksObserved);


    // Check for unobserved, uncarried objects that overlap with the robot's position,
    // and mark them as dirty.
    // For now we aren't worrying about collision with the robot while picking or placing since
    // we are trying to get close to objects in these modes.
    // TODO: Enable collision checking while picking and placing too
    // (I feel like we should be able to always do this, since we _do_ check that the
    //  object is not the docking object as part of the filter, and we do height checks as well.
    //  But I'm wary of changing it now either...)
    if(!_robot->GetDockingComponent().IsPickingOrPlacing())
    {
      BlockWorldFilter unobservedCollidingObjectFilter;
      unobservedCollidingObjectFilter.SetOriginMode(BlockWorldFilter::OriginMode::InRobotFrame);
      unobservedCollidingObjectFilter.SetFilterFcn([this](const ObservableObject* object) {
        return CheckForCollisionWithRobot(object);
      });

      ModifierFcn markAsDirty = [this](ObservableObject* object)
      {
        // Mark object and everything on top of it as "dirty". Next time we look
        // for it and don't see it, we will fully clear it and mark it as "unknown"
        _robot->GetObjectPoseConfirmer().MarkObjectDirty(object);
      };

      ModifyLocatedObjects(markAsDirty, unobservedCollidingObjectFilter);
    }

    if(ANKI_DEVELOPER_CODE)
    {
      SanityCheckBookkeeping();
    }

    return RESULT_OK;

  } // UpdateObservedMarkers()

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool BlockWorld::CheckForCollisionWithRobot(const ObservableObject* object) const
  {
    // If this object is _allowed_ to intersect with the robot, no reason to
    // check anything
    if(object->CanIntersectWithRobot()) {
      return false;
    }

    // If object was observed, it must be there, so don't check for collision
    const bool wasObserved = object->GetLastObservedTime() >= _robot->GetLastImageTimeStamp();
    if(wasObserved) {
      return false;
    }

    // Only check objects that are in accurate/known pose state
    if(!object->IsPoseStateKnown()) {
      return false;
    }

    const ObjectID& objectID = object->GetID();

    // Don't worry about collision with an object being carried or that we are
    // docking with, since we are expecting to be in close proximity to either
    const bool isCarryingObject = _robot->GetCarryingComponent().IsCarryingObject(objectID);
    const bool isDockingWithObject = _robot->GetDockingComponent().GetDockObject() == objectID;
    if(isCarryingObject || isDockingWithObject) {
      return false;
    }

    // Check block's bounding box in same coordinates as this robot to
    // see if it intersects with the robot's bounding box. Also check to see
    // block and the robot are at overlapping heights.  Skip this check
    // entirely if the block isn't in the same coordinate tree as the
    // robot.
    Pose3d objectPoseWrtRobotOrigin;
    if(false == object->GetPose().GetWithRespectTo(_robot->GetWorldOrigin(), objectPoseWrtRobotOrigin))
    {
      PRINT_NAMED_WARNING("BlockWorld.CheckForCollisionWithRobot.BadOrigin",
                          "Could not get %s %d pose (origin: %s) w.r.t. robot origin (%s)",
                          EnumToString(object->GetType()), objectID.GetValue(),
                          object->GetPose().FindRoot().GetName().c_str(),
                          _robot->GetWorldOrigin().GetName().c_str());
      return false;
    }

    // Check if the object is in the same plane as the robot
    // Note: we pad the robot's height by the object's half-height and then
    //       just treat the object as a point (similar to configuration-space
    //       expansion we do for the planner)
    const f32 objectHalfZDim = 0.5f*object->GetDimInParentFrame<'Z'>();
    const f32 objectHeight   = objectPoseWrtRobotOrigin.GetTranslation().z();
    const f32 robotBottom    = _robot->GetPose().GetTranslation().z();
    const f32 robotTop       = robotBottom + ROBOT_BOUNDING_Z;

    const bool inSamePlane = ((objectHeight >= (robotBottom - objectHalfZDim)) &&
                              (objectHeight <= (robotTop + objectHalfZDim)));

    if(!inSamePlane) {
      return false;
    }

    // Check if the object's bounding box intersects the robot's
    const Quad2f objectBBox = object->GetBoundingQuadXY(objectPoseWrtRobotOrigin);
    const Quad2f robotBBox = _robot->GetBoundingQuadXY(_robot->GetPose().GetWithRespectToRoot(),
                                                       ROBOT_BBOX_PADDING_FOR_OBJECT_COLLISION);

    const bool bboxIntersects = robotBBox.Intersects(objectBBox);
    if(bboxIntersects)
    {
      PRINT_CH_INFO("BlockWorld", "BlockWorld.CheckForCollisionWithRobot.ObjectRobotIntersection",
                    "Marking object %s %d as 'dirty', because it intersects robot's bounding quad.",
                    EnumToString(object->GetType()), object->GetID().GetValue());

      return true;
    }

    return false;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::ClearLocatedObjectHelper(ObservableObject* object)
  {
    if(object == nullptr) {
      PRINT_NAMED_WARNING("BlockWorld.ClearObjectHelper.NullObjectPointer",
                          "BlockWorld asked to clear a null object pointer.");
      return;
    }

    // Check to see if this object is the one the robot is localized to.
    // If so, the robot needs to be marked as localized to nothing.
    if(_robot->GetLocalizedTo() == object->GetID()) {
      PRINT_CH_INFO("BlockWorld", "BlockWorld.ClearObjectHelper.LocalizeRobotToNothing",
                    "Setting robot as localized to no object, because it "
                    "is currently localized to %s object with ID=%d, which is "
                    "about to be cleared.",
                    ObjectTypeToString(object->GetType()), object->GetID().GetValue());
      _robot->SetLocalizedTo(nullptr);
    }

    // Check to see if this object is the one the robot is carrying.
    if(_robot->GetCarryingComponent().GetCarryingObjectID() == object->GetID()) {
      PRINT_CH_INFO("BlockWorld", "BlockWorld.ClearObjectHelper.ClearingCarriedObject",
                    "Clearing %s object %d which robot thinks it is carrying.",
                    ObjectTypeToString(object->GetType()),
                    object->GetID().GetValue());
      _robot->GetCarryingComponent().UnSetCarryingObject();
    }

    if(_selectedObjectID == object->GetID()) {
      PRINT_CH_INFO("BlockWorld", "BlockWorld.ClearObjectHelper.ClearingSelectedObject",
                    "Clearing %s object %d which is currently selected.",
                    ObjectTypeToString(object->GetType()),
                    object->GetID().GetValue());
      _selectedObjectID.UnSet();
    }
  }


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::FindConnectedActiveMatchingObjects(const BlockWorldFilter& filter, std::vector<const ActiveObject*>& result) const
  {
    // slight abuse of the FindObjectHelper, I just use it for filtering, then I add everything that passes
    // the filter to the result vector
    ModifierFcn addToResult = [&result](ObservableObject* candidateObject) {
      // TODO this could be a checked_cast (dynamic in dev, static in shipping)
      const ActiveObject* candidateActiveObject = dynamic_cast<ActiveObject*>(candidateObject);
      result.push_back(candidateActiveObject);
    };

    // ignore return value, since the findLambda stored everything in result
    FindConnectedObjectHelper(filter, addToResult, false);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::FindConnectedActiveMatchingObjects(const BlockWorldFilter& filter, std::vector<ActiveObject*>& result)
  {
    // slight abuse of the FindObjectHelper, I just use it for filtering, then I add everything that passes
    // the filter to the result vector
    ModifierFcn addToResult = [&result](ObservableObject* candidateObject) {
      ActiveObject* candidateActiveObject = dynamic_cast<ActiveObject*>(candidateObject);
      result.push_back(candidateActiveObject);
    };

    // ignore return value, since the findLambda stored everything in result
    FindConnectedObjectHelper(filter, addToResult, false);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::FindLocatedMatchingObjects(const BlockWorldFilter& filter, std::vector<ObservableObject*>& result)
  {
    // slight abuse of the FindLocatedObjectHelper, I just use it for filtering, then I add everything that passes
    // the filter to the result vector
    ModifierFcn addToResult = [&result](ObservableObject* candidateObject) {
      result.push_back(candidateObject);
    };

    // ignore return value, since the findLambda stored everything in result
    FindLocatedObjectHelper(filter, addToResult, false);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::FindLocatedMatchingObjects(const BlockWorldFilter& filter, std::vector<const ObservableObject*>& result) const
  {
    // slight abuse of the FindLocatedObjectHelper, I just use it for filtering, then I add everything that passes
    // the filter to the result vector
    ModifierFcn addToResult = [&result](ObservableObject* candidateObject) {
      result.push_back(candidateObject);
    };

    // ignore return value, since the findLambda stored everything in result
    FindLocatedObjectHelper(filter, addToResult, false);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  const ObservableObject* BlockWorld::FindMostRecentlyObservedObject(const BlockWorldFilter& filterIn) const
  {
    RobotTimeStamp_t bestTime = 0;

    BlockWorldFilter filter(filterIn);
    filter.AddFilterFcn([&bestTime](const ObservableObject* current)
    {
      const RobotTimeStamp_t currentTime = current->GetLastObservedTime();
      if(currentTime > bestTime) {
        bestTime = currentTime;
        return true;
      } else {
        return false;
      }
    });

    return FindLocatedObjectHelper(filter);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  namespace {

  // Helper to create a filter common to several methods below
  static inline BlockWorldFilter GetIntersectingObjectsFilter(const Quad2f& quad, f32 padding_mm,
                                                              const BlockWorldFilter& filterIn)
  {
    BlockWorldFilter filter(filterIn);
    filter.AddFilterFcn([&quad,padding_mm](const ObservableObject* object) {
      // Get quad of object and check for intersection
      Quad2f quadExist = object->GetBoundingQuadXY(object->GetPose(), padding_mm);
      if( quadExist.Intersects(quad) ) {
        return true;
      } else {
        return false;
      }
    });

    return filter;
  }
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::FindLocatedIntersectingObjects(const ObservableObject* objectSeen,
                                           std::vector<const ObservableObject*>& intersectingExistingObjects,
                                           f32 padding_mm,
                                           const BlockWorldFilter& filter) const
  {
    Quad2f quadSeen = objectSeen->GetBoundingQuadXY(objectSeen->GetPose(), padding_mm);
    FindLocatedMatchingObjects(GetIntersectingObjectsFilter(quadSeen, padding_mm, filter), intersectingExistingObjects);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::FindLocatedIntersectingObjects(const ObservableObject* objectSeen,
                                           std::vector<ObservableObject*>& intersectingExistingObjects,
                                           f32 padding_mm,
                                           const BlockWorldFilter& filter)
  {
    Quad2f quadSeen = objectSeen->GetBoundingQuadXY(objectSeen->GetPose(), padding_mm);
    FindLocatedMatchingObjects(GetIntersectingObjectsFilter(quadSeen, padding_mm, filter), intersectingExistingObjects);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::FindLocatedIntersectingObjects(const Quad2f& quad,
                                           std::vector<const ObservableObject *> &intersectingExistingObjects,
                                           f32 padding_mm,
                                           const BlockWorldFilter& filterIn) const
  {
    FindLocatedMatchingObjects(GetIntersectingObjectsFilter(quad, padding_mm, filterIn), intersectingExistingObjects);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::FindLocatedIntersectingObjects(const Quad2f& quad,
                                           std::vector<ObservableObject *> &intersectingExistingObjects,
                                           f32 padding_mm,
                                           const BlockWorldFilter& filterIn)
  {
    FindLocatedMatchingObjects(GetIntersectingObjectsFilter(quad, padding_mm, filterIn), intersectingExistingObjects);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::GetLocatedObjectBoundingBoxesXY(const f32 minHeight, const f32 maxHeight, const f32 padding,
                                                   std::vector<std::pair<Quad2f,ObjectID> >& rectangles,
                                                   const BlockWorldFilter& filterIn) const
  {
    BlockWorldFilter filter(filterIn);

    // Note that we add this filter function, meaning we still rely on the
    // default filter function which rules out objects with unknown pose state
    filter.AddFilterFcn([minHeight, maxHeight, padding, &rectangles](const ObservableObject* object)
    {
      const Point3f rotatedSize( object->GetPose().GetRotation() * object->GetSize() );
      const f32 objectCenter = object->GetPose().GetWithRespectToRoot().GetTranslation().z();

      const f32 objectTop = objectCenter + (0.5f * rotatedSize.z());
      const f32 objectBottom = objectCenter - (0.5f * rotatedSize.z());

      const bool bothAbove = (objectTop >= maxHeight) && (objectBottom >= maxHeight);
      const bool bothBelow = (objectTop <= minHeight) && (objectBottom <= minHeight);

      if( !bothAbove && !bothBelow )
      {
        rectangles.emplace_back(object->GetBoundingQuadXY(padding), object->GetID());
        return true;
      }

      return false;
    });

    FindLocatedObjectHelper(filter);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::GetObstacles(std::vector<std::pair<Quad2f,ObjectID> >& boundingBoxes, const f32 padding) const
  {
    BlockWorldFilter filter;
    if (_robot->GetCarryingComponent().IsCarryingObject()) {
      filter.SetIgnoreIDs({{_robot->GetCarryingComponent().GetCarryingObjectID()}});
    }

    // Figure out height filters in world coordinates (because GetLocatedObjectBoundingBoxesXY()
    // uses heights of objects in world coordinates)
    const Pose3d robotPoseWrtOrigin = _robot->GetPose().GetWithRespectToRoot();
    const f32 minHeight = robotPoseWrtOrigin.GetTranslation().z();
    const f32 maxHeight = minHeight + _robot->GetHeight();

    GetLocatedObjectBoundingBoxesXY(minHeight, maxHeight, padding, boundingBoxes, filter);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool BlockWorld::IsZombiePoseOrigin(PoseOriginID_t originID) const
  {
    // really, pass in a valid origin ID
    DEV_ASSERT(_robot->GetPoseOriginList().ContainsOriginID(originID), "BlockWorld.IsZombiePoseOrigin.InvalidOriginID");

    // current world is not a zombie
    const bool isCurrent = (originID == _robot->GetPoseOriginList().GetCurrentOriginID());
    if ( isCurrent ) {
      return false;
    }

    // check if there are any objects we can localize to
    const bool hasLocalizableObjects = AnyRemainingLocalizableObjects(originID);
    const bool isZombie = !hasLocalizableObjects;
    return isZombie;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool BlockWorld::AnyRemainingLocalizableObjects() const
  {
    return AnyRemainingLocalizableObjects(PoseOriginList::UnknownOriginID);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool BlockWorld::AnyRemainingLocalizableObjects(PoseOriginID_t originID) const
  {
    // Filter out anything that can't be used for localization 
    BlockWorldFilter filter;
    filter.SetFilterFcn([](const ObservableObject* obj) {
      return obj->CanBeUsedForLocalization();
    });

    // Allow all origins if origin is nullptr or allow only the specified origin
    filter.SetOriginMode(BlockWorldFilter::OriginMode::Custom);
    if(originID != PoseOriginList::UnknownOriginID) {
      filter.AddAllowedOrigin(originID);
    }

    if(nullptr != FindLocatedObjectHelper(filter, nullptr, true)) {
      return true;
    } else {
      return false;
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::DeleteLocatedObjects(const BlockWorldFilter& filter)
  {
    // cache object info since we are going to destroy the objects
    struct DeletedObjectInfo {
      DeletedObjectInfo(const Pose3d& oldPose, const PoseState oldPoseState, const ObservableObject* objCopy)
      : _oldPose(oldPose), _oldPoseState(oldPoseState), _objectCopy(objCopy) { }
      const Pose3d _oldPose;
      const PoseState _oldPoseState;
      const ObservableObject* _objectCopy;
    };
    std::vector<DeletedObjectInfo> objectsToBroadcast;

    auto originIter = _locatedObjects.begin();
    while(originIter != _locatedObjects.end())
    {
      const PoseOriginID_t crntOriginID = originIter->first;
      auto& objectContainer = originIter->second;

      if(filter.ConsiderOrigin(crntOriginID, _robot->GetPoseOriginList().GetCurrentOriginID()))
      {
        for (auto objectIter = objectContainer.begin() ; objectIter != objectContainer.end() ; ) {
          auto* object = objectIter->get();
          if (object == nullptr) {
            LOG_ERROR("BlockWorld.DeleteLocatedObjects.NullObject", "origin %d", crntOriginID);
            objectIter = objectContainer.erase(objectIter);
          } else if (filter.ConsiderType(object->GetType()) &&
                     filter.ConsiderObject(object)) {
            // clear objects in current origin (others should not be needed)
            const bool isCurrentOrigin = (crntOriginID == _robot->GetPoseOriginList().GetCurrentOriginID());
            if ( isCurrentOrigin )
            {
              ClearLocatedObjectHelper(object);

              // Create a copy of the object so we can notify listeners
              {
                DEV_ASSERT(object->HasValidPose(), "BlockWorld.DeleteLocatedObjects.InvalidPoseState");
                ObservableObject* objCopy = object->CloneType();
                objCopy->CopyID(object);
                if (objCopy->IsActive()) {
                  objCopy->SetActiveID(object->GetActiveID()); // manually having to copy all IDs is fishy design
                  objCopy->SetFactoryID(object->GetFactoryID());
                }
                objectsToBroadcast.emplace_back( object->GetPose(), object->GetPoseState(), objCopy );
              }
            }

            objectIter = objectContainer.erase(objectIter);
          } else {
            ++objectIter;
          }
        }
      }
      
      if(objectContainer.empty()) {
        // All objects removed from this origin, erase it
        originIter = _locatedObjects.erase(originIter);
      } else {
        ++originIter;
      }
    }

    // notify of the deleted objects
    for(auto& objectDeletedInfo : objectsToBroadcast)
    {
      using namespace ExternalInterface;

      // cache values
      const ObjectID& deletedID = objectDeletedInfo._objectCopy->GetID(); // note this won't be valid after delete
      const Pose3d* oldPosePtr = &objectDeletedInfo._oldPose;
      const PoseState oldPoseState = objectDeletedInfo._oldPoseState;

      // tell PoseConfirmer this object is not confirmed anymore (in case we delete without unobserving)
      _robot->GetObjectPoseConfirmer().DeletedObjectInCurrentOrigin(deletedID);

      // PoseConfirmer::PoseChanged (should not have valid pose)
      DEV_ASSERT(!objectDeletedInfo._objectCopy->HasValidPose(), "ObjectPoseConfirmer.CopyInheritedPose");
      _robot->GetObjectPoseConfirmer().BroadcastObjectPoseChanged(*objectDeletedInfo._objectCopy, oldPosePtr, oldPoseState);

      RobotDeletedLocatedObject msg(deletedID);

      if( ANKI_DEV_CHEATS ) {
        SendObjectUpdateToWebViz( msg );
      }

      // RobotDeletedLocatedObject
      _robot->Broadcast(MessageEngineToGame(std::move(msg)));

      // delete the copy we made now, since it won't be useful anymore
      Util::SafeDelete(objectDeletedInfo._objectCopy);
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::DeselectCurrentObject()
  {
    if(_selectedObjectID.IsSet())
    {
      if(ENABLE_DRAWING)
      {
        // Erase the visualization of the selected object's preaction poses/lines. Note we do this
        // across all frames in case the selected object is in a different origin and we have delocalized
        BlockWorldFilter filter;
        filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
        filter.AddAllowedID(_selectedObjectID);

        const ObservableObject* object = FindLocatedMatchingObject(filter);
        if(nullptr != object) {
          object->EraseVisualization();
        }
      }
      _selectedObjectID.UnSet();
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::ClearLocatedObjectByIDInCurOrigin(const ObjectID& withID)
  {
    ObservableObject* object = GetLocatedObjectByID(withID);
    ClearLocatedObjectHelper(object);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::ClearLocatedObject(ObservableObject* object)
  {
    ClearLocatedObjectHelper(object);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool BlockWorld::SelectObject(const ObjectID& objectID)
  {
    ObservableObject* newSelection = GetLocatedObjectByID(objectID);

    if(newSelection != nullptr) {
      // Unselect current object of interest, if it still exists (Note that it may just get
      // reselected here, but I don't think we care.)
      DeselectCurrentObject();

      // Record new object of interest as selected so it will draw differently
      _selectedObjectID = objectID;
      PRINT_CH_INFO("BlockWorld", "BlockWorld.SelectObject",
                    "Selected Object with ID=%d", objectID.GetValue());

      return true;
    } else {
      PRINT_CH_INFO("BlockWorld", "BlockWorld.SelectObject.InvalidID",
                    "Object with ID=%d not found. Not updating selected object.", objectID.GetValue());
      return false;
    }
  } // SelectObject()

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::CycleSelectedObject()
  {
    bool currSelectedObjectFound = false;
    bool newSelectedObjectSet = false;

    // Iterate through all the objects
    BlockWorldFilter filter;
    std::vector<ObservableObject*> allObjects;
    FindLocatedMatchingObjects(filter, allObjects);
    for(auto const & obj : allObjects)
    {
      ActionableObject* object = dynamic_cast<ActionableObject*>(obj);
      if(object != nullptr &&
         !_robot->GetCarryingComponent().IsCarryingObject(object->GetID()))
      {
        //PRINT_INFO("currID: %d", block.first);
        if (currSelectedObjectFound) {
          // Current block of interest has been found.
          // Set the new block of interest to the next block in the list.
          _selectedObjectID = object->GetID();
          newSelectedObjectSet = true;
          //PRINT_INFO("new block found: id %d  type %d", block.first, blockType.first);
          break;
        } else if (object->GetID() == _selectedObjectID) {
          currSelectedObjectFound = true;
          if(ENABLE_DRAWING)
          {
            // Erase the visualization of the current selection so we can draw only the
            // the new one (even if we end up just re-drawing this one)
            object->EraseVisualization();
          }
          //PRINT_INFO("curr block found: id %d  type %d", block.first, blockType.first);
        }
      }

      if (newSelectedObjectSet) {
        break;
      }
    } // for all objects

    // If the current object of interest was found, but a new one was not set
    // it must have been the last block in the map. Set the new object of interest
    // to the first object in the map as long as it's not the same object.
    if (!currSelectedObjectFound || !newSelectedObjectSet) {

      // Find first object
      ObjectID firstObject; // initialized to un-set
      for(auto const & obj : allObjects) {
        const ActionableObject* object = dynamic_cast<ActionableObject*>(obj);
        if(object != nullptr &&
           !_robot->GetCarryingComponent().IsCarryingObject(object->GetID()))
        {
          firstObject = obj->GetID();
          break;
        }
      }


      if (firstObject == _selectedObjectID || !firstObject.IsSet()){
        //PRINT_INFO("Only one object in existence.");
      } else {
        //PRINT_INFO("Setting object of interest to first block");
        _selectedObjectID = firstObject;
      }
    }

    if(_selectedObjectID.IsSet())
    {
      DEV_ASSERT(nullptr != GetLocatedObjectByID(_selectedObjectID),
                 "BlockWorld.CycleSelectedObject.ObjectNotFound");
      PRINT_CH_DEBUG("BlockWorld", "BlockWorld.CycleSelectedObject",
                     "Object of interest: ID = %d",_selectedObjectID.GetValue());
    }
    else
    {
      PRINT_CH_DEBUG("BlockWorld", "BlockWorld.CycleSelectedObject.NoObject", "No object of interest found");
    }

  } // CycleSelectedObject()

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::DrawAllObjects() const
  {
    if(!ENABLE_DRAWING) {
      // Don't draw anything in shipping builds
      return;
    }

    auto webSender = WebService::WebVizSender::CreateWebVizSender("navmap", _robot->GetContext()->GetWebService());

    const ObjectID& locObject = _robot->GetLocalizedTo();

    // Note: only drawing objects in current coordinate frame!
    BlockWorldFilter filter;
    filter.SetOriginMode(BlockWorldFilter::OriginMode::InRobotFrame);
    ModifierFcn visualizeHelper = [this,&locObject,webSender](ObservableObject* object)
    {
      if(object->GetID() == _selectedObjectID) {
        // Draw selected object in a different color and draw its pre-action poses
        object->Visualize(NamedColors::SELECTED_OBJECT);

        const ActionableObject* selectedObject = dynamic_cast<const ActionableObject*>(object);
        if(selectedObject == nullptr) {
          PRINT_NAMED_WARNING("BlockWorld.DrawAllObjects.NullSelectedObject",
                              "Selected object ID = %d, but it came back null.",
                              GetSelectedObject().GetValue());
        } else {
          std::vector<std::pair<Quad2f,ObjectID> > obstacles;
          _robot->GetBlockWorld().GetObstacles(obstacles);
          selectedObject->VisualizePreActionPoses(obstacles, _robot->GetPose());
        }
      }
      else if(object->GetID() == locObject) {
        // Draw object we are localized to in a different color
        object->Visualize(NamedColors::LOCALIZATION_OBJECT);
      }
      else if(object->GetPoseState() == PoseState::Dirty) {
        // Draw dirty objects in a special color
        object->Visualize(NamedColors::DIRTY_OBJECT);
      }
      else if(object->GetPoseState() == PoseState::Invalid) {
        // Draw unknown objects in a special color
        object->Visualize(NamedColors::UNKNOWN_OBJECT);
      }
      else {
        // Draw "regular" objects in current frame in their internal color
        object->Visualize();
      }

      if( webSender ) {
        if( IsValidLightCube(object->GetType(), false) ) {
          Json::Value cubeInfo;
          const auto& pose = object->GetPose();
          cubeInfo["x"] = pose.GetTranslation().x();
          cubeInfo["y"] = pose.GetTranslation().y();
          cubeInfo["z"] = pose.GetTranslation().z();
          cubeInfo["angle"] = pose.GetRotationAngle<'Z'>().ToFloat();
          webSender->Data()["cubes"].append( cubeInfo );
        }
      }
    };

    FindLocatedObjectHelper(filter, visualizeHelper, false);

    // don't fill type unless there's some actual data (to avoid unnecessary sends)
    if( webSender && !webSender->Data().empty() ) {
      webSender->Data()["type"] = "MemoryMapCubes";
    }
  } // DrawAllObjects()

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::SendObjectUpdateToWebViz( const ExternalInterface::RobotDeletedLocatedObject& msg ) const
  {
    if( auto webSender = WebService::WebVizSender::CreateWebVizSender("observedobjects",
                                                                      _robot->GetContext()->GetWebService()) ) {
      webSender->Data()["type"] = "RobotDeletedLocatedObject";
      webSender->Data()["objectID"] = msg.objectID;
    }

  } // SendObjectUpdateToWebViz()

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::SendObjectUpdateToWebViz( const ExternalInterface::RobotObservedObject& msg ) const
  {
    if( auto webSender = WebService::WebVizSender::CreateWebVizSender("observedobjects",
                                                                      _robot->GetContext()->GetWebService()) ) {
      webSender->Data()["type"] = "RobotObservedObject";
      webSender->Data()["objectID"] = msg.objectID;
      webSender->Data()["objectType"] = ObjectTypeToString(msg.objectType);
      webSender->Data()["isActive"] = msg.isActive;
      webSender->Data()["timestamp"] = msg.timestamp;
    }

  } // SendObjectUpdateToWebViz()

} // namespace Vector
} // namespace Anki
