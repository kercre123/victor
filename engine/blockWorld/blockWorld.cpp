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

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/poseOriginList.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/rect_impl.h"
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
#include "engine/flatMat.h"
#include "engine/humanHead.h"
#include "engine/markerlessObject.h"
#include "engine/mat.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_Cliff.h"
#include "engine/objectPoseConfirmer.h"
#include "engine/platform.h"
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
#include "util/math/math.h"
#include "webServerProcess/src/webVizSender.h"

// TODO: Expose these as parameters
#define BLOCK_IDENTIFICATION_TIMEOUT_MS 500

#define DEBUG_ROBOT_POSE_UPDATES 0
#if DEBUG_ROBOT_POSE_UPDATES
#  define PRINT_LOCALIZATION_INFO(...) PRINT_NAMED_INFO("Localization", __VA_ARGS__)
#else
#  define PRINT_LOCALIZATION_INFO(...)
#endif

// Giving this its own local define, in case we want to control it independently of DEV_CHEATS / SHIPPING, etc.
#define ENABLE_DRAWING ANKI_DEV_CHEATS


namespace Anki {
namespace Vector {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// CONSOLE VARS

// Enable to draw (semi-experimental) bounding cuboids around stacks of 2 blocks
CONSOLE_VAR(bool, kVisualizeStacks, "BlockWorld", false);

// How long to wait until deleting non-cliff Markerless objects
CONSOLE_VAR(u32, kMarkerlessObjectExpirationTime_ms, "BlockWorld", 30000);

// How "recently" a cube can be seen for it not to get updated via UpdateStacks
CONSOLE_VAR(u32, kRecentlySeenTimeForStackUpdate_ms, "BlockWorld", 100);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  BlockWorld::BlockWorld()
  : UnreliableComponent<BCComponentID>(this, BCComponentID::BlockWorld)
  , IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::BlockWorld)
  , _lastPlayAreaSizeEventSec(0)
  , _playAreaSizeEventIntervalSec(60)
  , _didObjectsChange(false)
  , _robotMsgTimeStampAtChange(0)
  , _trackPoseChanges(false)
  {
  } // BlockWorld() Constructor

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::InitDependent(Robot* robot, const RobotCompMap& dependentComps)
  {
    _robot = robot;
    DEV_ASSERT(_robot != nullptr, "BlockWorld.Constructor.InvalidRobot");


    // TODO: Create each known block / matpiece from a configuration/definitions file

    //////////////////////////////////////////////////////////////////////////
    // 1x1 Cubes
    //

    //blockLibrary_.AddObject(new Block_Cube1x1(Block::FUEL_BLOCK_TYPE));

    /*
    _objectLibrary[ObjectFamily::Block].AddObject(new Block_Cube1x1(ObjectType::Block_ANGRYFACE));

    _objectLibrary[ObjectFamily::Block].AddObject(new Block_Cube1x1(ObjectType::Block_BULLSEYE2));
    _objectLibrary[ObjectFamily::Block].AddObject(new Block_Cube1x1(ObjectType::Block_BULLSEYE2_INVERTED));

    _objectLibrary[ObjectFamily::Block].AddObject(new Block_Cube1x1(ObjectType::Block_SQTARGET));

    _objectLibrary[ObjectFamily::Block].AddObject(new Block_Cube1x1(ObjectType::Block_FIRE));

    _objectLibrary[ObjectFamily::Block].AddObject(new Block_Cube1x1(ObjectType::Block_ANKILOGO));

    _objectLibrary[ObjectFamily::Block].AddObject(new Block_Cube1x1(ObjectType::Block_STAR5));
    */

    //_objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(ObjectType::Block_DICE));

    /*
    _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(ObjectType::Block_NUMBER1));
    _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(ObjectType::Block_NUMBER2));
    _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(ObjectType::Block_NUMBER3));
    _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(ObjectType::Block_NUMBER4));
    _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(ObjectType::Block_NUMBER5));
    _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(ObjectType::Block_NUMBER6));
     */
    //_objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(ObjectType::Block_BANGBANGBANG));

    /*
    _objectLibrary[ObjectFamily::Block].AddObject(new Block_Cube1x1(ObjectType::Block_ARROW));

    _objectLibrary[ObjectFamily::Block].AddObject(new Block_Cube1x1(ObjectType::Block_FLAG));
    _objectLibrary[ObjectFamily::Block].AddObject(new Block_Cube1x1(ObjectType::Block_FLAG2));
    _objectLibrary[ObjectFamily::Block].AddObject(new Block_Cube1x1(ObjectType::Block_FLAG_INVERTED));

    // For CREEP Test
    _objectLibrary[ObjectFamily::Block].AddObject(new Block_Cube1x1(ObjectType::Block_SPIDER));
    _objectLibrary[ObjectFamily::Block].AddObject(new Block_Cube1x1(ObjectType::Block_KITTY));
    _objectLibrary[ObjectFamily::Block].AddObject(new Block_Cube1x1(ObjectType::Block_BEE));
    */

    //////////////////////////////////////////////////////////////////////////
    // 1x1 Light Cubes
    //
    DefineObject(std::make_unique<ActiveCube>(ObjectType::Block_LIGHTCUBE1));
    DefineObject(std::make_unique<ActiveCube>(ObjectType::Block_LIGHTCUBE2));
    DefineObject(std::make_unique<ActiveCube>(ObjectType::Block_LIGHTCUBE3));

    //////////////////////////////////////////////////////////////////////////
    // 2x1 Blocks
    //

    //_objectLibrary[ObjectFamily::Block].AddObject(new Block_2x1(ObjectType::Block_BANGBANGBANG));


    //////////////////////////////////////////////////////////////////////////
    // Mat Pieces
    //

    // Flat mats:
    //_objectLibrary[ObjectFamily::Mat].AddObject(new FlatMat(ObjectType::FlatMat_LETTERS_4x4));
    //_objectLibrary[ObjectFamily::Mat].AddObject(new FlatMat(ObjectType::FlatMat_GEARS_4x4));

    // Platform piece:
    //_objectLibrary[ObjectFamily::Mat].AddObject(new Platform(Platform::Type::LARGE_PLATFORM));


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
    const ObjectFamily objFamily = object->GetFamily();

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
      else
      {
        ++_definedObjectTypeCount[objFamily];
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
    filter.AddAllowedFamily(ObjectFamily::CustomObject);
    filter.AddAllowedType(ObjectType::CustomFixedObstacle);
    DeleteLocatedObjects(filter);
    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::RobotDeletedFixedCustomObjects>();
  }

  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::DeleteCustomMarkerObjects& msg)
  {
    BlockWorldFilter filter;
    filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
    filter.AddAllowedFamily(ObjectFamily::CustomObject);
    filter.AddIgnoreType(ObjectType::CustomFixedObstacle); // everything custom _except_ fixed obstacles
    DeleteLocatedObjects(filter);
    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::RobotDeletedCustomMarkerObjects>();
  }

  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::DeleteAllCustomObjects& msg)
  {
    BlockWorldFilter filter;
    filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
    filter.AddAllowedFamily(ObjectFamily::CustomObject);
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

    DEV_ASSERT(numRemoved <= _definedObjectTypeCount[ObjectFamily::CustomObject],
      "BlockWorld.UndefineAllCustomObjects.RemovingTooManyObjectTypes");

    _definedObjectTypeCount[ObjectFamily::CustomObject] -= numRemoved;

    DEV_ASSERT(_definedObjectTypeCount[ObjectFamily::CustomObject] == 0,
               "BlockWorld.UndefineAllCustomObjects.NonZeroObjectCount");

    PRINT_NAMED_INFO("BlockWorld.HandleMessage.UndefineAllCustomObjects",
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

    for(const auto & objectsByOrigin : _locatedObjects) {
      if(filter.ConsiderOrigin(objectsByOrigin.first, _robot->GetPoseOriginList().GetCurrentOriginID())) {
        for(const auto & objectsByFamily : objectsByOrigin.second) {
          if(filter.ConsiderFamily(objectsByFamily.first)) {
            for(const auto & objectsByType : objectsByFamily.second) {
              if(filter.ConsiderType(objectsByType.first)) {
                for(const auto & objectsByID : objectsByType.second) {
                  ObservableObject* object_nonconst = objectsByID.second.get();
                  const ObservableObject* object = object_nonconst;

                  if(nullptr == object)
                  {
                    PRINT_NAMED_WARNING("BlockWorld.FindObjectHelper.NullExistingObject",
                                        "Origin:%s Family:%s Type:%s ID:%d is NULL",
                                        _robot->GetPoseOriginList().GetOriginByID(objectsByOrigin.first).GetName().c_str(),
                                        EnumToString(objectsByFamily.first),
                                        EnumToString(objectsByType.first),
                                        objectsByID.first.GetValue());
                    continue;
                  }

                  if(filter.ConsiderObject(object))
                  {
                    matchingObject = object_nonconst;
                    if(nullptr != modifierFcn) {
                      modifierFcn(matchingObject);
                    }
                    if(returnFirstFound) {
                      return matchingObject;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    return matchingObject;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ActiveObject* BlockWorld::FindConnectedObjectHelper(const BlockWorldFilter& filterIn,
                                                      const ModifierFcn& modifierFcn,
                                                      bool returnFirstFound) const
  {
    ActiveObject* matchingObject = nullptr;

    // TODO COZMO-7848:  create BlockWorldActiveObjetFilter vs BlockWorldFilter because some of the parameters
    //                   Or at least assert on filter unused flags
    // in BlockWorldFilter do not apply to connected objects. Eg: IsOnlyConsideringLatestUpdate, OriginMode, etc.
    // Moreover, additional fields could be available like `allowedActiveID`
    BlockWorldFilter filter(filterIn);
    DEV_ASSERT(!filter.IsOnlyConsideringLatestUpdate(), "BlockWorld.FindConnectedObjectHelper.InvalidFlag");

    for(auto & objectsByFamily : _connectedObjects) {
      if(filter.ConsiderFamily(objectsByFamily.first)) {
        for(auto & objectsByType : objectsByFamily.second) {
          if(filter.ConsiderType(objectsByType.first)) {
            for(auto & objectsByID : objectsByType.second)
            {
              ActiveObject* object_nonconst = objectsByID.second.get();
              const ActiveObject* object = object_nonconst;

              if(nullptr == object)
              {
                PRINT_NAMED_WARNING("BlockWorld.FindConnectedObjectHelper.NullObject",
                                    "Family:%s Type:%s ID:%d is NULL",
                                    EnumToString(objectsByFamily.first),
                                    EnumToString(objectsByType.first),
                                    objectsByID.first.GetValue());
                continue;
              }

              const bool considerObject = filter.ConsiderObject(object);
              if(considerObject)
              {
                matchingObject = object_nonconst;
                if(nullptr != modifierFcn) {
                  modifierFcn(matchingObject);
                }
                if(returnFirstFound) {
                  return matchingObject;
                }
              }
            }
          }
        }
      }
    }

    return matchingObject;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ObservableObject* BlockWorld::GetLocatedObjectByIdHelper(const ObjectID& objectID, ObjectFamily family) const
  {
    // Find the object with the given ID with any pose state, in the current world origin
    BlockWorldFilter filter;
    filter.AddAllowedID(objectID);

    // Restrict family if specified
    if ( ObjectFamily::Invalid != family ) {
      filter.AddAllowedFamily(family);
    }

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

    Vec3f closestDist(distThreshold);
    //ObservableObject* matchingObject = nullptr;

    BlockWorldFilter filter(filterIn);
    filter.AddFilterFcn([&pose, &closestDist](const ObservableObject* current)
                        {
                          Vec3f dist = ComputeVectorBetween(pose, current->GetPose());
                          dist.Abs();
                          if(dist.Length() < closestDist.Length()) {
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







    void CheckForOverlapHelper(const ObservableObject* objectToMatch,
                               ObservableObject* objectToCheck,
                               std::vector<ObservableObject*>& overlappingObjects)
    {

      // TODO: smarter block pose comparison
      //const float minDist = 5.f; // TODO: make parameter ... 0.5f*std::min(minDimSeen, objExist->GetMinDim());

      //const float distToExist_mm = (objExist.second->GetPose().GetTranslation() -
      //                              <robotThatSawMe???>->GetPose().GetTranslation()).length();

      //const float distThresh_mm = distThresholdFraction * distToExist_mm;

      //Pose3d P_diff;
      if( objectToCheck->IsSameAs(*objectToMatch) ) {
        overlappingObjects.push_back(objectToCheck);
      } /*else {
         fprintf(stdout, "Not merging: Tdiff = %.1fmm, Angle_diff=%.1fdeg\n",
         P_diff.GetTranslation().length(), P_diff.GetRotationAngle().getDegrees());
         objExist.second->IsSameAs(*objectSeen, distThresh_mm, angleThresh, P_diff);
         }*/

    } // CheckForOverlapHelper()


    void BlockWorld::FindOverlappingObjects(const ObservableObject* objectSeen,
                                            const ObjectsMapByType_t& objectsExisting,
                                            std::vector<ObservableObject*>& overlappingExistingObjects) const
    {
      auto objectsExistingIter = objectsExisting.find(objectSeen->GetType());
      if(objectsExistingIter != objectsExisting.end()) {
        for(const auto& objectToCheck : objectsExistingIter->second) {
          CheckForOverlapHelper(objectSeen, objectToCheck.second.get(), overlappingExistingObjects);
        }
      }

    } // FindOverlappingObjects()


    void BlockWorld::FindOverlappingObjects(const ObservableObject* objectExisting,
                                            const std::vector<ObservableObject*>& objectsSeen,
                                            std::vector<ObservableObject*>& overlappingSeenObjects) const
    {
      for(const auto& objectToCheck : objectsSeen) {
        CheckForOverlapHelper(objectExisting, objectToCheck, overlappingSeenObjects);
      }
    }

    void BlockWorld::FindOverlappingObjects(const ObservableObject* objectExisting,
                                            const std::multimap<f32, ObservableObject*>& objectsSeen,
                                            std::vector<ObservableObject*>& overlappingSeenObjects) const
    {
      for(const auto& objectToCheckPair : objectsSeen) {
        ObservableObject* objectToCheck = objectToCheckPair.second;
        CheckForOverlapHelper(objectExisting, objectToCheck, overlappingSeenObjects);
      }
    }


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
      if (observedObject->GetFamily() == ObjectFamily::LightCube)
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
                                    observedObject->GetFamily(),
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
                                                  obj->GetFamily(),
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
                                                  obj->GetFamily(),
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

    for(auto & objectsByFamily : originIter->second)
    {
      for(auto & objectsByType : objectsByFamily.second)
      {
        auto objectIter = objectsByType.second.find(objectID);
        if(objectIter != objectsByType.second.end())
        {
          std::shared_ptr<ObservableObject> object = objectIter->second;
          if(!object->GetPose().HasSameRootAs(oldOrigin))
          {
            const ObjectFamily family  = object->GetFamily();
            const ObjectType   objType = object->GetType();

            DEV_ASSERT(family == objectsByFamily.first, "BlockWorld.UpdateObjectOrigin.FamilyMismatch");
            DEV_ASSERT(objType == objectsByType.first,  "BlockWorld.UpdateObjectOrigin.TypeMismatch");
            DEV_ASSERT(objectID == object->GetID(),     "BlockWorld.UpdateObjectOrigin.IdMismatch");

            const Pose3d& newOrigin = object->GetPose().FindRoot();

            PRINT_CH_INFO("BlockWorld", "BlockWorld.UpdateObjectOrigin.ObjectFound",
                          "Updating ObjectID %d from origin %s to %s",
                          objectID.GetValue(),
                          oldOrigin.GetName().c_str(),
                          newOrigin.GetName().c_str());

            const PoseOriginID_t newOriginID = newOrigin.GetID();
            DEV_ASSERT_MSG(_robot->GetPoseOriginList().ContainsOriginID(newOriginID),
                           "BlockWorld.UpdateObjectOrigin.ObjectOriginNotInOriginList",
                           "Name:%s", object->GetPose().FindRoot().GetName().c_str());

            // Add to object's current origin
            _locatedObjects[newOriginID][family][objType][objectID] = object;

            // Notify pose confirmer
            _robot->GetObjectPoseConfirmer().AddInExistingPose(object.get());

            // Delete from old origin
            objectsByType.second.erase(objectIter);

            // Clean up if we just deleted the last object from this type/family/origin
            if(objectsByType.second.empty())
            {
              objectsByFamily.second.erase(objectsByType.first);
              if(objectsByFamily.second.empty())
              {
                originIter->second.erase(objectsByFamily.first);
                if(originIter->second.empty())
                {
                  _locatedObjects.erase(originIter);
                }
              }
            }
          }

          return RESULT_OK;
        }
      }
    }

    PRINT_CH_INFO("BlockWorld", "BlockWorld.UpdateObjectOrigin.ObjectNotFound",
                  "Object %d not found in origin %s",
                  objectID.GetValue(), oldOrigin.GetName().c_str());

    return RESULT_FAIL;
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

      // Look for a matching object in the new origin. Should have same family and type. If unique, should also have
      // same ID, or if not unique, the poses should match.
      BlockWorldFilter filterNew;
      filterNew.SetOriginMode(BlockWorldFilter::OriginMode::Custom);
      filterNew.AddAllowedOrigin(newOriginID);
      filterNew.AddAllowedFamily(oldObject->GetFamily());
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

        // if the ID changes, re-add in the origin
        const ObjectID& newIDInNewOrigin = oldObject->GetID();
        const ObjectID prevIDInNewOrigin = newObject->GetID(); // make copy because we are going to change its value

        // we also want to keep the MOST recent objectID, rather than the one we used to have for this object, because
        // if clients are bookkeeping IDs, the know about the new one (for example, if an action is already going
        // to pick up that objectID, it should not change by virtue of rejiggering)
        // Note: despite the name, oldObject is the most recent instance of this match. Thanks, Andrew.
        newObject->CopyID( oldObject );

        // update parent container to match the inherited ID to the key we use to store it
        if ( newIDInNewOrigin != prevIDInNewOrigin )
        {
          const ObjectFamily inFamily = oldObject->GetFamily();
          const ObjectType withType   = oldObject->GetType();
          ObjectsMapByID_t& mapByID = _locatedObjects.at(newOriginID).at(inFamily).at(withType);
          // copy smart pointer to increment reference
          mapByID[newIDInNewOrigin] = mapByID.at(prevIDInNewOrigin);
          mapByID.erase(prevIDInNewOrigin);
          // log this crazyness
          PRINT_CH_INFO("BlockWorld", "BlockWorld.UpdateObjectOrigins.MovedSharedPointerDueToIDChange",
                        "Object with ID %d in new origin (%s) has inherited ID %d",
                        prevIDInNewOrigin.GetValue(),
                        newOrigin.GetName().c_str(),
                        newIDInNewOrigin.GetValue());
        }
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
      // DeleteLocatedObjectsByOrigin(oldOrigin);
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
  Result BlockWorld::AddAndUpdateObjects(const std::multimap<f32, ObservableObject*>& objectsSeen,
                                         const RobotTimeStamp_t atTimestamp)
  {
    const Pose3d& currFrame = _robot->GetWorldOrigin();
    const PoseOriginID_t currFrameID = currFrame.GetID();

    // Construct a helper data structure for determining which objets we might
    // want to localize to
    PotentialObjectsForLocalizingTo potentialObjectsForLocalizingTo(*_robot);

    for(const auto& objSeenPair : objectsSeen)
    {
      // Note that we wrap shared_ptr around the passed-in object, which was created
      // by the caller (via Cloning from the library of known objects). We will either
      // add this object to the poseConfirmer, in which case it will not get deleted,
      // or it will simply be used to update an existing object's pose and/or for
      // localization, both done by potentialObjectsForLocalizingTo. In that case,
      // once potentialObjectsForLocalizingTo is done with it, and we have exited
      // this for loop, its refcount will be zero and it'll get deleted for us.
      std::shared_ptr<ObservableObject> objSeen(objSeenPair.second);

      // We use the distance to the observed object to decide (a) if the object is
      // close enough to do localization/identification and (b) to use only the
      // closest object in each coordinate frame for localization.
      const f32 distToObjSeen = ComputeDistanceBetween(_robot->GetPose(), objSeen->GetPose());

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
              that scenario, observations that come here would either udpate the robot (localizing) or update
              the object in blockworld (not localizing), but there would be no need to feed back that observation
              back into the PoseConfirmer/ObservationFilter, which is a one-way filter.

              For now, to prevent adding an observation twice or not adding it at all, flag here whether the
              observation is used by the PoseConfirmer before letting it cascade down when it confirms an object (which
              is a fix for a bug in which observations that confirm an object where not used for localization,
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
      // Note that if it just became confirmed we still want to localize to it in case that in can rejigger
      // other origins

      // Keep a list of objects matching this objSeen, by coordinate frame
      std::map<PoseOriginID_t, ObservableObject*> matchingObjects;

      // Match active objects by type and inactive objects by pose. Regardless,
      // only look at objects in the same family with the same type.
      // Also override the default filter function to intentionally consider objects
      // that are unknown here. Otherwise, we'd never be able to match new observations
      // to existing objects whose pose has been set to unknown!
      BlockWorldFilter filter;
      filter.AddAllowedFamily(objSeen->GetFamily());
      filter.AddAllowedType(objSeen->GetType());

      if (objSeen->IsUnique())
      {
        // Can consider matches for active objects in other coordinate frames,
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
        const ObjectID& carryingObjectID = _robot->GetCarryingComponent().GetCarryingObject();
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

      } // if/else object is active

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
      //        if(actionObject->IsBeingCarried() && _robot->GetCarryingObject() != obsID) {
      //          PRINT_NAMED_WARNING("BlockWorld.AddAndUpdateObject.CarryStateMismatch",
      //                              "Object %d thinks it is being carried, but does not match "
      //                              "robot %d's carried object ID (%d). Setting as uncarried.",
      //                              obsID.GetValue(), _robot->GetID(),
      //                              _robot->GetCarryingObject().GetValue());
      //          actionObject->SetBeingCarried(false);
      //        }
      //      }

      // Tell the world about the observed object. NOTE: it is guaranteed to be in the current frame.
      BroadcastObjectObservation(observedObject);

      _didObjectsChange = true;
      _robotMsgTimeStampAtChange = Anki::Util::Max(atTimestamp, _robot->GetMapComponent().GetCurrentMemoryMap()->GetLastChangedTimeStamp());

    } // for each object seen

    // NOTE: This will be a no-op if we no objects got inserted above as being potentially
    //       useful for localization
    Result localizeResult = potentialObjectsForLocalizingTo.LocalizeRobot();

    return localizeResult;

  } // AddAndUpdateObjects()

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::UpdatePoseOfStackedObjects()
  {
    DEV_ASSERT(_trackPoseChanges, "BlockWorld.UpdatePoseOfStackedObjects.CanRunOnlyWhileTrackingPoseChanges");

    // iterate all changed objects updating any objects we think are on top of them
    auto changedObjectIt = _objectPoseChangeList.begin();
    while ( changedObjectIt != _objectPoseChangeList.end() )
    {
      // grab the object whose pose we changed (we can't trust caching pointers in case we rejigger)
      ObservableObject* changedObjectPtr = GetLocatedObjectByID( changedObjectIt->_id );
      if ( nullptr != changedObjectPtr )
      {
        // find the object that is currently on top of the old position

        // TODO COZMO-5591
        // change FindLocatedObjectOnTopOf to FindObjectsOnTopOf and return a vector. Potentially we could have more
        // than object directly on top of us, or a moved object could have ended up on top of our old pose, which
        // would then trump the object that used to be our old top. This could be solved by returning all
        // current objects on top of our old pose, and then discarding those that have changed their poses, allowing
        // us to fix both issues. For the moment, because we need to ship I am supporting only one (old code
        // was supporting one anyway)
        ObservableObject* myOldCopy = changedObjectPtr->CloneType();
        myOldCopy->InitPose(changedObjectIt->_oldPose, changedObjectIt->_oldPoseState);

        BlockWorldFilter filter;
        // Ignore the object we are looking on top off so that we don't consider it as on top of itself
        filter.AddIgnoreID(changedObjectPtr->GetID());

        // some objects should not be updated! (specially those that are not managed through observations)
        // TODO rsam/andrew: I don't like not having to specify true/false for families. Whether a new family
        // gets included or ignored happens silently for filters if we don't static_assert requiring all
        filter.AddIgnoreFamily(Anki::Vector::ObjectFamily::MarkerlessObject);
        filter.AddIgnoreFamily(Anki::Vector::ObjectFamily::CustomObject);

        // When Cozmo is looking at a stack/pyramid from a certain distance, the top cube can toggle rapidly between
        // 'known' and 'dirty' pose state, causing the cube LEDs to flash on and off rapidly. This may cause users
        // to think there is an issue with the cube. To avoid this, we do not allow updating a stack if we have seen
        // the top cube "recently", because it's likely that we are simply not seeing all the cubes' markers in all the
        // frames. See also notes in COZMO-10580.
        const RobotTimeStamp_t lastProcImageTime_ms = _robot->GetLastImageTimeStamp();
        BlockWorldFilter::FilterFcn notSeenRecently = [lastProcImageTime_ms](const ObservableObject* obj)
        {
          if(obj->GetLastObservedTime() + kRecentlySeenTimeForStackUpdate_ms < lastProcImageTime_ms) {
            // Not seen recently, so DO allow this object through the filter for updating
            return true;
          }
          // Seen recently, don't update this object
          return false;
        };

        filter.AddFilterFcn(notSeenRecently);

        // find object
        ObservableObject* objectOnTopOfOldPose = FindLocatedObjectOnTopOf(*myOldCopy, STACKED_HEIGHT_TOL_MM, filter);
        if ( nullptr != objectOnTopOfOldPose )
        {
          // we found an object currently on top of our old pose
          const ObjectID& topID = objectOnTopOfOldPose->GetID();

          // if this is not an object we are carrying
          if ( !_robot->GetCarryingComponent().IsCarryingObject(topID) )
          {
            // check if it used to be there too or we have already moved it this udpate
            auto matchIDlambda = [&topID](const PoseChange& a) { return a._id == topID; };
            const bool alreadyChanged = std::find_if(_objectPoseChangeList.begin(), _objectPoseChangeList.end(), matchIDlambda) != _objectPoseChangeList.end();
            if ( !alreadyChanged )
            {
              // we haven't changed it this frame, we want to modify it based on the change we made to the bottom one
              Pose3d topPose = objectOnTopOfOldPose->GetPose();
              if(topPose.GetWithRespectTo(myOldCopy->GetPose(), topPose))
              {
                // P_top_wrt_origin = P_newBtm_wrt_origin * P_top_wrt_oldBtm:
                topPose.PreComposeWith(changedObjectPtr->GetPose());
                topPose.SetParent(changedObjectPtr->GetPose().GetParent());

                // update its pose based on the stack dependency. We expect this observation to add the entry for this
                // object in objectPoseUpdates, and thus naturally iterating our way up stacks of more than 2 objects
                Result result = _robot->GetObjectPoseConfirmer().AddObjectRelativeObservation(objectOnTopOfOldPose, topPose, changedObjectPtr);
                if(RESULT_OK != result)
                {
                  PRINT_NAMED_WARNING("BlockWorld.UpdateRotationOfObjectsStackedOn.AddRelativeObservationFailed",
                                      "Giving up on rest of stack");
                }
              }
              else
              {
                PRINT_NAMED_WARNING("BlockWorld.UpdateStacks.OriginMismatch",
                                    "Can't obtain topPose wrt old, but that's exactly how we found the object in topPose.");
              }
            } // else: object already moved
          } // else: we are carrying the object on top
        } // else: there are no objects on top

        Util::SafeDelete(myOldCopy);
      }
      else
      {
        // if the object changed to Invalid (unobserved, unknown, ..), then we don't have to update objects
        // that were on top of it here. The system that flagged as unobserved should have updated the top one
        // two.
        // TODO: Is that currently happening ^?
        // TODO Test: see a stack, look to bottom only, move stack, see cube behind (will unobserve bottom of stack).
        //            Does this flag the top as unknown too? Should it?
        PRINT_CH_INFO("BlockWorld", "BlockWorld.UpdateStacks", "'%d' does not exist in current frame. Ignoring change.",
          changedObjectIt->_id.GetValue() );
      }

      // continue to next object
      ++changedObjectIt;
    }
  } // UpdatePoseOfStackedObjects()

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

    for(auto & objectFamily : originIter->second)
    {
      for(auto & objectsByType : objectFamily.second)
      {
        ObjectsMapByID_t& objectIdMap = objectsByType.second;
        auto objectIter = objectIdMap.begin();
        while(objectIter != objectIdMap.end())
        {
          ObservableObject* object = objectIter->second.get();
          if(nullptr == object)
          {
            PRINT_NAMED_WARNING("BlockWorld.CheckForUnobservedObjects.NullObject",
                                "Family:%s Type:%s ID:%d is NULL. Deleting entry.",
                                EnumToString(objectFamily.first),
                                EnumToString(objectsByType.first),
                                objectIter->first.GetValue());
            objectIter = objectIdMap.erase(objectIter);
            continue;
          }

          // Look for "unobserved" objects not seen atTimestamp -- but skip objects:
          //    - that are currently being carried
          //    - that we are currently docking to
          //    - whose pose origin does not match the robot's
          //    - who are a charger (since those stay around)
          const RobotTimeStamp_t lastVisuallyMatchedTime = _robot->GetObjectPoseConfirmer().GetLastVisuallyMatchedTime(object->GetID());
          const bool isUnobserved = ( (lastVisuallyMatchedTime < atTimestamp) &&
                                      (_robot->GetCarryingComponent().GetCarryingObject() != object->GetID()) &&
                                      (_robot->GetDockingComponent().GetDockObject() != object->GetID()) );
          if ( isUnobserved )
          {
            unobservedObjectIDs.push_back(object->GetID());
          }

          ++objectIter;

        } // for object IDs of this type
      } // for each object type
    } // for each object family

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

      using NotVisibleReason = Vision::KnownMarker::NotVisibleReason;
      NotVisibleReason reason = unobservedObject->IsVisibleFromWithReason(camera,
                                                                   MAX_MARKER_NORMAL_ANGLE_FOR_SHOULD_BE_VISIBLE_CHECK_RAD,
                                                                   MIN_MARKER_SIZE_FOR_SHOULD_BE_VISIBLE_CHECK_PIX,
                                                                   xBorderPad, yBorderPad,
                                                                   hasNothingBehind);

      const bool isDirtyPoseState = (PoseState::Dirty == unobservedObject->GetPoseState());

      bool shouldBeVisible = true;
      bool occluded        = false;
      bool objectAligned   = true;

      switch (reason) {
        case NotVisibleReason::IS_VISIBLE:
        { 
          break; 
        }
        case NotVisibleReason::OCCLUDED: 
        { 
          occluded = true;
          shouldBeVisible = false;
          break; 
        }
        case NotVisibleReason::NOTHING_BEHIND: 
        { 
          shouldBeVisible = false;
          break; 
        }
        case NotVisibleReason::NORMAL_NOT_ALIGNED:
        case NotVisibleReason::BEHIND_CAMERA:
        case NotVisibleReason::OUTSIDE_FOV:
        case NotVisibleReason::TOO_SMALL:
        case NotVisibleReason::CAMERA_NOT_CALIBRATED:
        case NotVisibleReason::POSE_PROBLEM:
        {
          objectAligned = false;
          shouldBeVisible = false;
          break;
        }
      }

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
        // if the marker should be visible by the camera and it is not occluded
        if( !occluded && objectAligned ) 
        {
          _robot->GetMapComponent().MarkObjectUnobserved(*unobservedObject);
        } else {
          // We "should" have seen the object! Clear it.
          PRINT_CH_INFO("BlockWorld", "BlockWorld.CheckForUnobservedObjects.MarkingUnobservedObject",
                        "Marking object %d unobserved, which should have been seen, but wasn't. "
                        "(shouldBeVisible:%d hasNothingBehind:%d isDirty:%d, occluded:%d, objectAligned:%d)",
                        unobservedObject->GetID().GetValue(),
                        shouldBeVisible, hasNothingBehind, isDirtyPoseState, occluded, objectAligned);

          Result markResult = _robot->GetObjectPoseConfirmer().MarkObjectUnobserved(unobservedObject);
          if(RESULT_OK != markResult)
          {
            PRINT_NAMED_WARNING("BlockWorldCheckForUnobservedObjects.MarkObjectUnobservedFailed", "");
          }
        }
      }

    } // for each unobserved object

  } // CheckForUnobservedObjects()

  Result BlockWorld::AddMarkerlessObject(const Pose3d& p, ObjectType type)
  {
    RobotTimeStamp_t lastTimestamp = _robot->GetLastMsgTimestamp();

    // Create an instance of the detected object
    auto markerlessObject = std::make_shared<MarkerlessObject>(type);
    markerlessObject->SetLastObservedTime((TimeStamp_t)lastTimestamp);

    // Raise origin of object above ground.
    // NOTE: Assuming detected obstacle is at ground level no matter what angle the head is at.
    Pose3d raiseObject(0, Z_AXIS_3D(), Vec3f(0,0,0.5f*markerlessObject->GetSize().z()));
    Pose3d obsPose = p * raiseObject;
    obsPose.SetParent(_robot->GetPose().GetParent());

    // Initialize with Known pose so it won't delete immediately because it isn't re-seen
    markerlessObject->InitPose(obsPose, PoseState::Known);

    // Check if this prox obstacle already exists
    std::vector<ObservableObject*> existingObjects;
    const auto originIter = _locatedObjects.find(_robot->GetPoseOriginList().GetCurrentOriginID());
    if(originIter != _locatedObjects.end())
    {
      FindOverlappingObjects(markerlessObject.get(), originIter->second[ObjectFamily::MarkerlessObject], existingObjects);
    }

    // Update the last observed time of existing overlapping obstacles
    for(auto obj : existingObjects) {
      obj->SetLastObservedTime((TimeStamp_t)lastTimestamp);
    }

    // No need to add the obstacle again if it already exists
    if (!existingObjects.empty()) {
      return RESULT_OK;
    }


    // Check if the obstacle intersects with any other existing objects in the scene.
    BlockWorldFilter filter;
    if(_robot->GetLocalizedTo().IsSet()) {
      filter.AddAllowedFamily(ObjectFamily::LightCube);
      filter.AddAllowedFamily(ObjectFamily::MarkerlessObject);
    }

    FindLocatedIntersectingObjects(markerlessObject.get(), existingObjects, 0, filter);
    if (!existingObjects.empty()) {
      return RESULT_OK;
    }

    // set new ID before adding to the world, since this is a new object
    DEV_ASSERT( !markerlessObject->GetID().IsSet(), "BlockWorld.AddMarkerlessObject.NewObjectHasID" );
    markerlessObject->SetID();

    AddLocatedObject(markerlessObject);
    _didObjectsChange = true;
    _robotMsgTimeStampAtChange = Anki::Util::Max(lastTimestamp, _robot->GetMapComponent().GetCurrentMemoryMap()->GetLastChangedTimeStamp());

    return RESULT_OK;
  }

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
    _didObjectsChange = true;
    _robotMsgTimeStampAtChange = Anki::Util::Max(_robot->GetLastMsgTimestamp(), _robot->GetMapComponent().GetCurrentMemoryMap()->GetLastChangedTimeStamp());

    return customObject->GetID();
  }

  bool BlockWorld::DidObjectsChange() const {
    return _didObjectsChange;
  }
  
  const RobotTimeStamp_t& BlockWorld::GetTimeOfLastChange() const {
    return _robotMsgTimeStampAtChange;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ObjectID BlockWorld::AddConnectedActiveObject(ActiveID activeID, FactoryID factoryID, ObjectType objType)
  {
    // only connected objects should be added through this method, so a required activeID is a must
    DEV_ASSERT(activeID != ObservableObject::InvalidActiveID, "BlockWorld.AddConnectedActiveObject.CantAddInvalidActiveID");

    // NOTE: If you hit any of the following VERIFY, please notify Raul and Al.
    // rsam: Al and I have made assumptions about when this gets called. Checking here that the assumptions are correct,
    // and if they are not, we need to re-evaluate this code, for example due to robot timing issues.

    // Validate that ActiveID is not currently a connected object. We assume that if the robot is reporting
    // an activeID, it should not be still used here (should have reported a disconnection)
    const ActiveObject* conObjWithActiveID = GetConnectedActiveObjectByActiveID( activeID );
    if ( nullptr != conObjWithActiveID)
    {
      // Al/rsam: it is possible to receive multiple connection messages for the same cube. Verify here that factoryID
      // and activeType match, and if they do, simply ignore the message, since we already have a valid instance
      const bool isSameObject = (factoryID == conObjWithActiveID->GetFactoryID()) &&
                                (objType == conObjWithActiveID->GetType());
      if ( isSameObject ) {
        PRINT_CH_INFO("BlockWorld", "BlockWorld.AddConnectedActiveObject.FoundMatchingObjectAtSameSlot",
                      "objectID %d, activeID %d, factoryID %s, type %s",
                      conObjWithActiveID->GetID().GetValue(),
                      conObjWithActiveID->GetActiveID(),
                      conObjWithActiveID->GetFactoryID().c_str(),
                      EnumToString(conObjWithActiveID->GetType()));
        return conObjWithActiveID->GetID();
      }

      // if it's not the same, then what the hell, we are currently using that activeID for other object!
      PRINT_NAMED_ERROR("BlockWorld.AddConnectedActiveObject.ConflictingActiveID",
                        "ActiveID:%d found when we tried to add that activeID as connected object. Disconnecting previous.",
                        activeID);

      // clear the pointer and destroy it
      conObjWithActiveID = nullptr;
      RemoveConnectedActiveObject(activeID);
    }

    // Validate that factoryId is not currently a connected object. We assume that if the robot is reporting
    // a factoryID, the same object should not be in any current activeIDs.
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
    ActiveObject* newActiveObjectPtr = CreateActiveObjectByType(objType, activeID, factoryID);
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
            DEV_ASSERT( matchObjectID == sameTypeObject->GetID(), "BlockWorld.AddConnectedActiveObject.NotSAmeObjectID");
          } else {
            // set once
            matchObjectID = sameTypeObject->GetID();
          }

          // COZMO-9663: Separate localizable property from poseState
          const bool propagateStack = false;
          _robot->GetObjectPoseConfirmer().MarkObjectDirty(sameTypeObject, propagateStack);

          // check if the instance has activeID
          if (sameTypeObject->GetActiveID() == ObservableObject::InvalidActiveID)
          {
            // it doesn't have an activeID, we are connecting to it, set
            sameTypeObject->SetActiveID(activeID);
            sameTypeObject->SetFactoryID(factoryID);
            PRINT_CH_INFO("BlockWorld", "BlockWorld.AddConnectedActiveObject.FoundMatchingObjectWithNoActiveID",
                          "objectID %d, activeID %d, type %s",
                          sameTypeObject->GetID().GetValue(), sameTypeObject->GetActiveID(), EnumToString(objType));
          } else {
            // it has an activeID, we were connected. Is it the same object?
            if ( sameTypeObject->GetFactoryID() != factoryID )
            {
              // uhm, this is a different object (or factoryID was not set)
              PRINT_CH_INFO("BlockWorld",
                            "AddActiveObject.FoundOtherActiveObjectOfSameType",
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
              PRINT_CH_INFO("BlockWorld", "BlockWorld.AddConnectedActiveObject.FoundIdenticalObjectOnDifferentSlot",
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

      // NOTE: If this error does not happen for some time, we should remove this `else` block to simplify code.
      // We should not find any objects in any origins that have this activeID. Otherwise that means they have
      // not disconnected properly. If there's a timing issue with connecting an object to an activeID before
      // disconnecting a previous object, we would like to know, so we can act accordingly. Add this error here
      // to detect that situation.
      PRINT_NAMED_ERROR("BlockWorld.AddConnectedActiveObject.ConflictingActiveID",
                        "Objects with ActiveID:%d were found when we tried to add that activeID as connected object.",
                        activeID);

      // In case there is a timing issue, see how we would fix it:
      // 1) if the match has the same factoryID, we must not have cleaned it up on disconnection, inherit objectID
      // 2) if the match has an invalid factoryID, we did not clean up activeID properly, bind again any origin
      // 3) if the match has a different factoryID, we missed that object's disconnection, delete it and do not bind

      ObservableObject* matchingObject = matchingObjects.front();

      // A match was found but does it have the same factory ID?
      if (matchingObject->GetFactoryID() == factoryID)
      {
        // somehow (error) we did not have an active connected object for it despite having activeID
        PRINT_CH_INFO("BlockWorld", "BlockWorld.AddConnectedActiveObject.FoundMatchingActiveObject",
                      "objectID %d, activeID %d, type %s, factoryID %s",
                      matchingObject->GetID().GetValue(), matchingObject->GetActiveID(), EnumToString(objType), matchingObject->GetFactoryID().c_str());

        // inherit objectID in the activeObject we created
        newActiveObjectPtr->CopyID(matchingObject);
      }
      else if (matchingObject->GetFactoryID().empty())
      {
        // somehow (error) it never connected before (it shouldn't have had activeID)
        PRINT_NAMED_WARNING("BlockWorld.AddConnectedActiveObject.FoundMatchingActiveObjectThatWasNeverConnected",
                         "objectID %d, activeID %d, type %s, factoryID %s",
                         matchingObject->GetID().GetValue(), matchingObject->GetActiveID(), EnumToString(objType), matchingObject->GetFactoryID().c_str());

        // if now we have a factoryID, go fix all located instances of the new object setting their factoryID
        if(!factoryID.empty())
        {
          // Need to check existing objects in other frames and update to match new object
          std::vector<ObservableObject*> matchingObjectsInAllFrames;
          BlockWorldFilter filterInAnyByID;
          filterInAnyByID.AddFilterFcn([activeID](const ObservableObject* object) { return object->GetActiveID() == activeID;} );
          filterInAnyByID.SetAllowedTypes({objType});
          filterInAnyByID.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
          FindLocatedMatchingObjects(filterInAnyByID, matchingObjectsInAllFrames);

          PRINT_CH_INFO("BlockWorld", "BlockWorld.AddConnectedActiveObject.UpdateExistingObjectsFactoryID",
                           "Updating %zu existing objects in other frames to match object with factoryID %s and activeID %d",
                           matchingObjectsInAllFrames.size(),
                           factoryID.c_str(),
                           activeID);

          ObjectID idPrev;
          if(!matchingObjectsInAllFrames.empty())
          {
            idPrev = matchingObjectsInAllFrames.front()->GetID();
          }
          for(ObservableObject* obj : matchingObjectsInAllFrames)
          {
            DEV_ASSERT(obj->GetID() == idPrev, "Matching objects in different frames don't have same ObjectId");
            obj->SetFactoryID(factoryID);
            obj->SetActiveID(activeID); // should not be necessary
            idPrev = obj->GetID();
          }
        }

        // we updated all located instances of the new connected object, so we can inherit the objectID
        newActiveObjectPtr->CopyID(matchingObject);
      }
      else
      {
        // ActiveID matched, but FactoryID did not, delete the located instance of this object
        PRINT_NAMED_ERROR("BlockWorld.AddConnectedActiveObject.MismatchedFactoryID",
                          "objectID %d, activeID %d, type %s, factoryID %s (expected %s)",
                          matchingObject->GetID().GetValue(), matchingObject->GetActiveID(), EnumToString(objType), factoryID.c_str(), matchingObject->GetFactoryID().c_str());

        BlockWorldFilter filter;
        filter.AddAllowedID(matchingObject->GetID());
        DeleteLocatedObjects(filter);

        // do not inherit the objectID we just removed
        DEV_ASSERT( !newActiveObjectPtr->GetID().IsSet(), "BlockWorld.AddConnectedActiveObject.UnexpectedObjectID" );
        newActiveObjectPtr->SetID();
      }
    }

    // at this point the new active connected object has a valid objectID, we can finally add it to the world
    DEV_ASSERT( newActiveObjectPtr->GetID().IsSet(), "BlockWorld.AddConnectedActiveObject.ObjectIDWasNeverSet" );
    _connectedObjects[newActiveObjectPtr->GetFamily()][newActiveObjectPtr->GetType()][newActiveObjectPtr->GetID()].reset( newActiveObjectPtr );

    // return the assigned objectID
    return newActiveObjectPtr->GetID();
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ObjectID BlockWorld::RemoveConnectedActiveObject(ActiveID activeID)
  {
    ObjectID removedObjectID;

    // search in each family
    ActiveObjectsMapByFamily_t::iterator familyIt = _connectedObjects.begin();
    const ActiveObjectsMapByFamily_t::iterator familyEnd = _connectedObjects.end();
    while( familyIt != familyEnd )
    {
      // search in each objectType
      ActiveObjectsMapByType_t& objectTypeMap = familyIt->second;
      ActiveObjectsMapByType_t::iterator objectTypeIt = objectTypeMap.begin();
      const ActiveObjectsMapByType_t::iterator objectTypeEnd = objectTypeMap.end();
      while( objectTypeIt != objectTypeEnd )
      {
        // search in every object (indexed byID)
        ActiveObjectsMapByID_t& objectIDMap = objectTypeIt->second;
        ActiveObjectsMapByID_t::iterator objectIt = objectIDMap.begin();
        const ActiveObjectsMapByID_t::iterator objectEnd = objectIDMap.end();
        while( objectIt != objectEnd )
        {
          // grab reference to the smartptr
          std::shared_ptr<ActiveObject>& object = objectIt->second;
          if( object->GetActiveID() == activeID )
          {
            // found the object, set objectID so we can flag located instances
            removedObjectID = object->GetID();
            // now reset the shared_ptr (via reference), and erase the entry (to prevent using nullptr later)
            object.reset();
            objectIt = objectIDMap.erase( objectIt );
            break;
          } else {
            ++objectIt;
          }
        } // - objects

        // if found, finish loop, otherwise go to next object type
        if ( removedObjectID.IsSet() ) {
          break;
        } else {
          ++objectTypeIt;
        }
      } // - object types

      // if found, finish loop, otherwise go to next family
      if ( removedObjectID.IsSet() ) {
        break;
      } else {
        ++familyIt;
      }
    } // - families

    // We can't localize to the instances of this object anymore. We can do so by removing their activeID, which
    // we need to do anyway. Flagging as Dirty would be optional, but see COZMO-9663
    if ( removedObjectID.IsSet() )
    {
      // COZMO-9663: Separate localizable property from poseState
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

    // Check if the object intersects with any other existing prox objects in the scene, then delete them
    BlockWorldFilter filter;
    filter.AddAllowedType(ObjectType::ProxObstacle);
    DeleteIntersectingObjects(object, 0, filter);


    // grab the current pointer and check it's empty (do not expect overwriting)
    std::shared_ptr<ObservableObject>& objectLocation =
      _locatedObjects[objectOriginID][object->GetFamily()][object->GetType()][object->GetID()];
    DEV_ASSERT(objectLocation == nullptr, "BlockWorld.AddLocatedObject.ObjectIDInUseInOrigin");
    objectLocation = object; // store the new object, this increments refcount

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

    // fire event to represent "first time an object has been seen in this origin"
    Util::sInfoF("robot.object_located", {}, "%s", EnumToString(object->GetType()));

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

  Result BlockWorld::AddCollisionObstacle(const Pose3d& p)
  {
    const Result ret = AddMarkerlessObject(p, ObjectType::CollisionObstacle);
    return ret;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::RemoveMarkersWithinMarkers(std::list<Vision::ObservedMarker>& currentObsMarkers)
  {
    for(auto markerIter1 = currentObsMarkers.begin(); markerIter1 != currentObsMarkers.end(); ++markerIter1)
    {
      const Vision::ObservedMarker& marker1 = *markerIter1;

      for(auto markerIter2 = currentObsMarkers.begin(); markerIter2 != currentObsMarkers.end(); /* incrementing decided in loop */ )
      {
        const Vision::ObservedMarker& marker2 = *markerIter2;

        if(marker1.GetTimeStamp() != marker2.GetTimeStamp())
        {
          PRINT_NAMED_WARNING("BlockWorld.RemoveMarkersWithinMarkers.MismatchedTimestamps",
                              "Marker1 t=%u vs. Marker2 t=%u",
                              marker1.GetTimeStamp(), marker2.GetTimeStamp());
          ++markerIter2;
          continue;
        }

        // These two markers must be different and observed at the same time
        if(markerIter1 != markerIter2) {

          // See if #2 is inside #1
          bool marker2isInsideMarker1 = true;
          for(auto & corner : marker2.GetImageCorners()) {
            if(marker1.GetImageCorners().Contains(corner) == false) {
              marker2isInsideMarker1 = false;
              break;
            }
          }

          if(marker2isInsideMarker1) {
            PRINT_CH_INFO("BlockWorld", "BlockWorld.RemoveMarkersWithinMarkers",
                          "Removing %s marker completely contained within %s marker.",
                          marker2.GetCodeName(), marker1.GetCodeName());
            // Note: erase does increment of iterator for us
            markerIter2 = currentObsMarkers.erase(markerIter2);
          } else {
            // Need to iterate marker2
            ++markerIter2;
          } // if/else marker2isInsideMarker1
        } else {
          // Need to iterate marker2
          ++markerIter2;
        } // if/else marker1 != marker2 && time1 != time2
      } // for markerIter2
    } // for markerIter1

  } // RemoveMarkersWithinMarkers()

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result BlockWorld::SanityCheckBookkeeping() const
  {
    // Sanity check our containers to make sure each located object's properties
    // match the keys of the containers within which it is stored
    for(auto const& objectsByOrigin : _locatedObjects)
    {
      for(auto const& objectsByFamily : objectsByOrigin.second)
      {
        for(auto const& objectsByType : objectsByFamily.second)
        {
          for(auto const& objectsByID : objectsByType.second)
          {
            auto const& object = objectsByID.second;

            ANKI_VERIFY(objectsByID.first == object->GetID(),
                        "BlockWorld.SanityCheckBookkeeping.MismatchedID",
                        "%s Object has ID:%d but is keyed by ID:%d",
                        EnumToString(object->GetType()),
                        object->GetID().GetValue(), objectsByID.first.GetValue());

            ANKI_VERIFY(objectsByType.first == object->GetType(),
                        "BlockWorld.SanityCheckBookkeeping.MismatchedType",
                        "Object %d has Type:%s but is keyed by Type:%s",
                        object->GetID().GetValue(),
                        EnumToString(object->GetType()), EnumToString(objectsByType.first));

            ANKI_VERIFY(objectsByFamily.first == object->GetFamily(),
                        "BlockWorld.SanityCheckBookkeeping.MismatchedFamily",
                        "%s Object %d has Family:%s but is keyed by Family:%s",
                        EnumToString(object->GetType()), object->GetID().GetValue(),
                        EnumToString(object->GetFamily()), EnumToString(objectsByFamily.first));

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
    }

    return RESULT_OK;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result BlockWorld::UpdateObservedMarkers(const std::list<Vision::ObservedMarker>& currentObsMarkers)
  {
    ANKI_CPU_PROFILE("BlockWorld::UpdateObservedMarkers");

    const f32 currentTimeSec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if (_lastPlayAreaSizeEventSec + _playAreaSizeEventIntervalSec < currentTimeSec) {
      _lastPlayAreaSizeEventSec = currentTimeSec;
      const auto currentNavMemoryMap = _robot->GetMapComponent().GetCurrentMemoryMap();
      const double areaM2 = currentNavMemoryMap->GetExploredRegionAreaM2();
      Anki::Util::sInfoF("robot.play_area_size", {}, "%.2f", areaM2);
    }

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
      //RemoveMarkersWithinMarkers(currentObsMarkers);

      // Reset the flag telling us objects changed here, before we update any objects:
      _didObjectsChange = false;

      // Add, update, and/or localize the robot to any objects indicated by the
      // observed markers
      {
        // Sanity checks for robot's origin
        DEV_ASSERT(_robot->GetPose().IsChildOf(_robot->GetWorldOrigin()),
                   "BlockWorld.Update.RobotParentShouldBeOrigin");
        DEV_ASSERT(_robot->IsPoseInWorldOrigin(_robot->GetPose()),
                   "BlockWorld.Update.BadRobotOrigin");

        // Keep the objects sorted by increasing distance from the robot.
        // This will allow us to only use the closest object that can provide
        // localization information (if any) to update the robot's pose.
        // Note that we use a multimap to handle the corner case that there are two
        // objects that have the exact same distance. (We don't want to only report
        // seeing one of them and it doesn't matter which we use to localize.)
        std::multimap<f32, ObservableObject*> objectsSeen;

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

      // update stacks
      UpdatePoseOfStackedObjects();
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
        const bool propagateStack = true;
        _robot->GetObjectPoseConfirmer().MarkObjectDirty(object, propagateStack);
      };

      ModifyLocatedObjects(markAsDirty, unobservedCollidingObjectFilter);
    }

    Result lastResult = UpdateMarkerlessObjects(_robot->GetLastImageTimeStamp());

    if(ANKI_DEVELOPER_CODE)
    {
      DEV_ASSERT(RESULT_OK == SanityCheckBookkeeping(), "BlockWorld.UpdateObservedMarkers.SanityCheckBookkeepingFailed");
    }

    return lastResult;

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
                    "Marking object %s %d as 'dirty', because it intersects robot %d's bounding quad.",
                    EnumToString(object->GetType()), object->GetID().GetValue(), _robot->GetID());

      return true;
    }

    return false;
  }

  Result BlockWorld::UpdateMarkerlessObjects(RobotTimeStamp_t atTimestamp)
  {
    // Remove old obstacles or ones intersecting with robot (except cliffs)
    BlockWorldFilter filter;
    filter.AddAllowedFamily(ObjectFamily::MarkerlessObject);
    filter.AddIgnoreType(ObjectType::CliffDetection);

    const f32 robotBottom  = _robot->GetPose().GetTranslation().z();
    const f32 robotTop     = robotBottom + ROBOT_BOUNDING_Z;
    const Quad2f robotBBox = _robot->GetBoundingQuadXY(_robot->GetPose().GetWithRespectToRoot());

    filter.AddFilterFcn([this, atTimestamp, robotBottom, robotTop, &robotBBox](const ObservableObject* object) -> bool
                        {
                          if(object->GetLastObservedTime() + kMarkerlessObjectExpirationTime_ms < atTimestamp)
                          {
                            // Object has expired
                            PRINT_CH_DEBUG("BlockWorld", "BlockWorld.UpdateMarkerlessObjects.RemovingExpired",
                                           "%s %d not seen since %d. Current time=%d",
                                           EnumToString(object->GetType()), object->GetID().GetValue(),
                                           object->GetLastObservedTime(), (TimeStamp_t)atTimestamp);
                            return true;
                          }

                          if(object->GetLastObservedTime() >= atTimestamp)
                          {
                            // Don't remove by collision markerless objects that were _just_
                            // created/observed
                            return false;
                          }

                          Pose3d objectPoseWrtRobotOrigin;
                          if(true == object->GetPose().GetWithRespectTo(_robot->GetWorldOrigin(), objectPoseWrtRobotOrigin))
                          {
                            const f32  objectHeight = objectPoseWrtRobotOrigin.GetTranslation().z();
                            const bool inSamePlane = (objectHeight >= robotBottom && objectHeight <= robotTop);

                            if( inSamePlane )
                            {
                              const Quad2f objectBBox = object->GetBoundingQuadXY(objectPoseWrtRobotOrigin);

                              if( robotBBox.Intersects(objectBBox) )
                              {
                                // Object intersects robot's position
                                PRINT_CH_DEBUG("BlockWorld", "BlockWorld.UpdateMarkerlessObjects.RemovingIntersectWithRobot",
                                               "%s %d",
                                               EnumToString(object->GetType()), object->GetID().GetValue());
                                return true;
                              }
                            }
                          }

                          return false;
                        });

    DeleteLocatedObjects(filter);

    return RESULT_OK;
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
                    "Setting robot %d as localized to no object, because it "
                    "is currently localized to %s object with ID=%d, which is "
                    "about to be cleared.",
                    _robot->GetID(), ObjectTypeToString(object->GetType()), object->GetID().GetValue());
      _robot->SetLocalizedTo(nullptr);
    }

    // TODO: If this is a mat piece, check to see if there are any objects "on" it (COZMO-138)
    // If so, clear them too or update their poses somehow? (Deleting seems easier)

    // Check to see if this object is the one the robot is carrying.
    if(_robot->GetCarryingComponent().GetCarryingObject() == object->GetID()) {
      PRINT_CH_INFO("BlockWorld", "BlockWorld.ClearObjectHelper.ClearingCarriedObject",
                    "Clearing %s object %d which robot %d thinks it is carrying.",
                    ObjectTypeToString(object->GetType()),
                    object->GetID().GetValue(),
                    _robot->GetID());
      _robot->GetCarryingComponent().UnSetCarryingObjects();
    }

    if(_selectedObjectID == object->GetID()) {
      PRINT_CH_INFO("BlockWorld", "BlockWorld.ClearObjectHelper.ClearingSelectedObject",
                    "Clearing %s object %d which is currently selected.",
                    ObjectTypeToString(object->GetType()),
                    object->GetID().GetValue());
      _selectedObjectID.UnSet();
    }

    ObservableObject* objectOnTop = FindLocatedObjectOnTopOf(*object, STACKED_HEIGHT_TOL_MM);
    if(objectOnTop != nullptr)
    {
      ClearLocatedObjectHelper(objectOnTop);
    }

    // TODO: evaluate if we want the concept of "previously known to be somewhere"
    // _robot->GetObjectPoseConfirmer().MarkObjectUnknown(object);

    // Flag that we removed an object
    _didObjectsChange = true;
    _robotMsgTimeStampAtChange = Anki::Util::Max(_robot->GetLastMsgTimestamp(), _robot->GetMapComponent().GetCurrentMemoryMap()->GetLastChangedTimeStamp());
  }

  ObservableObject* BlockWorld::FindObjectOnTopOrUnderneathHelper(const ObservableObject& referenceObject,
                                                                  f32 zTolerance,
                                                                  const BlockWorldFilter& filterIn,
                                                                  bool onTop) const
  {
    // Three checks:
    // 1. objects are within same coordinate frame
    // 2. centroid of candidate object projected onto "ground" (XY) plane must lie within
    //    the check object's projected bounding box
    // 3. (a) for "onTop": bottom of candidate object must be "near" top of reference object
    //    (b) for "!onTop": top of canddiate object must be "near" bottom of reference object

    const Pose3d refWrtOrigin = referenceObject.GetPose().GetWithRespectToRoot();
    const Quad2f refProjectedQuad = referenceObject.GetBoundingQuadXY(refWrtOrigin);

    // Find the point at the top middle of the object on bottom
    // (or if !onTop, the bottom middle of the object on top)
    const f32 zSize = referenceObject.GetDimInParentFrame<'Z'>(refWrtOrigin);
    const f32 topOfObjectOnBottom = (refWrtOrigin.GetTranslation().z() +
                                    (onTop ? 0.5f : -0.5f) * zSize);

    BlockWorldFilter filter(filterIn);
    filter.AddIgnoreID(referenceObject.GetID());
    filter.AddFilterFcn(
      [&topOfObjectOnBottom, &refWrtOrigin, &refProjectedQuad, &zTolerance, &onTop](const ObservableObject* candidateObject) -> bool
      {
        // This should never happen: objects in blockworld should always have parents (and not be origins themselves)
        DEV_ASSERT(refWrtOrigin.HasParent(), "BlockWorld.FindLocatedObjectOnTopOfUnderneathHelper.NullParent");

        Pose3d candidateWrtOrigin;
        const bool inSameFrame = candidateObject->GetPose().GetWithRespectTo(refWrtOrigin.GetParent(), candidateWrtOrigin);
        if(!inSameFrame)
        {
          return false;
        }

        //re-assign z coordinate for intersection check
        const float candidateCurrentZ = candidateWrtOrigin.GetTranslation().z();
        Vec3f candidateProjectedTranslation = {candidateWrtOrigin.GetTranslation().x(),
                                                     candidateWrtOrigin.GetTranslation().y(),
                                                     refWrtOrigin.GetTranslation().z()};
        candidateWrtOrigin.SetTranslation(candidateProjectedTranslation);

        // perform intersection check
        const Quad2f candidateProjected = candidateObject->GetBoundingQuadXY(candidateWrtOrigin);
        const bool projectedQuadsIntersect = refProjectedQuad.Intersects(candidateProjected);

        // restore candidate z
        candidateProjectedTranslation.z() = candidateCurrentZ;
        candidateWrtOrigin.SetTranslation(candidateProjectedTranslation);

        if(!projectedQuadsIntersect)
        {
          return false;
        }

        // Find the point at bottom middle of the object we're checking to be on top
        // (or if !onTop, the top middle of object we're checking to be underneath)
        const f32 zSize = candidateObject->GetDimInParentFrame<'Z'>(candidateWrtOrigin);
        const f32 bottomOfCandidateObject = (candidateWrtOrigin.GetTranslation().z() +
                                            (onTop ? -0.5f : 0.5f) * zSize);

        // If the top of the bottom object and the bottom the candidate top object are
        // close enough together, return this as the object on top
        const f32 dist = std::abs(topOfObjectOnBottom - bottomOfCandidateObject);

        if(Util::IsFltLE(dist, zTolerance))
        {
          return true;
        }
        else
        {
          return false;
        }
      });

    ObservableObject* foundObject = FindLocatedObjectHelper(filter, nullptr, true);

    if(kVisualizeStacks && _robot->GetContext()->GetVizManager() != nullptr)
    {
      // Cheap method to visualize stacks as a cuboid around both objects involved
      const u32 stackID = referenceObject.GetID().GetValue() + (onTop ? 250 : 500);
      if(nullptr != foundObject)
      {
        Quad2f foundQuad = foundObject->GetBoundingQuadXY(foundObject->GetPose().GetWithRespectToRoot());

        // Get bounding box for the two object's projected bounding quads
        std::vector<Point2f> corners;
        corners.reserve(8);
        std::copy(refProjectedQuad.begin(), refProjectedQuad.end(), std::back_inserter(corners));
        std::copy(foundQuad.begin(), foundQuad.end(), std::back_inserter(corners));
        Rectangle<f32> bbox(corners);

        Pose3d vizPose(0, Z_AXIS_3D(), Point3f(bbox.GetXmid(), bbox.GetYmid(), topOfObjectOnBottom));

        // compute maxRotatedAxis_zValue: the value of the axis that contributes the most in Z after the object
        // has been rotated (like we do in the actual code, this is just render)
        const Vec3f& foundObjSize = foundObject->GetSize();
        const Vec3f zCoordRotation = foundObject->GetPose().GetWithRespectToRoot().GetRotation().GetRotationMatrix().GetRow(2);
        const float rotatedXAxis_zValue = std::abs(zCoordRotation.x() * foundObjSize.x());
        const float rotatedYAxis_zValue = std::abs(zCoordRotation.y() * foundObjSize.y());
        const float rotatedZAxis_zValue = std::abs(zCoordRotation.z() * foundObjSize.z());

        const float maxRotatedAxis_zValue = std::max( rotatedXAxis_zValue,
                                                      std::max(rotatedYAxis_zValue, rotatedZAxis_zValue) );
        const f32 height = topOfObjectOnBottom + maxRotatedAxis_zValue;

        _robot->GetContext()->GetVizManager()->DrawCuboid(stackID,
                                                          Point3f(bbox.GetHeight(),
                                                                  bbox.GetWidth(),
                                                                  height),
                                                          vizPose,
                                                          onTop ? ColorRGBA(0.5f,0.f,0.75f,0.6f) : ColorRGBA(0.75f,0.1f,0.5f,0.6f) );
      }
      else
      {
        _robot->GetContext()->GetVizManager()->EraseCuboid(stackID);
      }
    }

    return foundObject;
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
  void BlockWorld::DeleteIntersectingObjects(const Quad2f& quad,
                                           f32 padding_mm,
                                           const BlockWorldFilter& filter)
  {
    DeleteLocatedObjects(GetIntersectingObjectsFilter(quad, padding_mm, filter));
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::DeleteIntersectingObjects(const std::shared_ptr<ObservableObject>& object,
                                             f32 padding_mm,
                                             const BlockWorldFilter& filter)
  {
    Quad2f quadSeen = object->GetBoundingQuadXY(object->GetPose(), padding_mm);
    DeleteLocatedObjects(GetIntersectingObjectsFilter(quadSeen, padding_mm, filter));
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
    filter.SetIgnoreIDs(std::set<ObjectID>(_robot->GetCarryingComponent().GetCarryingObjects()));

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
      auto& familyContainer = originIter->second;

      if(filter.ConsiderOrigin(crntOriginID, _robot->GetPoseOriginList().GetCurrentOriginID()))
      {
        auto familyIter = familyContainer.begin();
        while(familyIter != familyContainer.end())
        {
          const ObjectFamily crntFamily = familyIter->first;
          auto& typeContainer = familyIter->second;

          if(filter.ConsiderFamily(crntFamily))
          {
            auto typeIter = typeContainer.begin();
            while(typeIter != typeContainer.end())
            {
              const ObjectType crntType = typeIter->first;
              auto& idContainer = typeIter->second;

              if(filter.ConsiderType(crntType))
              {
                auto idIter = idContainer.begin();
                while(idIter != idContainer.end())
                {
                  const ObjectID crntID = idIter->first;

                  ObservableObject* object = idIter->second.get();
                  if(nullptr == object)
                  {
                    // This shouldn't happen, but warn if we encounter it (for debugging) and remove it from the container
                    PRINT_NAMED_WARNING("BlockWorld.DeleteLocatedObjects.NullObject",
                                        "Deleting null object in origin %d with Family:%s Type:%s ID:%d",
                                        crntOriginID,
                                        EnumToString(crntFamily),
                                        EnumToString(crntType),
                                        crntID.GetValue());

                    idIter = idContainer.erase(idIter);
                  }
                  else if(filter.ConsiderObject(object))
                  {
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

                    idIter = idContainer.erase(idIter);
                  }
                  else
                  {
                    ++idIter;
                  }
                } // while(id)
              } // if(ConsiderType)

              if(idContainer.empty()) {
                // All IDs removed from this type, erase it
                typeIter = typeContainer.erase(typeIter);
              } else {
                ++typeIter;
              }
            } // while(type)
          } // if(ConsiderFamily)

          if(typeContainer.empty()) {
            // All types removed from this family, erase it
            familyIter = familyContainer.erase(familyIter);
          } else {
            ++familyIter;
          }
        } // while(family)
      } // if(ConsiderOrigin)

      if(familyContainer.empty()) {
        // All families removed from this origin, erase it
        originIter = _locatedObjects.erase(originIter);
      } else {
        ++originIter;
      }
    } // while(origin)

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
    // Markerless objects are not Actionable, so ignore them for selection
    filter.SetIgnoreFamilies({ObjectFamily::MarkerlessObject, ObjectFamily::CustomObject});
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
      } // for each family


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
          selectedObject->VisualizePreActionPoses(obstacles, &_robot->GetPose());
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
        if( object->GetFamily() == ObjectFamily::LightCube ) {
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
      webSender->Data()["objectFamily"] = ObjectFamilyToString(msg.objectFamily);
      webSender->Data()["objectType"] = ObjectTypeToString(msg.objectType);
      webSender->Data()["isActive"] = msg.isActive;
      webSender->Data()["timestamp"] = msg.timestamp;
    }

  } // SendObjectUpdateToWebViz()

} // namespace Vector
} // namespace Anki
