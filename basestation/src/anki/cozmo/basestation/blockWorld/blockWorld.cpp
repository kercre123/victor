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
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"

// Putting engine config include first so we get anki/common/types.h instead of anki/types.h
// TODO: Fix this types.h include mess (COZMO-3752)
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/poseOriginList.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/rect_impl.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/common/shared/utilities_shared.h"
#include "anki/cozmo/basestation/activeCube.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/bridge.h"
#include "anki/cozmo/basestation/charger.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/customObject.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/flatMat.h"
#include "anki/cozmo/basestation/humanHead.h"
#include "anki/cozmo/basestation/markerlessObject.h"
#include "anki/cozmo/basestation/mat.h"
#include "anki/cozmo/basestation/navMemoryMap/iNavMemoryMap.h"
#include "anki/cozmo/basestation/navMemoryMap/navMemoryMapFactory.h"
#include "anki/cozmo/basestation/navMemoryMap/quadData/navMemoryMapQuadData_Cliff.h"
#include "anki/cozmo/basestation/objectPoseConfirmer.h"
#include "anki/cozmo/basestation/platform.h"
#include "anki/cozmo/basestation/potentialObjectsForLocalizingTo.h"
#include "anki/cozmo/basestation/ramp.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/vision/basestation/observableObjectLibrary_impl.h"
#include "anki/vision/basestation/visionMarker.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/global/globalDefinitions.h"
#include "util/math/math.h"

// The amount of time a proximity obstacle exists beyond the latest detection
#define PROX_OBSTACLE_LIFETIME_MS  4000

// The sensor value that must be met/exceeded in order to have detected an obstacle
#define PROX_OBSTACLE_DETECT_THRESH   5

// TODO: Expose these as parameters
#define BLOCK_IDENTIFICATION_TIMEOUT_MS 500

#define DEBUG_ROBOT_POSE_UPDATES 0
#if DEBUG_ROBOT_POSE_UPDATES
#  define PRINT_LOCALIZATION_INFO(...) PRINT_NAMED_INFO("Localization", __VA_ARGS__)
#else
#  define PRINT_LOCALIZATION_INFO(...)
#endif



namespace Anki {
namespace Cozmo {

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// CONSOLE VARS

// how often we request redrawing maps. Added because I think clad is getting overloaded with the amount of quads
CONSOLE_VAR(float, kMemoryMapRenderRate_sec, "BlockWorld.MemoryMap", 0.25f);
  
// kObjectRotationChangeToReport_deg: if the rotation of an object changes by this much, memory map will be notified
CONSOLE_VAR(float, kObjectRotationChangeToReport_deg, "BlockWorld.MemoryMap", 10.0f);
// kObjectPositionChangeToReport_mm: if the position of an object changes by this much, memory map will be notified
CONSOLE_VAR(float, kObjectPositionChangeToReport_mm, "BlockWorld.MemoryMap", 5.0f);

// kRobotRotationChangeToReport_deg: if the rotation of the robot changes by this much, memory map will be notified
CONSOLE_VAR(float, kRobotRotationChangeToReport_deg, "BlockWorld.MemoryMap", 20.0f);
// kRobotPositionChangeToReport_mm: if the position of the robot changes by this much, memory map will be notified
CONSOLE_VAR(float, kRobotPositionChangeToReport_mm, "BlockWorld.MemoryMap", 8.0f);

// kOverheadEdgeCloseMaxLenForTriangle_mm: maximum length of the close edge to be considered a triangle instead of a quad
CONSOLE_VAR(float, kOverheadEdgeCloseMaxLenForTriangle_mm, "BlockWorld.MemoryMap", 15.0f);
// kOverheadEdgeFarMaxLenForLine_mm: maximum length of the far edge to be considered a line instead of a triangle or a quad
CONSOLE_VAR(float, kOverheadEdgeFarMaxLenForLine_mm, "BlockWorld.MemoryMap", 15.0f);
// kOverheadEdgeFarMinLenForLine_mm: minimum length of the far edge to even report the line
CONSOLE_VAR(float, kOverheadEdgeFarMinLenForClearReport_mm, "BlockWorld.MemoryMap", 3.0f); // tested 5 and was too big
// kOverheadEdgeSegmentNoiseLen_mm: segments whose length is smaller than this will be considered noise
CONSOLE_VAR(float, kOverheadEdgeSegmentNoiseLen_mm, "BlockWorld.MemoryMap", 6.0f);

// kDebugRenderOverheadEdges: enables/disables debug render of points reported from vision
CONSOLE_VAR(bool, kDebugRenderOverheadEdges, "BlockWorld.MemoryMap", false);
// kDebugRenderOverheadEdgeClearQuads: enables/disables debug render of nonBorder quads from overhead detection (clear)
CONSOLE_VAR(bool, kDebugRenderOverheadEdgeClearQuads, "BlockWorld.MemoryMap", false);
// kDebugRenderOverheadEdgeBorderQuads: enables/disables debug render of border quads only (interesting edges)
CONSOLE_VAR(bool, kDebugRenderOverheadEdgeBorderQuads, "BlockWorld.MemoryMap", false);

// kReviewInterestingEdges: if set to true, interesting edges are reviewed after adding new ones to see whether they are still interesting
CONSOLE_VAR(bool, kReviewInterestingEdges, "BlockWorld.kReviewInterestingEdges", true);

// Enable to draw (semi-experimental) bounding cuboids around stacks of 2 blocks
CONSOLE_VAR(bool, kVisualizeStacks, "BlockWorld", false);
  
// How long to wait until deleting non-cliff Markerless objects
CONSOLE_VAR(u32, kMarkerlessObjectExpirationTime_ms, "BlockWorld", 30000);
  
// Whether or not to put unrecognized markerless objects like collision/prox obstacles and cliffs into the memory map
CONSOLE_VAR(bool, kAddUnrecognizedMarkerlessObjectsToMemMap, "BlockWorld.MemoryMap", false);

// Whether or not to put custom objects in the memory map (COZMO-9360)
CONSOLE_VAR(bool, kAddCustomObjectsToMemMap, "BlockWorld.MemoryMap", false);
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helper namespace
namespace {

// return the content type we would set in the memory type for each object family
NavMemoryMapTypes::EContentType ObjectFamilyToMemoryMapContentType(ObjectFamily family, bool isAdding)
{
  using ContentType = NavMemoryMapTypes::EContentType;
  ContentType retType = ContentType::Unknown;
  switch(family)
  {
    case ObjectFamily::Block:
    case ObjectFamily::LightCube:
      // pick depending on addition or removal
      retType = isAdding ? ContentType::ObstacleCube : ContentType::ObstacleCubeRemoved;
      break;
    case ObjectFamily::Charger:
      retType = isAdding ? ContentType::ObstacleCharger : ContentType::ObstacleChargerRemoved;
      break;
    case ObjectFamily::MarkerlessObject:
    {
      // old .badIsAdding message
      if(!isAdding)
      {
        PRINT_NAMED_WARNING("ObjectFamilyToMemoryMapContentType.MarkerlessOject.RemovalNotSupported",
                            "ContentType MarkerlessObject removal is not supported. kAddUnrecognizedMarkerlessObjectsToMemMap was (%s)",
                            kAddUnrecognizedMarkerlessObjectsToMemMap ? "true" : "false");
      }
      else
      {
        PRINT_NAMED_WARNING("ObjectFamilyToMemoryMapContentType.MarkerlessOject.AdditionNotSupported",
                            "ContentType MarkerlessObject addition is not supported. kAddUnrecognizedMarkerlessObjectsToMemMap was (%s)",
                            kAddUnrecognizedMarkerlessObjectsToMemMap ? "true" : "false");
        // retType = ContentType::ObstacleUnrecognized;
      }
      break;
    }
      
    case ObjectFamily::CustomObject:
    {
      // old .badIsAdding message
      if(!isAdding)
      {
        PRINT_NAMED_WARNING("ObjectFamilyToMemoryMapContentType.CustomOject.RemovalNotSupported",
                            "ContentType CustomObject removal is not supported. kCustomObjectsToMemMap was (%s)",
                            kAddCustomObjectsToMemMap ? "true" : "false");
      }
      else
      {
        PRINT_NAMED_WARNING("ObjectFamilyToMemoryMapContentType.CustomOject.AdditionNotSupported",
                            "ContentType CustomObject addition is not supported. kCustomObjectsToMemMap was (%s)",
                            kAddCustomObjectsToMemMap ? "true" : "false");
      }
      break;
    }
      
    case ObjectFamily::Invalid:
    case ObjectFamily::Unknown:
    case ObjectFamily::Ramp:
    case ObjectFamily::Mat:
    break;
  };

  return retType;
}

};

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  BlockWorld::BlockWorld(Robot* robot)
  : _robot(robot)
  , _lastPlayAreaSizeEventSec(0)
  , _playAreaSizeEventIntervalSec(60)
  , _didObjectsChange(false)
  , _robotMsgTimeStampAtChange(0)
  , _currentNavMemoryMapOrigin(nullptr)
  , _isNavMemoryMapRenderEnabled(true)
  , _trackPoseChanges(false)
  , _memoryMapBroadcastRate_sec(-1.0f)
  , _nextMemoryMapBroadcastTimeStamp(0)
  , _blockConfigurationManager(new BlockConfigurations::BlockConfigurationManager(*robot))
  {
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
    
    // Long Bridge
    //_objectLibrary[ObjectFamily::Mat].AddObject(new Bridge(Bridge::Type::LONG_BRIDGE));
    
    // Short Bridge
    // TODO: Need to update short bridge markers so they don't look so similar to long bridge at oblique viewing angle
    // _objectLibrary[ObjectFamily::Mat].AddObject(new MatPiece(MatPiece::Type::SHORT_BRIDGE));
    
    
    //////////////////////////////////////////////////////////////////////////
    // Ramps
    //
    //_objectLibrary[ObjectFamily::RAMPS].AddObject(new Ramp());
    
    
    //////////////////////////////////////////////////////////////////////////
    // Charger
    //
    DefineObject(std::make_unique<Charger>());
    
    if(_robot->HasExternalInterface())
    {
      SetupEventHandlers(*_robot->GetExternalInterface());
    }
          
  } // BlockWorld() Constructor

  void BlockWorld::SetupEventHandlers(IExternalInterface& externalInterface)
  {
    using namespace ExternalInterface;
    auto helper = MakeAnkiEventUtil(externalInterface, *this, _eventHandles);
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DeleteAllCustomObjects>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SelectNextObject>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::CreateFixedCustomObject>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DefineCustomBox>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DefineCustomCube>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DefineCustomWall>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetMemoryMapRenderEnabled>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestLocatedObjectStates>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestConnectedObjects>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetMemoryMapBroadcastFrequency_sec>();
  }

  BlockWorld::~BlockWorld()
  {
    
  } // ~BlockWorld() Destructor
  
  Result BlockWorld::DefineObject(std::unique_ptr<const ObservableObject>&& object)
  {
    const ObjectFamily objFamily = object->GetFamily(); // Remove with COZMO-9319
    const ObjectType objType = object->GetType(); // Store due to std::move
    const Result addResult = _objectLibrary[objFamily].AddObject(std::move(object));
    
    if(RESULT_OK == addResult)
    {
      PRINT_CH_INFO("BlockWorld", "BlockWorld.DefineObject.AddedObjectDefinition",
                    "Defined %s in Object Library", EnumToString(objType));
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
  void BlockWorld::HandleMessage(const ExternalInterface::DeleteAllCustomObjects& msg)
  {
    _robot->GetContext()->GetVizManager()->EraseAllVizObjects();
    DeleteLocatedObjectsByFamily(ObjectFamily::CustomObject);
    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::RobotDeletedAllCustomObjects>(_robot->GetID());
  };
  
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
    
    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::CreatedFixedCustomObject>(_robot->GetID(), id);
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
    
    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::DefinedCustomObject>(_robot->GetID(),
                                                                                                          success);
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
    
    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::DefinedCustomObject>(_robot->GetID(),
                                                                                                          success);
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
    
    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::DefinedCustomObject>(_robot->GetID(),
                                                                                                          success);
  };
  
  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::SetMemoryMapRenderEnabled& msg)
  {
    SetMemoryMapRenderEnabled(msg.enabled);
  };

  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::RequestLocatedObjectStates& msg)
  {
    BroadcastLocatedObjectStates();
  };

  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::RequestConnectedObjects& msg)
  {
    BroadcastConnectedObjects();
  };
  
  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::SetMemoryMapBroadcastFrequency_sec& msg)
  {
    _memoryMapBroadcastRate_sec = msg.frequency;
    _nextMemoryMapBroadcastTimeStamp = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
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
      const TimeStamp_t atTimestamp = _currentObservedMarkerTimestamp;
      
      filter.AddFilterFcn([atTimestamp](const ObservableObject* object) -> bool
                          {
                            const bool seenAtTimestamp = (object->GetLastObservedTime() == atTimestamp);
                            return seenAtTimestamp;
                          });
    }
    
    for(auto & objectsByOrigin : _locatedObjects) {
      if(filter.ConsiderOrigin(objectsByOrigin.first, _robot->GetWorldOrigin())) {
        for(auto & objectsByFamily : objectsByOrigin.second) {
          if(filter.ConsiderFamily(objectsByFamily.first)) {
            for(auto & objectsByType : objectsByFamily.second) {
              if(filter.ConsiderType(objectsByType.first)) {
                for(auto & objectsByID : objectsByType.second) {
                  ObservableObject* object_nonconst = objectsByID.second.get();
                  const ObservableObject* object = object_nonconst;
                  
                  if(nullptr == object)
                  {
                    PRINT_NAMED_WARNING("BlockWorld.FindObjectHelper.NullExistingObject",
                                        "Origin:%s(%p) Family:%s Type:%s ID:%d is NULL",
                                        objectsByOrigin.first->GetName().c_str(),
                                        objectsByOrigin.first,
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
    
    RobotObservedObject observation(_robot->GetID(),
                                    observedObject->GetLastObservedTime(),
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
    filter.SetFilterFcn([this,&objectStates](const ObservableObject* obj)
                        {
                          ConnectedObjectState objectState(obj->GetID(),
                                                  obj->GetFamily(),
                                                  obj->GetType());
                          
                          objectStates.objects.push_back(std::move(objectState));
                          return true;
                        });
    
    // Iterate over all objects and add them to the available objects list if they pass the filter
    FindLocatedObjectHelper(filter, nullptr, false);
    
    _robot->Broadcast(MessageEngineToGame(std::move(objectStates)));
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result BlockWorld::UpdateObjectOrigin(const ObjectID& objectID, const Pose3d* oldOrigin)
  {
    auto originIter = _locatedObjects.find(oldOrigin);
    if(originIter == _locatedObjects.end())
    {
      PRINT_CH_INFO("BlockWorld", "BlockWorld.UpdateObjectOrigin.BadOrigin",
                    "Origin %s (%p) not found",
                    oldOrigin->GetName().c_str(), oldOrigin);
      
      return RESULT_FAIL;
    }
    
    for(auto & objectsByFamily : originIter->second)
    {
      for(auto & objectsByType : objectsByFamily.second)
      {
        auto objectIter = objectsByType.second.find(objectID);
        if(objectIter != objectsByType.second.end())
        {
          std::shared_ptr<ObservableObject> object = objectIter->second;
          const Pose3d* newOrigin  = &object->GetPose().FindOrigin();
          if(newOrigin != oldOrigin)
          {
            const ObjectFamily family  = object->GetFamily();
            const ObjectType   objType = object->GetType();
            
            DEV_ASSERT(family == objectsByFamily.first, "BlockWorld.UpdateObjectOrigin.FamilyMismatch");
            DEV_ASSERT(objType == objectsByType.first,  "BlockWorld.UpdateObjectOrigin.TypeMismatch");
            DEV_ASSERT(objectID == object->GetID(),     "BlockWorld.UpdateObjectOrigin.IdMismatch");
            
            PRINT_CH_INFO("BlockWorld", "BlockWorld.UpdateObjectOrigin.ObjectFound",
                          "Updating ObjectID %d from origin %s(%p) to %s(%p)",
                          objectID.GetValue(),
                          oldOrigin->GetName().c_str(), oldOrigin,
                          newOrigin->GetName().c_str(), newOrigin);
            
            // Add to object's current origin
            _locatedObjects[newOrigin][family][objType][objectID] = object;
            
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
                  "Object %d not found in origin %s (%p)",
                  objectID.GetValue(), oldOrigin->GetName().c_str(), oldOrigin);
    
    return RESULT_FAIL;
  }
  
  Result BlockWorld::UpdateObjectOrigins(const Pose3d *oldOrigin,
                                         const Pose3d *newOrigin)
  {
    Result result = RESULT_OK;
    
    if(nullptr == oldOrigin || nullptr == newOrigin) {
      PRINT_NAMED_ERROR("BlockWorld.UpdateObjectOrigins.OriginFail",
                        "Old and new origin must not be NULL");
      
      return RESULT_FAIL;
    }
    
    // Look for objects in the old origin
    BlockWorldFilter filterOld;
    filterOld.SetOriginMode(BlockWorldFilter::OriginMode::Custom);
    filterOld.AddAllowedOrigin(oldOrigin);
    
    // Use the modifier function to update matched objects to the new origin
    ModifierFcn originUpdater = [oldOrigin,newOrigin,&result,this](ObservableObject* oldObject)
    {
      Pose3d newPose;
      
      if(_robot->IsCarryingObject(oldObject->GetID()))
      {
        // Special case: don't use the pose w.r.t. the origin b/c carried objects' parent
        // is the lift. The robot is already in the new frame by the time this called,
        // so we don't need to adjust anything
        DEV_ASSERT(_robot->GetWorldOrigin() == newOrigin, "BlockWorld.UpdateObjectOrigins.RobotNotInNewOrigin");
        DEV_ASSERT(&oldObject->GetPose().FindOrigin() == newOrigin,
                   "BlockWorld.UpdateObjectOrigins.OldCarriedObjectNotInNewOrigin");
        newPose = oldObject->GetPose();
      }
      else if(false == oldObject->GetPose().GetWithRespectTo(*newOrigin, newPose))
      {
        PRINT_NAMED_ERROR("BlockWorld.UpdateObjectOrigins.OriginFail",
                          "Could not get object %d w.r.t new origin %s",
                          oldObject->GetID().GetValue(),
                          newOrigin->GetName().c_str());
        
        result = RESULT_FAIL;
        return;
      }
      
      const Vec3f& T_old = oldObject->GetPose().GetTranslation();
      const Vec3f& T_new = newPose.GetTranslation();
      
      // Look for a matching object in the new origin. Should have same family and type. If unique, should also have
      // same ID, or if not unique, the poses should match.
      BlockWorldFilter filterNew;
      filterNew.SetOriginMode(BlockWorldFilter::OriginMode::Custom);
      filterNew.AddAllowedOrigin(newOrigin);
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
                      oldOrigin->GetName().c_str(),
                      newOrigin->GetName().c_str(),
                      oldObject->IsUnique() ? "type" : "pose",
                      newObject->GetID().GetValue(),
                      T_old.x(), T_old.y(), T_old.z(),
                      T_new.x(), T_new.y(), T_new.z());
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
                      "Adding %s object with ID %d to new origin %s (%p)",
                      EnumToString(newObject->GetType()),
                      newObject->GetID().GetValue(),
                      newOrigin->GetName().c_str(),
                      newOrigin);
      }
      
    };
    
    // Apply the filter and modify each object that matches
    ModifyLocatedObjects(originUpdater, filterOld);
    
    if(RESULT_OK == result) {
      // Erase all the objects in the old frame now that their counterparts in the new
      // frame have had their poses updated
      DeleteLocatedObjectsByOrigin(oldOrigin);
    }
    
    // Now go through all the objects already in the origin we are switching _to_
    // (the "new" origin) and make sure PoseConfirmer knows about them since we
    // have delocalized since last being in this origin (which clears the PoseConfirmer)
    BlockWorldFilter filterNew;
    filterOld.SetOriginMode(BlockWorldFilter::OriginMode::Custom);
    filterOld.AddAllowedOrigin(newOrigin);
    
    ModifierFcn addToPoseConfirmer = [this,&newOrigin](ObservableObject* object){
      _robot->GetObjectPoseConfirmer().AddInExistingPose(object);
    };
    
    FindLocatedObjectHelper(filterNew, addToPoseConfirmer, false);
    
    // Notify the world about the objects in the new coordinate frame, in case
    // we added any based on rejiggering (not observation). Include unconnected
    // ones as well.
    BroadcastLocatedObjectStates();
    
    // if memory maps are enabled, we can merge old into new
    {
      // oldOrigin is the pointer/id of the map we were just building, and it's going away. It's the current map
      // newOrigin is the pointer/id of the map that is staying, it's the one we rejiggered to, and we haven't changed in a while
      DEV_ASSERT(_navMemoryMaps.find(oldOrigin) != _navMemoryMaps.end(),
                 "BlockWorld.UpdateObjectOrigins.missingMapOriginOld");
      DEV_ASSERT(_navMemoryMaps.find(newOrigin) != _navMemoryMaps.end(),
                 "BlockWorld.UpdateObjectOrigins.missingMapOriginNew");
      DEV_ASSERT(oldOrigin == _currentNavMemoryMapOrigin, "BlockWorld.UpdateObjectOrigins.updatingMapNotCurrent");

      // before we merge the object information from the memory maps, apply rejiggering also to their
      // reported poses
      UpdateOriginsOfObjectsReportedInMemMap(oldOrigin, newOrigin);

      // grab the underlying memory map and merge them
      INavMemoryMap* oldMap = _navMemoryMaps[oldOrigin].get();
      INavMemoryMap* newMap = _navMemoryMaps[newOrigin].get();
      
      // COZMO-6184: the issue localizing to a zombie map was related to a cube being disconnected while we delocalized.
      // The issue has been fixed, but this code here would have prevented a crash and produce an error instead, so I
      // am going to keep the code despite it may not run anymore
      if ( nullptr == newMap )
      {
        // error to identify the issue
        PRINT_NAMED_ERROR("BlockWorld.UpdateObjectOrigins.NullMapFound",
                          "Origin '%s' did not have a memory map. Creating empty",
                          newOrigin->GetName().c_str());
        
        // create empty map since somehow we lost the one we had
        VizManager* vizMgr = _robot->GetContext()->GetVizManager();
        INavMemoryMap* emptyMemoryMap = NavMemoryMapFactory::CreateDefaultNavMemoryMap(vizMgr, _robot);
        
        // set in the container of maps
        _navMemoryMaps[newOrigin].reset(emptyMemoryMap);
        // set the pointer to this newly created instance
        newMap = emptyMemoryMap;
      }

      // continue the merge as we were going to do, so at least we don't lose the information we were just collecting
      Pose3d oldWrtNew;
      const bool success = oldOrigin->GetWithRespectTo(*newOrigin, oldWrtNew);
      DEV_ASSERT(success, "BlockWorld.UpdateObjectOrigins.BadOldWrtNull");
      newMap->Merge(oldMap, oldWrtNew);
      
      // switch back to what is becoming the new map
      _currentNavMemoryMapOrigin = newOrigin;
      
      // now we can delete what is become the old map, since we have merged its data into the new one
      _navMemoryMaps.erase( oldOrigin ); // smart pointer will delete memory
    }
    
    // Since object origins have changed we need to force a configuration
    // update during the configuration manager's update
    _blockConfigurationManager->FlagForRebuild();
    
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
                      "Deleting objects from (%p) because it was zombie", originIt->first);
        originIt = _locatedObjects.erase(originIt);
      } else {
        PRINT_CH_DEBUG("BlockWorld", "BlockWorld.DeleteObjectsFromZombieOrigins.KeepingOrigin", 
                       "Origin (%p) is still good (keeping objects)", originIt->first);
        ++originIt;
      }
    }
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  const INavMemoryMap* BlockWorld::GetNavMemoryMap() const
  {
    // current map (if any) must match current robot origin
    DEV_ASSERT((_currentNavMemoryMapOrigin == nullptr) || (_robot->GetWorldOrigin() == _currentNavMemoryMapOrigin),
               "BlockWorld.GetNavMemoryMap.BadOrigin");
    
    const INavMemoryMap* curMap = nullptr;
    if ( nullptr != _currentNavMemoryMapOrigin ) {
      auto matchPair = _navMemoryMaps.find(_currentNavMemoryMapOrigin);
      if ( matchPair != _navMemoryMaps.end() ) {
        curMap = matchPair->second.get();
      } else {
        DEV_ASSERT(false, "BlockWorld.GetNavMemoryMap.MissingMap");
      }
    }
    return curMap;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  INavMemoryMap* BlockWorld::GetNavMemoryMap()
  {
    INavMemoryMap* curMap = nullptr;
    if ( nullptr != _currentNavMemoryMapOrigin ) {
      auto matchPair = _navMemoryMaps.find(_currentNavMemoryMapOrigin);
      if ( matchPair != _navMemoryMaps.end() ) {
        curMap = matchPair->second.get();
      } else {
        DEV_ASSERT(false, "BlockWorld.GetNavMemoryMap.MissingMap");
      }
    }
    return curMap;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::UpdateRobotPoseInMemoryMap()
  {
    ANKI_CPU_PROFILE("BlockWorld::UpdateRobotPoseInMemoryMap");
    
    // grab current robot pose
    DEV_ASSERT(_robot->GetWorldOrigin() == _currentNavMemoryMapOrigin, "BlockWorld.OnRobotPoseChanged.InvalidWorldOrigin");
    const Pose3d& robotPose = _robot->GetPose();
    const Pose3d& robotPoseWrtOrigin = robotPose.GetWithRespectToOrigin();
    
    // check if we have moved far enough that we need to resend
    const Point3f distThreshold(kRobotPositionChangeToReport_mm, kRobotPositionChangeToReport_mm, kRobotPositionChangeToReport_mm);
    const Radians angleThreshold( DEG_TO_RAD(kRobotRotationChangeToReport_deg) );
    const bool isPrevSet = (_navMapReportedRobotPose.GetParent() != nullptr);
    const bool isFarFromPrev = !isPrevSet || (!robotPoseWrtOrigin.IsSameAs(_navMapReportedRobotPose, distThreshold, angleThreshold));
      
    // if we need to add
    const bool addAgain = isFarFromPrev;
    if ( addAgain )
    {
      INavMemoryMap* currentNavMemoryMap = GetNavMemoryMap();
      DEV_ASSERT(currentNavMemoryMap, "BlockWorld.UpdateRobotPoseInMemoryMap.NoMemoryMap");
      // cliff quad: clear or cliff
      {
        // TODO configure this size somethere else
        Point3f cliffSize = MarkerlessObject(ObjectType::ProxObstacle).GetSize() * 0.5f;
        Quad3f cliffquad {
          {+cliffSize.x(), +cliffSize.y(), cliffSize.z()},  // up L
          {-cliffSize.x(), +cliffSize.y(), cliffSize.z()},  // lo L
          {+cliffSize.x(), -cliffSize.y(), cliffSize.z()},  // up R
          {-cliffSize.x(), -cliffSize.y(), cliffSize.z()}}; // lo R
        robotPoseWrtOrigin.ApplyTo(cliffquad, cliffquad);

        // depending on cliff on/off, add as ClearOfCliff or as Cliff
        if ( _robot->IsCliffDetected() )
        {
          // build data we want to embed for this quad
          NavMemoryMapQuadData_Cliff cliffData;
          Vec3f rotatedFwdVector = robotPoseWrtOrigin.GetRotation() * X_AXIS_3D();
          cliffData.directionality = Vec2f{rotatedFwdVector.x(), rotatedFwdVector.y()};
          currentNavMemoryMap->AddQuad(cliffquad, cliffData);
        }
        else
        {
          currentNavMemoryMap->AddQuad(cliffquad, INavMemoryMap::EContentType::ClearOfCliff);
        }
      }

      const Quad2f& robotQuad = _robot->GetBoundingQuadXY(robotPoseWrtOrigin);

      // regular clear of obstacle
      currentNavMemoryMap->AddQuad(robotQuad, INavMemoryMap::EContentType::ClearOfObstacle );

      // also notify behavior whiteboard.
      // rsam: should this information be in the map instead of the whiteboard? It seems a stretch that
      // blockworld knows now about behaviors, maybe all this processing of quads should be done in a separate
      // robot component, like a VisualInformationProcessingComponent
      _robot->GetAIComponent().GetWhiteboard().ProcessClearQuad(robotQuad);

      // update las reported pose
      _navMapReportedRobotPose = robotPoseWrtOrigin;
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::FlagGroundPlaneROIInterestingEdgesAsUncertain()
  {
    // get quad wrt robot
    const Pose3d& curRobotPose = _robot->GetPose().GetWithRespectToOrigin();
    Quad3f groundPlaneWrtRobot;
    curRobotPose.ApplyTo(GroundPlaneROI::GetGroundQuad(), groundPlaneWrtRobot);
    
    // ask memory map to clear
    INavMemoryMap* currentNavMemoryMap = GetNavMemoryMap();
    DEV_ASSERT(currentNavMemoryMap, "BlockWorld.FlagGroundPlaneROIInterestingEdgesAsUncertain.NullMap");
    const INavMemoryMap::EContentType typeInteresting = INavMemoryMap::EContentType::InterestingEdge;
    const INavMemoryMap::EContentType typeUnknown = INavMemoryMap::EContentType::Unknown;
    currentNavMemoryMap->ReplaceContent(groundPlaneWrtRobot, typeInteresting, typeUnknown);
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::FlagQuadAsNotInterestingEdges(const Quad2f& quadWRTOrigin)
  {
    INavMemoryMap* currentNavMemoryMap = GetNavMemoryMap();
    DEV_ASSERT(currentNavMemoryMap, "BlockWorld.FlagQuadAsNotInterestingEdges.NullMap");
    currentNavMemoryMap->AddQuad(quadWRTOrigin, INavMemoryMap::EContentType::NotInterestingEdge);
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::FlagInterestingEdgesAsUseless()
  {
    // flag all content as Unknown: ideally we would add a new type (SmallInterestingEdge), so that we know
    // we detected something, but we discarded it because it didn't have enough info; however that increases
    // complexity when raycasting, finding boundaries, readding edges, etc. By flagging Unknown we simply say
    // "there was something here, but we are not sure what it was", which can be good to re-explore the area
  
    INavMemoryMap* currentNavMemoryMap = GetNavMemoryMap();
    DEV_ASSERT(currentNavMemoryMap, "BlockWorld.FlagInterestingEdgesAsUseless.NullMap");
    const INavMemoryMap::EContentType newType = INavMemoryMap::EContentType::Unknown;
    currentNavMemoryMap->ReplaceContent(INavMemoryMap::EContentType::InterestingEdge, newType);
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::CreateLocalizedMemoryMap(const Pose3d* worldOriginPtr)
  {
    // Since we are going to create a new memory map, check if any of the existing ones have become a zombie
    // This could happen if either the current map never saw a localizable object, or if objects in previous maps
    // have been moved or deactivated, which invalidates them as localizable
    NavMemoryMapTable::iterator iter = _navMemoryMaps.begin();
    while ( iter != _navMemoryMaps.end() )
    {
      const bool isZombie = IsZombiePoseOrigin( iter->first );
      if ( isZombie ) {
        // PRINT_CH_DEBUG("BlockWorld", "CreateLocalizedMemoryMap", "Deleted map (%p) because it was zombie", iter->first);
        LOG_EVENT("blockworld.memory_map.deleting_zombie_map", "%s", iter->first->GetName().c_str() );
        iter = _navMemoryMaps.erase(iter);
        
        // also remove the reported poses in this origin for every object (fixes a leak, and better tracks where objects are)
        for( auto& posesForObjectIt : _navMapReportedPoses ) {
          OriginToPoseInMapInfo& posesPerOriginForObject = posesForObjectIt.second;
          const Pose3d* zombieOrigin = iter->first;
          posesPerOriginForObject.erase( zombieOrigin );
        }
      } else {
        //PRINT_CH_DEBUG("BlockWorld", "CreateLocalizedMemoryMap", "Map (%p) is still good", iter->first);
        LOG_EVENT("blockworld.memory_map.keeping_alive_map", "%s", iter->first->GetName().c_str() );
        ++iter;
      }
    }
    
    // clear all memory map rendering because indexHints are changing
    ClearNavMemoryMapRender();
    
    // if the origin is null, we would never merge the map, which could leak if a new one was created
    // do not support this by not creating one at all if the origin is null
    DEV_ASSERT(nullptr != worldOriginPtr, "BlockWorld.CreateLocalizedMemoryMap.NullOrigin");
    if ( nullptr != worldOriginPtr )
    {
      // create a new memory map in the given origin
      VizManager* vizMgr = _robot->GetContext()->GetVizManager();
      INavMemoryMap* navMemoryMap = NavMemoryMapFactory::CreateDefaultNavMemoryMap(vizMgr, _robot);
      _navMemoryMaps.emplace( std::make_pair(worldOriginPtr, std::unique_ptr<INavMemoryMap>(navMemoryMap)) );
      _currentNavMemoryMapOrigin = worldOriginPtr;
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::DrawNavMemoryMap() const
  {
    if(ANKI_DEV_CHEATS)
    {
      if ( _isNavMemoryMapRenderEnabled )
      {
        // check refresh rate
        static f32 nextDrawTimeStamp = 0;
        const f32 currentTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        if ( nextDrawTimeStamp > currentTimeInSeconds ) {
          return;
        }
        // we are rendering reset refresh time
        nextDrawTimeStamp = currentTimeInSeconds + kMemoryMapRenderRate_sec;
     
        size_t lastIndexNonCurrent = 0;
      
        // rendering all current maps with indexHint
        for (const auto& memMapPair : _navMemoryMaps)
        {
          const bool isCurrent = memMapPair.first == _currentNavMemoryMapOrigin;
          
          size_t indexHint = isCurrent ? 0 : (++lastIndexNonCurrent);
          memMapPair.second->Draw(indexHint);
        }
      }
    }
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::BroadcastNavMemoryMap()
  {
    if ( _memoryMapBroadcastRate_sec >= 0.0f )
    {
      const float currentTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if (FLT_GT(_nextMemoryMapBroadcastTimeStamp, currentTimeInSeconds)) {
        return;
      }
      // Reset the timer but don't accumulate error
      do {
        _nextMemoryMapBroadcastTimeStamp += _memoryMapBroadcastRate_sec;
      } while (FLT_LE(_nextMemoryMapBroadcastTimeStamp, currentTimeInSeconds));

      // Send only the current map
      const auto& currentOriginMap = _navMemoryMaps.find(_currentNavMemoryMapOrigin);
      if (currentOriginMap != _navMemoryMaps.end()) {
        // Look up and send the origin ID also
        const auto& originList = _robot->GetPoseOriginList();
        const uint32_t originID = originList.GetOriginID(currentOriginMap->first);
        if (originID != PoseOriginList::UnknownOriginID) {
          currentOriginMap->second->Broadcast(originID);
        }
      }
    }
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::ClearNavMemoryMapRender() const
  {
    if(ANKI_DEV_CHEATS)
    {
      for ( const auto& memMapPair : _navMemoryMaps )
      {
        memMapPair.second->ClearDraw();
      }
    }
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::SetMemoryMapRenderEnabled(bool enabled)
  {
    // if disabling, clear render now. If enabling wait until next render time
    if ( _isNavMemoryMapRenderEnabled && !enabled ) {
      ClearNavMemoryMapRender();
    }
  
    // set new value
    _isNavMemoryMapRenderEnabled = enabled;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result BlockWorld::AddAndUpdateObjects(const std::multimap<f32, ObservableObject*>& objectsSeen,
                                         const ObjectFamily& inFamily,
                                         const TimeStamp_t atTimestamp)
  {
    const Pose3d* currFrame = &_robot->GetPose().FindOrigin();
    
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
      
        // Add observation
        const bool wasRobotMoving = false; // assume false, otherwise we wouldn't have gotten this far w/ marker?
        const bool isConfirmingObservation = _robot->GetObjectPoseConfirmer().AddVisualObservation(objSeen,
                                                                                curMatchInOrigin,
                                                                                wasRobotMoving,
                                                                                distToObjSeen);
        if ( !isConfirmingObservation ) {
          PRINT_CH_INFO("BlockWorld", "BlockWorld.AddAndUpdateObjects.NonConfirmingObservation",
            "Added non-confirming visual observation for %d", objSeen->GetID().GetValue() );
          
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
      std::map<const Pose3d*, ObservableObject*> matchingObjects;
      
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
        
        if(objSeen->IsActive())
        {
          const ActiveObject* conObjMatch = GetConnectedActiveObjectByID( objSeen->GetID() );
          if(nullptr == conObjMatch)
          {
            // We expect to have already heard from all (powered) active objects,
            // so if we see one we haven't heard from (and therefore added) yet, then
            // perhaps it isn't on?
            PRINT_NAMED_WARNING("BlockWorld.AddAndUpdateObjects.NoMatchForActiveObject",
                                "Observed active object of type %s but it's not connected. Is the battery plugged in?",
                                EnumToString(objSeen->GetType()));
          }
        }

        bool matchedCarryingObject = false;
        for(ObservableObject* objectFound : objectsFound)
        {
          assert(nullptr != objectFound);
          
          const Pose3d* origin = &objectFound->GetPose().FindOrigin();
          
          if(origin == currFrame)
          {
            // handle special case of seeing the object that we are carrying. Alternatively all this code could
            // be in potential objects to localize to or in addObservation, but here it seems to detect early
            // what's going on
            if (_robot->IsCarryingObject(objectFound->GetID()))
            {
              if (_robot->GetLiftHeight() >= LIFT_HEIGHT_HIGHDOCK &&
                  objectFound->IsSameAs(*objSeen))
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
              else
              {
                // Otherwise, it must've been moved off the lift so unset its carry state
                // and update it as normal
                PRINT_CH_INFO("BlockWorld", "BlockWorld.AddAndUpdateObjects.SeeingCarryingObjectNotOnLift",
                              "Thought we were carrying object %d but seeing it in non-carry pose",
                              _robot->GetCarryingObject().GetValue());
                _robot->UnSetCarryObject(objectFound->GetID());
                
                // if the observation was used to correct the pose, it should have dettached
                DEV_ASSERT(objectFound->GetPose().GetParent() != &_robot->GetLiftPose(),
                           "BlockWorld.ConfirmingObservationDidNotCorrectLift");
                
                // moreover, only the confirming observation should have to do this
                DEV_ASSERT(observationAlreadyUsed, "BlockWorld.NonConfirmingObservationStillCarryingObject");
              }
            }
          }
          
          // Check for duplicates (same type observed in the same coordinate frame)
          // and add to our map of matches by origin
          auto iter = matchingObjects.find(origin);
          if(iter != matchingObjects.end()) {
            PRINT_NAMED_WARNING("BlockWorld.AddAndUpdateObjects.MultipleMatchesForUniqueObjectInSameFrame",
                                "Observed unique object of type %s matches multiple existing objects of "
                                "same type and in the same frame (%p).",
                                EnumToString(objSeen->GetType()), origin);
          } else {
            matchingObjects[origin] = objectFound;
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
        const ObjectID& carryingObjectID = _robot->GetCarryingObject();
        filter.AddFilterFcn([&carryingObjectID](const ObservableObject* candidate) {
          const bool isObjectBeingCarried = (candidate->GetID() == carryingObjectID);
          return !isObjectBeingCarried;
        });
        
        ObservableObject* matchingObject = FindLocatedClosestMatchingObject(*objSeen,
                                                                            objSeen->GetSameDistanceTolerance(),
                                                                            objSeen->GetSameAngleTolerance(),
                                                                            filter);
        if (nullptr != matchingObject) {
          DEV_ASSERT(&matchingObject->GetPose().FindOrigin() == currFrame,
                     "BlockWorld.AddAndUpdateObjects.MatchedPassiveObjectInOtherCoordinateFrame");
          matchingObjects[currFrame] = matchingObject;
        }
        
      } // if/else object is active

      // We know the object was confirmed, so we must have found the instance in the current origin
      ObservableObject* observedObject = nullptr;
      
      auto currFrameMatchIter = matchingObjects.find(currFrame);
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
      DEV_ASSERT(&observedObject->GetPose().FindOrigin() == currFrame,
                 "BlockWorld.AddAndUpdateObjects.ObservedObjectNotInCurrentFrame");
      
      // Add all observed markers of this object as occluders, once it has been
      // identified (it's possible we're seeing an existing object behind its
      // last-known location, in which case we don't want to delete the existing
      // one before realizing the new observation is the same object):
      if(ActiveIdentityState::Identified == observedObject->GetIdentityState())
      {
        std::vector<const Vision::KnownMarker *> observedMarkers;
        observedObject->GetObservedMarkers(observedMarkers);
        for(auto marker : observedMarkers) {
          _robot->GetVisionComponent().GetCamera().AddOccluder(*marker);
        }
      }
      
      const ObjectID obsID = observedObject->GetID();
      DEV_ASSERT(obsID.IsSet(), "BlockWorld.AddAndUpdateObjects.IDnotSet");
      
      
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
      _robotMsgTimeStampAtChange = atTimestamp;
      
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
      if ( changedObjectPtr )
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

        // find object
        ObservableObject* objectOnTopOfOldPose = FindLocatedObjectOnTopOf(*myOldCopy, STACKED_HEIGHT_TOL_MM, filter);
        if ( objectOnTopOfOldPose )
        {
          // we found an object currently on top of our old pose
          const ObjectID& topID = objectOnTopOfOldPose->GetID();
          
          // if this is not an object we are carrying
          if ( !_robot->IsCarryingObject(topID) )
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

  u32 BlockWorld::CheckForUnobservedObjects(TimeStamp_t atTimestamp)
  {
    u32 numVisibleObjects = 0;
    
    // Don't bother if the robot is picked up or if it was moving too fast to
    // have been able to see the markers on the objects anyway.
    // NOTE: Just using default speed thresholds, which should be conservative.
    if(_robot->GetOffTreadsState() != OffTreadsState::OnTreads ||
       _robot->GetVisionComponent().WasMovingTooFast(atTimestamp))
    {
      return numVisibleObjects;
    }
    
    // Create a list of observed and unobserved objects for further consideration below
    std::vector<ObservableObject*> unobservedObjects; // not const pointers b/c we may mark as unobserved below
    std::vector<const ObservableObject*> observedObjects;
    
    auto originIter = _locatedObjects.find(_robot->GetWorldOrigin());
    if(originIter == _locatedObjects.end()) {
      // No objects relative to this origin: Nothing to do
      return numVisibleObjects;
    }
    
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
          
          bool objectDeleted = false;
          
          // 1. Store objects we have just seen as "observed"
          // 2. Look for "unobserved" objects not seen atTimestamp -- but skip objects:
          //    - that are currently being carried
          //    - that we are currently docking to
          //    - whose pose origin does not match the robot's
          //    - who are a charger (since those stay around)
          if(object->GetLastObservedTime() >= atTimestamp)
          {
            observedObjects.push_back(object);
          }
          else if(_robot->GetCarryingObject() != object->GetID() &&
                  _robot->GetDockObject() != object->GetID() &&
                  &object->GetPose().FindOrigin() == _robot->GetWorldOrigin() &&
                  object->GetFamily() != ObjectFamily::Charger)
          {
            if(object->IsActive() &&
               ActiveIdentityState::WaitingForIdentity == object->GetIdentityState() &&
               object->GetLastObservedTime() < atTimestamp - BLOCK_IDENTIFICATION_TIMEOUT_MS) {
              
              // If this is an active object and identification has timed out
              // delete it if radio connection has not been established yet.
              // Otherwise, retry identification.
              if (object->GetActiveID() < 0) {
                PRINT_CH_INFO("BlockWorld", "BlockWorld.CheckForUnobservedObjects.IdentifyTimedOut",
                              "Deleting unobserved %s active object %d that has "
                              "not completed identification in %dms",
                              EnumToString(object->GetType()),
                              object->GetID().GetValue(), BLOCK_IDENTIFICATION_TIMEOUT_MS);
                
                objectIter = DeleteLocatedObjectAt(objectIter, objectsByType.first, objectFamily.first);
                objectDeleted = true;
              } else {
                // Don't delete objects that are still in radio communication. Retrigger Identify?
                //PRINT_NAMED_WARNING("BlockWorld.CheckForUnobservedObjects.RetryIdentify", "Re-attempt identify on object %d (%s)", object->GetID().GetValue(), EnumToString(object->GetType()));
                //object->Identify();
              }
              
            } else {
              // Otherwise, add it to the list for further checks below to see if
              // we "should" have seen the object
              if(_unidentifiedActiveObjects.count(object->GetID()) == 0) {
                unobservedObjects.push_back(object);
              }
            }
          }
          
          if(!objectDeleted)
          {
            ++objectIter;
          }
          
        } // for object IDs of this type
      } // for each object type
    } // for each object family
    
    // TODO: Don't bother with this if the robot is docking? (picking/placing)??
    // Now that the occlusion maps are complete, check each unobserved object's
    // visibility in each camera
    const Vision::Camera& camera = _robot->GetVisionComponent().GetCamera();
    DEV_ASSERT(camera.IsCalibrated(), "BlockWorld.CheckForUnobservedObjects.CameraNotCalibrated");
    for(ObservableObject* unobservedObject : unobservedObjects) {
      
      // Remove objects that should have been visible based on their last known
      // location, but which must not be there because we saw something behind
      // that location:
      const u16 xBorderPad = static_cast<u16>(0.05*static_cast<f32>(camera.GetCalibration()->GetNcols()));
      const u16 yBorderPad = static_cast<u16>(0.05*static_cast<f32>(camera.GetCalibration()->GetNrows()));
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
        // Make sure there are no currently-observed, (just-)identified objects
        // with the same active ID present. (If there are, we'll reassign IDs
        // on the next update instead of clearing the existing object now.)
        bool matchingActiveIdFound = false;
        if(unobservedObject->IsActive()) {
          for(auto object : observedObjects) {
            if(ActiveIdentityState::Identified == object->GetIdentityState() &&
               object->GetActiveID() == unobservedObject->GetActiveID()) {
              matchingActiveIdFound = true;
              break;
            }
          }
        }
        
        if(!matchingActiveIdFound) {
          // We "should" have seen the object! Clear it.
          PRINT_CH_INFO("BlockWorld", "BlockWorld.CheckForUnobservedObjects.MarkingUnobservedObject",
                        "Marking object %d unobserved, which should have been seen, but wasn't. "
                        "(shouldBeVisible:%d hasNothingBehind:%d isDirty:%d",
                        unobservedObject->GetID().GetValue(),
                        shouldBeVisible, hasNothingBehind, isDirtyPoseState);
          
          Result markResult = _robot->GetObjectPoseConfirmer().MarkObjectUnobserved(unobservedObject);
          if(RESULT_OK != markResult)
          {
            PRINT_NAMED_WARNING("BlockWorldCheckForUnobservedObjects.MarkObjectUnobservedFailed", "");
        }
      }
      }
      else if(unobservedObject->GetFamily() != ObjectFamily::Mat && !_robot->IsCarryingObject(unobservedObject->GetID()))
      {
        // If the object should _not_ be visible (i.e. none of its markers project
        // into the camera), but some part of the object is within frame, it is
        // close enough, and was seen fairly recently, then
        // let listeners know it's "visible" but not identifiable, so we can
        // still interact with it in the UI, for example.
        
        // Did we see this currently-unobserved object in the last N seconds?
        // This is to avoid using this feature (reporting unobserved objects
        // that project into the image as observed) too liberally, and instead
        // only for objects seen pretty recently, e.g. for the case that we
        // have driven in too close and can't see an object we were just approaching.
        // TODO: Expose / remove / fine-tune this setting
        const s32 seenWithin_sec = -1; // Set to <0 to disable
        const bool seenRecently = (seenWithin_sec < 0 ||
                                   _robot->GetLastMsgTimestamp() - unobservedObject->GetLastObservedTime() < seenWithin_sec*1000);
        
        // How far away is the object from our current position? Again, to be
        // conservative, we are only going to use this feature if the object is
        // pretty close to the robot.
        // TODO: Expose / remove / fine-tune this setting
        const f32 distThreshold_mm = -1.f; // 150.f; // Set to <0 to disable
        const bool closeEnough = (distThreshold_mm < 0.f ||
                                  (_robot->GetPose().GetTranslation() -
                                  unobservedObject->GetPose().GetTranslation()).LengthSq() < distThreshold_mm*distThreshold_mm);
        
        // Check any of the markers should be visible and that the reason for
        // them not being visible is not occlusion.
        // For now just ignore the left and right 22.5% of the image blocked by the lift,
        // *iff* we are using VGA images, which have a wide enough FOV to be occluded
        // by the lift. (I.e., assume QVGA is a cropped, narrower FOV)
        // TODO: Actually project a lift into the image and figure out what it will occlude
        u16 xBorderPad = 0;
        switch(camera.GetCalibration()->GetNcols())
        {
          case 640:
            xBorderPad = static_cast<u16>(0.225f * static_cast<f32>(camera.GetCalibration()->GetNcols()));
            break;
          case 400:
            // TODO: How much should be occluded?
            xBorderPad = static_cast<u16>(0.20f * static_cast<f32>(camera.GetCalibration()->GetNcols()));
            break;
          case 320:
            // Nothing to do, leave at zero
            break;
          default:
            // Not expecting other resolutions
            PRINT_NAMED_WARNING("BlockWorld.CheckForUnobservedObjects",
                                "Unexpeted camera calibration ncols=%d.",
                                camera.GetCalibration()->GetNcols());
        }
        
        Vision::KnownMarker::NotVisibleReason reason;
        bool markersShouldBeVisible = false;
        bool markerIsOccluded = false;
        for(auto & marker : unobservedObject->GetMarkers()) {
          if(marker.IsVisibleFrom(_robot->GetVisionComponent().GetCamera(), DEG_TO_RAD(45), 20.f, false, xBorderPad, 0, reason)) {
            // As soon as one marker is visible, we can stop
            markersShouldBeVisible = true;
            break;
          } else if(reason == Vision::KnownMarker::NotVisibleReason::OCCLUDED) {
            // Flag that any of the markers was not visible because it was occluded
            // If this is the case, then we don't want to signal this object as
            // partially visible.
            markerIsOccluded = true;
          }
          // This should never be true because we set requireSomethingBehind to false in IsVisibleFrom() above.
          assert(reason != Vision::KnownMarker::NotVisibleReason::NOTHING_BEHIND);
        }

        if(seenRecently && closeEnough && !markersShouldBeVisible && !markerIsOccluded)
        {
          // First three checks for object passed, now see if any of the object's
          // corners are in our FOV
          // TODO: Avoid ProjectObject here because it also happens inside BroadcastObjectObservation
          f32 distance;
          std::vector<Point2f> projectedCorners;
          _robot->GetVisionComponent().GetCamera().ProjectObject(*unobservedObject, projectedCorners, distance);
          
          if(distance > 0.f) { // in front of camera?
            for(auto & corner : projectedCorners) {
              
              if(camera.IsWithinFieldOfView(corner))
              {
                using namespace ExternalInterface;
                const Rectangle<f32> boundingBox(projectedCorners);
                
                ObjectProjectsIntoFOV message(unobservedObject->GetLastObservedTime(),
                                              unobservedObject->GetFamily(),
                                              unobservedObject->GetType(),
                                              unobservedObject->GetID().GetValue(),
                                              CladRect(boundingBox.GetX(),
                                                       boundingBox.GetY(),
                                                       boundingBox.GetWidth(),
                                                       boundingBox.GetHeight()));
                
                _robot->Broadcast(MessageEngineToGame(std::move(message)));
                
                ++numVisibleObjects;
                
              } // if(IsWithinFieldOfView)
            } // for(each projectedCorner)
          } // if(distance > 0)
        }
      }
      
    } // for each unobserved object
    
    return numVisibleObjects;
  } // CheckForUnobservedObjects()
  
  void BlockWorld::RemoveUsedMarkers(std::list<Vision::ObservedMarker>& observedMarkers)
  {
    for(auto markerIter = observedMarkers.begin(); markerIter != observedMarkers.end();)
    {
      if (markerIter->IsUsed()) {
        markerIter = observedMarkers.erase(markerIter);
      } else {
        ++markerIter;
      }
    }
  }

  Result BlockWorld::AddMarkerlessObject(const Pose3d& p, ObjectType type)
  {
    TimeStamp_t lastTimestamp = _robot->GetLastMsgTimestamp();

    // Create an instance of the detected object
    auto markerlessObject = std::make_shared<MarkerlessObject>(type);
    markerlessObject->SetLastObservedTime(lastTimestamp);

    // Raise origin of object above ground.
    // NOTE: Assuming detected obstacle is at ground level no matter what angle the head is at.
    Pose3d raiseObject(0, Z_AXIS_3D(), Vec3f(0,0,0.5f*markerlessObject->GetSize().z()));
    Pose3d obsPose = p * raiseObject;
    obsPose.SetParent(_robot->GetPose().GetParent());
    
    // Initialize with Known pose so it won't delete immediately because it isn't re-seen
    markerlessObject->InitPose(obsPose, PoseState::Known);
    
    // Check if this prox obstacle already exists
    std::vector<ObservableObject*> existingObjects;
    auto originIter = _locatedObjects.find(_robot->GetWorldOrigin());
    if(originIter != _locatedObjects.end())
    {
      FindOverlappingObjects(markerlessObject.get(), originIter->second[ObjectFamily::MarkerlessObject], existingObjects);
    }
    
    // Update the last observed time of existing overlapping obstacles
    for(auto obj : existingObjects) {
      obj->SetLastObservedTime(lastTimestamp);
    }
    
    // No need to add the obstacle again if it already exists
    if (!existingObjects.empty()) {
      return RESULT_OK;
    }
    
    
    // Check if the obstacle intersects with any other existing objects in the scene.
    BlockWorldFilter filter;
    if(_robot->GetLocalizedTo().IsSet()) {
      // Ignore the mat object that the robot is localized to (?)
      filter.AddIgnoreID(_robot->GetLocalizedTo());
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
    _robotMsgTimeStampAtChange = lastTimestamp;
    
    // add cliffs to memory map, or other objects if feature is enabled
    if ( type == ObjectType::CliffDetection )
    {
      // cliffs currently have extra data (for directionality)
      const Pose3d& robotPose = _robot->GetPose();
      const Pose3d& robotPoseWrtOrigin = robotPose.GetWithRespectToOrigin();
      NavMemoryMapQuadData_Cliff cliffData;
      Vec3f rotatedFwdVector = robotPoseWrtOrigin.GetRotation() * X_AXIS_3D();
      cliffData.directionality = Vec2f{rotatedFwdVector.x(), rotatedFwdVector.y()};
      
      // calculate cliff quad where it's being placed (wrt origin since memory map is 2d wrt current origin)
      const Quad2f& cliffQuad = markerlessObject->GetBoundingQuadXY( p.GetWithRespectToOrigin() );
    
      INavMemoryMap* currentNavMemoryMap = GetNavMemoryMap();
      DEV_ASSERT(currentNavMemoryMap, "BlockWorld.OnRobotPoseChanged.NoMemoryMap");
      currentNavMemoryMap->AddQuad(cliffQuad, cliffData);
    }
    else if ( kAddUnrecognizedMarkerlessObjectsToMemMap )
    {
      // Add as obstacle in the memory map
      AddObjectReportToMemMap(*markerlessObject.get(), obsPose);
    }
    
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
    _robotMsgTimeStampAtChange = _robot->GetLastMsgTimestamp();
    
    return customObject->GetID();
  }

  bool BlockWorld::DidObjectsChange() const {
    return _didObjectsChange;
  }
  
  const TimeStamp_t& BlockWorld::GetTimeOfLastChange() const {
    return _robotMsgTimeStampAtChange;
  }

  Result BlockWorld::UpdateObjectPoses(std::list<Vision::ObservedMarker>& obsMarkers,
                                       const ObjectFamily& inFamily,
                                       const TimeStamp_t atTimestamp)
  {
    // Sanity checks for robot's origin
    DEV_ASSERT(_robot->GetPose().GetParent() == _robot->GetWorldOrigin(),
                 "BlockWorld.UpdateObjectPoses.RobotParentShouldBeOrigin");
    DEV_ASSERT(&_robot->GetPose().FindOrigin() == _robot->GetWorldOrigin(),
                 "BlockWorld.UpdateObjectPoses.BadRobotOrigin");
    
    const ObservableObjectLibrary& objectLibrary = _objectLibrary[inFamily];
    
    // Keep the objects sorted by increasing distance from the robot.
    // This will allow us to only use the closest object that can provide
    // localization information (if any) to update the robot's pose.
    // Note that we use a multimap to handle the corner case that there are two
    // objects that have the exact same distance. (We don't want to only report
    // seeing one of them and it doesn't matter which we use to localize.)
    std::multimap<f32, ObservableObject*> objectsSeen;
    
    // Don't bother with this update at all if we didn't see at least one
    // marker (which is our indication we got an update from the robot's
    // vision system
    if(!obsMarkers.empty())
    {
      // Extract only observed markers from obsMarkersAtTimestamp
      objectLibrary.CreateObjectsFromMarkers(obsMarkers, objectsSeen);
      
      // Remove used markers from list
      RemoveUsedMarkers(obsMarkers);
    
      // Use them to add or update existing blocks in our world
      Result lastResult = AddAndUpdateObjects(objectsSeen, inFamily, atTimestamp);
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_ERROR("BlockWorld.UpdateObjectPoses.AddAndUpdateFailed", "");
        return lastResult;
      }
    }
    
    return RESULT_OK;
    
  } // UpdateObjectPoses()

  /*
  Result BlockWorld::UpdateProxObstaclePoses()
  {
    TimeStamp_t lastTimestamp = _robot->GetLastMsgTimestamp();
    
    // Add prox obstacle if detected and one doesn't already exist
    for (ProxSensor_t sensor = (ProxSensor_t)(0); sensor < NUM_PROX; sensor = (ProxSensor_t)(sensor + 1)) {
      if (!_robot->IsProxSensorBlocked(sensor) && _robot->GetProxSensorVal(sensor) >= PROX_OBSTACLE_DETECT_THRESH) {
        
        // Create an instance of the detected object
        MarkerlessObject *m = new MarkerlessObject(ObjectType::ProxObstacle);
        
        // Get pose of detected object relative to robot according to which sensor it was detected by.
        Pose3d proxTransform = Robot::ProxDetectTransform[sensor];
        
        // Raise origin of object above ground.
        // NOTE: Assuming detected obstacle is at ground level no matter what angle the head is at.
        Pose3d raiseObject(0, Z_AXIS_3D(), Vec3f(0,0,0.5f*m->GetSize().z()));
        proxTransform = proxTransform * raiseObject;
        
        proxTransform.SetParent(_robot->GetPose().GetParent());
        
        // Compute pose of detected object
        Pose3d obsPose(_robot->GetPose());
        obsPose = obsPose * proxTransform;
        m->SetPose(obsPose);
        m->SetPoseParent(_robot->GetPose().GetParent());
        
        // Check if this prox obstacle already exists
        std::vector<ObservableObject*> existingObjects;
        FindOverlappingObjects(m, _locatedObjects[ObjectFamily::MarkerlessObject], existingObjects);
        
        // Update the last observed time of existing overlapping obstacles
        for(auto obj : existingObjects) {
          obj->SetLastObservedTime(lastTimestamp);
        }
        
        // No need to add the obstacle again if it already exists
        if (!existingObjects.empty()) {
          delete m;
          return RESULT_OK;
        }
        
        
        // Check if the obstacle intersects with any other existing objects in the scene.
        std::set<ObjectFamily> ignoreFamilies;
        std::set<ObjectType> ignoreTypes;
        std::set<ObjectID> ignoreIDs;
        if(_robot->IsLocalized()) {
          // Ignore the mat object that the robot is localized to (?)
          ignoreIDs.insert(_robot->GetLocalizedTo());
        }
        FindIntersectingObjects(m, existingObjects, 0, ignoreFamilies, ignoreTypes, ignoreIDs);
        if (!existingObjects.empty()) {
          delete m;
          return RESULT_OK;
        }

        
        m->SetLastObservedTime(lastTimestamp);
        AddNewObject(ObjectFamily::MarkerlessObject, m);
        _didObjectsChange = true;
        _robotMsgTimeStampAtChange = _robot->GetLastMsgTimestamp();
      }
    } // end for all prox sensors
    
    // Delete any existing prox objects that are too old.
    // Note that we use find() here because there may not be any markerless objects
    // yet, and using [] indexing will create things.
    auto markerlessFamily = _locatedObjects.find(ObjectFamily::MarkerlessObject);
    if(markerlessFamily != _locatedObjects.end())
    {
      auto proxTypeMap = markerlessFamily->second.find(ObjectType::ProxObstacle);
      if(proxTypeMap != markerlessFamily->second.end())
      {
        for (auto proxObsIter = proxTypeMap->second.begin();
             proxObsIter != proxTypeMap->second.end();
                 */
/* increment iter in loop, depending on erase*/    /*
)
        {
          if (lastTimestamp - proxObsIter->second->GetLastObservedTime() > PROX_OBSTACLE_LIFETIME_MS)
          {
            proxObsIter = ClearObject(proxObsIter, ObjectType::ProxObstacle,
                                      ObjectFamily::MarkerlessObject);
            
          } else {
            // Didn't erase anything, increment iterator
            ++proxObsIter;
          }
        }
      }
    }
    
    return RESULT_OK;
  }
  */


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ObjectID BlockWorld::AddConnectedActiveObject(ActiveID activeID, FactoryID factoryID, ActiveObjectType activeObjectType)
  {
    const ObjectType objType = ActiveObject::GetTypeFromActiveObjectType(activeObjectType);
    
    // only connected objects should be added through this method, so a required activeID is a must
    DEV_ASSERT(activeID != ObservableObject::InvalidActiveID, "BlockWorld.AddConnectedActiveObject.CantAddInvalidActiveID");
  
    if (activeID >= (int)ActiveObjectConstants::MAX_NUM_ACTIVE_OBJECTS) {
      PRINT_NAMED_WARNING("BlockWorld.AddConnectedActiveObject.InvalidActiveID", "activeID %d", activeID);
      return ObjectID();
    }

    // NOTE: If you hit any of the following VERIFY, please notify Raul and Al.
    // rsam: Al and I have made assumptions about when this gets called. Checking here that the assumptions are correct,
    // and if they are not, we need to re-evaluate this code, for example due to robot timing issues.
  
    // Validate that ActiveID is not currently a connected object. We assume that if the robot is reporting
    // an activeID, it should not be still used here (should have reported a disconnection)
    const ActiveObject* const conObjWithActiveID = GetConnectedActiveObjectByActiveID( activeID );
    ANKI_VERIFY( nullptr == conObjWithActiveID, "BlockWorld.AddConnectedActiveObject.ActiveIDAlreadyUsed", "%d", activeID );

    // Validate that factoryId is not currently a connected object. We assume that if the robot is reporting
    // a factoryID, the same object should not be in any current activeIDs.
    BlockWorldFilter filter;
    filter.SetFilterFcn([factoryID](const ObservableObject* object) {
      return object->GetFactoryID() == factoryID;
    });
    const ActiveObject* const conObjectWithFactoryID = FindConnectedObjectHelper(filter, nullptr, true);
    ANKI_VERIFY( nullptr == conObjectWithFactoryID, "BlockWorld.AddConnectedActiveObject.FactoryIDAlreadyUsed", "%u", factoryID );

    // This is the new object we are going to create. We can't insert it in _connectedObjects until
    // we know the objectID, so we create it first, and then we look for unconnected matches (we have seen the
    // object but we had not connected to it.) If we find one, we will inherit the objectID from that match; if
    // we don't find a match, we will assign it a new objectID. Then we can add to the container of _connectedObjects.
    ActiveObject* newActiveObjectPtr = CreateActiveObject(activeObjectType, activeID, factoryID);
    if ( nullptr == newActiveObjectPtr ) {
      // failed to create the object (that function should print the error, exit here with unSet ID)
      return ObjectID();
    }
    
    // we can't add to the _connectedObjects until the objectID has been decided
    
    // Is there an active object with the same activeID and type that already exists?
    BlockWorldFilter filterByActiveID;
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
                            "ActiveID %d (factoryID 0x%x) is same type as another existing object (objectID %d, activeID %d, factoryID 0x%x, type %s) updating ids to match",
                            activeID,
                            factoryID,
                            sameTypeObject->GetID().GetValue(),
                            sameTypeObject->GetActiveID(),
                            sameTypeObject->GetFactoryID(),
                            EnumToString(objType));
              
              // if we have a new factoryID, override the old instances with the new one we connected to
              if(factoryID > 0)
              {
                sameTypeObject->SetActiveID(activeID);
                sameTypeObject->SetFactoryID(factoryID);
              }
            } else {
              PRINT_CH_INFO("BlockWorld", "BlockWorld.AddConnectedActiveObject.FoundIdenticalObjectOnDifferentSlot",
                            "Updating activeID of block with factoryID 0x%x from %d to %d",
                            sameTypeObject->GetFactoryID(), sameTypeObject->GetActiveID(), activeID);
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
                      "objectID %d, activeID %d, type %s, factoryID 0x%x",
                      matchingObject->GetID().GetValue(), matchingObject->GetActiveID(), EnumToString(objType), matchingObject->GetFactoryID());
        
        // inherit objectID in the activeObject we created
        newActiveObjectPtr->CopyID(matchingObject);
      }
      else if (matchingObject->GetFactoryID() == 0)
      {
        // somehow (error) it never connected before (it shouldn't have had activeID)
        PRINT_NAMED_WARNING("BlockWorld.AddConnectedActiveObject.FoundMatchingActiveObjectThatWasNeverConnected",
                         "objectID %d, activeID %d, type %s, factoryID 0x%x",
                         matchingObject->GetID().GetValue(), matchingObject->GetActiveID(), EnumToString(objType), matchingObject->GetFactoryID());
        
        // if now we have a factoryID, go fix all located instances of the new object setting their factoryID
        if(factoryID > 0)
        {
          // Need to check existing objects in other frames and update to match new object
          std::vector<ObservableObject*> matchingObjectsInAllFrames;
          BlockWorldFilter filterInAnyByID;
          filterInAnyByID.AddFilterFcn([activeID](const ObservableObject* object) { return object->GetActiveID() == activeID;} );
          filterInAnyByID.SetAllowedTypes({objType});
          filterInAnyByID.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
          FindLocatedMatchingObjects(filterInAnyByID, matchingObjectsInAllFrames);
        
          PRINT_CH_INFO("BlockWorld", "BlockWorld.AddConnectedActiveObject.UpdateExistingObjectsFactoryID",
                           "Updating %zu existing objects in other frames to match object with factoryID 0x%x and activeID %d",
                           matchingObjectsInAllFrames.size(),
                           factoryID,
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
                          "objectID %d, activeID %d, type %s, factoryID 0x%x (expected 0x%x)",
                          matchingObject->GetID().GetValue(), matchingObject->GetActiveID(), EnumToString(objType), factoryID, matchingObject->GetFactoryID());
        DeleteLocatedObjectByIDInCurOrigin(matchingObject->GetID());
        
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
    
    // TODO: consider a new poseState here as "it was previously useful, but we can't trust the pose anymore since
    // the object has disconnected and we may miss moved messages".
    // For now, the old code used to flag as unknown, which in the new code equals to removing the objects
    if ( removedObjectID.IsSet() ) {
      DeleteLocatedObjectsByID(removedObjectID);
    }
    
    // return the objectID
    return removedObjectID;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ActiveObject* BlockWorld::CreateActiveObject(ActiveObjectType activeObjectType, ActiveID activeID, FactoryID factoryID)
  {
    ActiveObject* retPtr = nullptr;
    const ObjectType objType = ActiveObject::GetTypeFromActiveObjectType(activeObjectType);
    switch(objType) {
      case ObjectType::Block_LIGHTCUBE1:
      case ObjectType::Block_LIGHTCUBE2:
      case ObjectType::Block_LIGHTCUBE3:
      {
        retPtr = new ActiveCube(activeID, factoryID, activeObjectType);
        break;
      }
      case ObjectType::Charger_Basic:
      {
        retPtr = new Charger(activeID, factoryID, activeObjectType);
        break;
      }
      default:
      {
        PRINT_NAMED_ERROR("BlockWorld.AddConnectedActiveObject.UnsupportedActiveObjectType",
                           "%s (ActiveObjectType: 0x%hx)", EnumToString(objType), activeObjectType);
      }
    }
    return retPtr;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::AddLocatedObject(const std::shared_ptr<ObservableObject>& object)
  {
    DEV_ASSERT(object->HasValidPose(), "BlockWorld.AddLocatedObject.NotAValidPoseState");
    DEV_ASSERT(object->GetID().IsSet(), "BlockWorld.AddLocatedObject.ObjectIDNotSet");

    const PoseOrigin* objectOrigin = &object->GetPose().FindOrigin();
    
    // allow adding only in current origin
    DEV_ASSERT(objectOrigin == _robot->GetWorldOrigin(), "BlockWorld.AddLocatedObject.NotCurrentOrigin");
    
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

    // grab the current pointer and check it's empty (do not expect overwriting)
    std::shared_ptr<ObservableObject>& objectLocation =
      _locatedObjects[objectOrigin][object->GetFamily()][object->GetType()][object->GetID()];
    DEV_ASSERT(objectLocation == nullptr, "BlockWorld.AddLocatedObject.ObjectIDInUseInOrigin");
    objectLocation = object; // store the new object, this increments refcount

    // set the viz manager on this new object
    object->SetVizManager(_robot->GetContext()->GetVizManager());
    
    PRINT_CH_INFO("BlockWorld", "BlockWorld.AddLocatedObject",
                  "Adding new %s%s object and ID=%d ActID=%d FacID=0x%x at (%.1f, %.1f, %.1f), in frame %s.",
                  object->IsActive() ? "active " : "",
                  EnumToString(object->GetType()),
                  object->GetID().GetValue(),
                  object->GetActiveID(),
                  object->GetFactoryID(),
                  object->GetPose().GetTranslation().x(),
                  object->GetPose().GetTranslation().y(),
                  object->GetPose().GetTranslation().z(),
                  object->GetPose().FindOrigin().GetName().c_str());
    
    // fire event to represent "first time an object has been seen in this origin"
    Util::sEventF("robot.object_located", {}, "%s", EnumToString(object->GetType()));

    // make sure that everyone gets notified that there's a new object in town, I mean in this origin
    {
      const ObjectID& objectID = object->GetID();
      const Pose3d* newPosePtr = &object->GetPose();
      const Pose3d* oldPosePtr = nullptr;
      const PoseState newPoseState = object->GetPoseState();
      const PoseState oldPoseState = PoseState::Invalid;
      const bool isActive = object->IsActive();
      const ObjectFamily family = object->GetFamily();
      _robot->GetObjectPoseConfirmer().BroadcastObjectPoseChanged(objectID, isActive, family, oldPosePtr, oldPoseState, newPosePtr, newPoseState);
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::OnObjectPoseChanged(const ObjectID& objectID,
                                       const ObjectFamily family,
                                       const Pose3d* oldPose, PoseState oldPoseState,
                                       const Pose3d* newPose, PoseState newPoseState)
  {
    // Sanity checks
    #if ANKI_DEVELOPER_CODE
    {
      DEV_ASSERT(objectID.IsSet(), "BlockWorld.OnObjectPoseChanged.InvalidObjectID");
      // Check: PoseState=Invalid <-> Pose=nullptr
      const bool oldStateInvalid = !ObservableObject::IsValidPoseState(oldPoseState);
      const bool oldPoseNull     = (nullptr == oldPose);
      DEV_ASSERT(oldStateInvalid == oldPoseNull, "BlockWorld.OnObjectPoseChanged.InvalidOldPoseParameters");
      const bool newStateInvalid = !ObservableObject::IsValidPoseState(newPoseState);
      const bool newPoseNull     = (nullptr == newPose);
      DEV_ASSERT(newStateInvalid == newPoseNull, "BlockWorld.OnObjectPoseChanged.InvalidNewPoseParameters");
      // Check: Can't set from and to Invalid
      DEV_ASSERT(newStateInvalid != oldStateInvalid, "BlockWorld.OnObjectPoseChanged.BothStatesAreInvalid");
      // Check: newPose valid/invalid correlates with the object instance in the world (if invalid, null object),
      const ObservableObject* locatedObject = GetLocatedObjectByID(objectID);
      const bool isObjectNull = (nullptr == locatedObject);
      DEV_ASSERT(newStateInvalid == isObjectNull, "BlockWorld.OnObjectPoseChanged.PoseStateAndObjectDontMatch");
    }
    #endif

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

    // - - - - -
    // update memory map
    // - - - - -
	const bool objectTrackedInMemoryMap = (family != ObjectFamily::CustomObject || kAddCustomObjectsToMemMap); // COZMO-9360
	if( objectTrackedInMemoryMap ) 
    {	
    /* 
      Three things can happen:
       a) first time we see an object: OldPoseState=!Valid, NewPoseState= Valid
       b) updating an object:          OldPoseState= Valid, NewPoseState= Valid
       c) deleting an object:          OldPoseState= Valid, NewPoseState=!Valid
     */
    const bool oldValid = ObservableObject::IsValidPoseState(oldPoseState);
    const bool newValid = ObservableObject::IsValidPoseState(newPoseState);
    if ( !oldValid && newValid )
    {
      // first time we see the object, add report
      const ObservableObject* object = GetLocatedObjectByID(objectID);
      AddObjectReportToMemMap(*object, *newPose);
    }
    else if ( oldValid && newValid )
    {
      // updating the pose of an object, decide if we update the report. As an optimization, we don't update
      // it if the poses are close enough
      const ObservableObject* object = GetLocatedObjectByID(objectID); // can't return null, asserted
      const int objectIdInt = objectID.GetValue();
      OriginToPoseInMapInfo& reportedPosesForObject = _navMapReportedPoses[objectIdInt];
      const PoseOrigin* curOrigin = &newPose->FindOrigin();
      auto poseInNewOriginIter = reportedPosesForObject.find( curOrigin );
      if ( poseInNewOriginIter != reportedPosesForObject.end() )
      {
        // note that for distThreshold, since Z affects whether we add to the memory map, distThreshold should
        // be smaller than the threshold to not report
        DEV_ASSERT(kObjectPositionChangeToReport_mm < object->GetDimInParentFrame<'Z'>()*0.5f,
                  "OnObjectPoseChanged.ChangeThresholdTooBig");
        const float distThreshold = kObjectPositionChangeToReport_mm;
        const Radians angleThreshold( DEG_TO_RAD(kObjectRotationChangeToReport_deg) );

        // compare new pose with previous entry and decide if isFarFromPrev
        const PoseInMapInfo& info = poseInNewOriginIter->second;
        const bool isFarFromPrev =
          ( !info.isInMap || (!newPose->IsSameAs(info.pose, Point3f(distThreshold), angleThreshold)));
        
        // if it is far from previous (or previous was not in the map, remove-add)
        if ( isFarFromPrev ) {
          RemoveObjectReportFromMemMap(*object, curOrigin);
          AddObjectReportToMemMap(*object, *newPose);
        }
      }
      else
      {
        // did not find an entry in the current origin for this object, add it now
        const ObservableObject* object = GetLocatedObjectByID(objectID);
        AddObjectReportToMemMap(*object, *newPose);
      }
    }
    else if ( oldValid && !newValid )
    {
      // deleting an object, remove its report using oldOrigin (the origin it was removed from)
      const PoseOrigin* oldOrigin = &oldPose->FindOrigin();
      const ObservableObject* object = GetLocatedObjectByID(objectID);
      RemoveObjectReportFromMemMap(*object, oldOrigin);
    }
    else
    {
      // not possible
      PRINT_NAMED_ERROR("BlockWorld.OnObjectPoseChanged.BothStatesAreInvalid",
                        "Object %d changing from Invalid to Invalid", objectID.GetValue());
    }
	}
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::OnObjectVisuallyVerified(const ObservableObject* object)
  {
      // -- clear memory map from robot to markers
      ClearRobotToMarkersInMemMap(object);
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::AddObjectReportToMemMap(const ObservableObject& object, const Pose3d& newPose)
  {
    const int objectId = object.GetID().GetValue();
    const ObjectFamily objectFam = object.GetFamily();
    const NavMemoryMapTypes::EContentType addType = ObjectFamilyToMemoryMapContentType(objectFam, true);
    if ( addType == NavMemoryMapTypes::EContentType::Unknown )
    {
      // this is ok, this obstacle family is not tracked in the memory map
      PRINT_CH_INFO("BlockWorld", "BlockWorld.AddObjectReportToMemMap.InvalidAddType",
                    "Family '%s' is not known in memory map",
                    ObjectFamilyToString(objectFam) );
      return;
    }
    
    // find the memory map for the given origin
    const PoseOrigin* origin = &newPose.FindOrigin();
    auto matchPair = _navMemoryMaps.find(origin);
    if ( matchPair != _navMemoryMaps.end() )
    {
      // in order to properly handle stacks, do not add the quad to the memory map for objects that are not
      // on the floor
      Pose3d objWrtRobot;
      if ( newPose.GetWithRespectTo(_robot->GetPose(), objWrtRobot) )
      {
        INavMemoryMap* memoryMap = matchPair->second.get();
        
        const bool isFloating = object.IsPoseTooHigh(objWrtRobot, 1.f, STACKED_HEIGHT_TOL_MM, 0.f);
        if ( isFloating )
        {
          // store in as a reported pose, but set as not in map (the pose value is not relevant)
          _navMapReportedPoses[objectId][origin] = PoseInMapInfo(newPose, false);
        }
        else
        {
          // add to memory map flattened out wrt origin
          Pose3d newPoseWrtOrigin = newPose.GetWithRespectToOrigin();
          const Quad2f& newQuad = object.GetBoundingQuadXY(newPoseWrtOrigin);
          memoryMap->AddQuad(newQuad, addType);
          
          // store in as a reported pose
          _navMapReportedPoses[objectId][origin] = PoseInMapInfo(newPoseWrtOrigin, true);
          
          // since we added an obstacle, any borders we saw while dropping it should not be interesting
          const float kScaledQuadToIncludeEdges = 2.0f;
          // kScaledQuadToIncludeEdges: we want to consider interesting edges around this obstacle as non-interesting,
          // since we know they belong to this object. The quad to search for these edges has to be equal to the
          // obstacle quad plus the margin in which we would find edges. For example, a good tight limit would be the size
          // of the smallest quad in the memory map, since edges should be adjacent to the cube. This quad however is merely
          // to limit the search for interesting edges, so it being bigger than the tightest threshold should not
          // incur in a big penalty hit
          const Quad2f& edgeQuad = newQuad.GetScaled(kScaledQuadToIncludeEdges);
          ReviewInterestingEdges(edgeQuad);
        }
      }
      else
      {
        // should not happen, so warn about it
        PRINT_NAMED_WARNING("BlockWorld.AddObjectReportToMemMap.InvalidPose",
                            "Could not get object's new pose wrt robot. Won't add to map");
      }
    }
    else
    {
      // if the map was removed (for zombies), we shouldn't be asking to add an object to it
      DEV_ASSERT(matchPair == _navMemoryMaps.end(), "BlockWorld.AddObjectReportToMemMap.NoMapForOrigin");
    }
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::RemoveObjectReportFromMemMap(const ObservableObject& object, const Pose3d* origin)
  {
    const int objectId = object.GetID().GetValue();
    const ObjectFamily objectFam = object.GetFamily();
    const NavMemoryMapTypes::EContentType removalType = ObjectFamilyToMemoryMapContentType(objectFam, false);
    if ( removalType == NavMemoryMapTypes::EContentType::Unknown )
    {
      // this is not ok, this obstacle family can be added but can't be removed from the map
      PRINT_NAMED_WARNING("BlockWorld.RemoveObjectReportFromMemMap.InvalidRemovalType",
                          "Family '%s' does not have a removal type in memory map",
                          ObjectFamilyToString(objectFam) );
      return;
    }
    
    // find origins for the given object
    const ObjectIdToPosesPerOrigin::iterator originsForObjectIt = _navMapReportedPoses.find(objectId);
    if ( originsForObjectIt != _navMapReportedPoses.end() )
    {
      OriginToPoseInMapInfo& infosPerOrigin = originsForObjectIt->second;
      const OriginToPoseInMapInfo::iterator infosForOriginIt = infosPerOrigin.find(origin);
      if ( infosForOriginIt != infosPerOrigin.end() )
      {
        PoseInMapInfo& info = infosForOriginIt->second;
        
        // if it's already not in the map do nothing
        if ( info.isInMap )
        {
          // pose should be correct if it's in map (could be from an old origin if it's not in map)
          DEV_ASSERT(infosForOriginIt->second.pose.GetParent() == origin,
                     "BlockWorld.RemoveObjectReportFromMemMap.PoseNotFlattenedOut");
          
          // find the memory map for the given origin
          auto matchPair = _navMemoryMaps.find(origin);
          if ( matchPair != _navMemoryMaps.end() )
          {
            INavMemoryMap* memoryMap = matchPair->second.get();

            // remove from the memory map
            const Pose3d& oldPoseInThisOrigin = info.pose;
            const Quad2f& newQuad = object.GetBoundingQuadXY(oldPoseInThisOrigin);
            memoryMap->AddQuad(newQuad, removalType);
          }
          else
          {
            // if the map was removed (for zombies), we shouldn't be asking to remove an object from it
            DEV_ASSERT(matchPair == _navMemoryMaps.end(), "BlockWorld.RemoveObjectReportFromMemMap.NoMapForOrigin");
          }
        
          // flag as not in map anymore
          // we do not want to remove the entry in case we rejigger origins. By setting this flag we are saying
          // "we had a pose here, but has become unknown", rather than "we don't have a pose" if we called erase
          info.isInMap = false;
        }
      }
    }
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::UpdateOriginsOfObjectsReportedInMemMap(const Pose3d* curOrigin, const Pose3d* relocalizedOrigin)
  {
    // for every object in the current map, we have a decision to make. We are going to bring that memory map
    // into what is becoming the current one. That means also bringing the last reported pose of every object
    // onto the new map. The current map is obviously more up to date than the map we merge into, since the map
    // we merge into is map we identified a while ago. This means that if an object moved and we now know where
    // it is, the good pose is in the currentMap, not in the mapWeMergeInto. So, for every object in the currentMap
    // we are going to remove their pose from the mapWeMergeInto. This will make the map we merge into gain the new
    // info, at the same time that we remove info known to not be the most accurate
    
    // for every object in the current origin
    for ( auto& pairIdToPoseInfoByOrigin : _navMapReportedPoses )
    {
      // find object in the world
      const ObservableObject* object = GetLocatedObjectByID(pairIdToPoseInfoByOrigin.first);
      if ( nullptr == object )
      {
        PRINT_CH_INFO("BlockWorld", "BlockWorld.UpdateObjectsReportedInMepMap.NotAnObject",
                      "Could not find object ID '%d' in BlockWorld updating their quads", pairIdToPoseInfoByOrigin.first );
        continue;
      }
    
      // find the reported pose for this object in the current origin
      OriginToPoseInMapInfo& poseInfoByOriginForObj = pairIdToPoseInfoByOrigin.second;
      const auto& matchInCurOrigin = poseInfoByOriginForObj.find(curOrigin);
      const bool isObjectReportedInCurrent = (matchInCurOrigin != poseInfoByOriginForObj.end());
      if ( isObjectReportedInCurrent )
      {
        // we have an entry in the current origin. We don't care if `isInMap` is true or false. If it's true
        // it means we have a better position available in this frame, if it's false it means we saw the object
        // in this frame, but somehow it became unknown. If it became unknown, the position it had in the origin
        // we are relocalizing to is old and not to be trusted. This is the reason why we don't erase reported poses,
        // but rather flag them as !isInMap.
        // Additionally we don't have to worry about the container we are iterating changing, since iterators are not
        // affected by changing a boolean, but are if we erased from it.
        RemoveObjectReportFromMemMap(*object, relocalizedOrigin);
        
        // we are bringing over the current info into the relocalized origin, update the reported pose in the
        // relocalized origin to be that of the newest information
        poseInfoByOriginForObj[relocalizedOrigin].isInMap = matchInCurOrigin->second.isInMap;
        if ( matchInCurOrigin->second.isInMap ) {
          // bring over the pose if it's in map (otherwise we don't care about the pose)
          // when we bring it, flatten out to the relocalized origin
          DEV_ASSERT(relocalizedOrigin == &matchInCurOrigin->second.pose.FindOrigin(),
                     "BlockWorld.UpdateObjectsReportedInMepMap.PoseDidNotHookGranpa");
          poseInfoByOriginForObj[relocalizedOrigin].pose = matchInCurOrigin->second.pose.GetWithRespectToOrigin();
        }
        // also, erase the current origin from the reported poses of this object, since we will never use it after this
        // Note this should not alter the iterators we are using for _navMapReportedPoses
        poseInfoByOriginForObj.erase(curOrigin);
      }
      else
      {
        // we don't have this object in the current memory map. The info from this object if at all is in the previous
        // origin (then one we are relocalizing to), or another origin not related to these two, do nothing in those
        // cases
      }
    }
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::ClearRobotToMarkersInMemMap(const ObservableObject* object)
  {
    // the newPose should be directly in the robot's origin
    DEV_ASSERT(object->GetPose().GetParent() == _robot->GetWorldOrigin(),
               "BlockWorld.ClearRobotToMarkersInMemMap.ObservedObjectParentNotRobotOrigin");

    INavMemoryMap* currentNavMemoryMap = GetNavMemoryMap();
    
    // we are creating a quad projected on the ground from the robot to every marker we see. In order to do so
    // the bottom corners of the ground quad match the forward ones of the robot (robotQuad::xLeft). The names
    // cornerBR, cornerBL are the corners in the ground quads (BottomRight and BottomLeft).
    // Later on we will generate the top corners for the ground quad (cornerTL, corner TR)
    const Quad2f& robotQuad = _robot->GetBoundingQuadXY(_robot->GetPose().GetWithRespectToOrigin());
    Point3f cornerBR{ robotQuad[Quad::TopLeft   ].x(), robotQuad[Quad::TopLeft ].y(), 0};
    Point3f cornerBL{ robotQuad[Quad::BottomLeft].x(), robotQuad[Quad::BottomLeft].y(), 0};
  
    // get the markers we have seen from this object
    std::vector<const Vision::KnownMarker*> observedMarkers;
    object->GetObservedMarkers(observedMarkers);
  
    for ( const auto& observedMarkerIt : observedMarkers )
    {
      // An observed marker. Assign the marker's bottom corners as the top corners for the ground quad
      // The names of the corners (cornerTL and cornerTR) are those of the ground quad: TopLeft and TopRight
      DEV_ASSERT(&observedMarkerIt->GetPose().FindOrigin() == _robot->GetWorldOrigin(),
                 "BlockWorld.ClearVisionFromRobotToMarkers.MarkerOriginShouldBeRobotOrigin");
      
      const Quad3f& markerCorners = observedMarkerIt->Get3dCorners(observedMarkerIt->GetPose().GetWithRespectToOrigin());
      Point3f cornerTL = markerCorners[Quad::BottomLeft];
      Point3f cornerTR = markerCorners[Quad::BottomRight];
      
      // Create a quad between the bottom corners of a marker and the robot forward corners, and tell
      // the navmesh that it should be clear, since we saw the marker
      Quad2f clearVisionQuad { cornerTL, cornerBL, cornerTR, cornerBR };
      
      // update navmesh with a quadrilateral between the robot and the seen object
      currentNavMemoryMap->AddQuad(clearVisionQuad, INavMemoryMap::EContentType::ClearOfObstacle);
      
      // also notify behavior whiteboard.
      // rsam: should this information be in the map instead of the whiteboard? It seems a stretch that
      // blockworld knows now about behaviors, maybe all this processing of quads should be done in a separate
      // robot component, like a VisualInformationProcessingComponent
      _robot->GetAIComponent().GetWhiteboard().ProcessClearQuad(clearVisionQuad);
    }
  }
  
  
  void BlockWorld::OnRobotDelocalized(const Pose3d* newWorldOrigin)
  {
    // delete objects that have become useless since we delocalized last time
    DeleteObjectsFromZombieOrigins();
    
    // create a new memory map for this origin
    CreateLocalizedMemoryMap(newWorldOrigin);
    
    // deselect blockworld's selected object, if it has one
    DeselectCurrentObject();
    
    // notify about updated object states
    BroadcastLocatedObjectStates();
  }
  Result BlockWorld::AddCliff(const Pose3d& p)
  {
    // at the moment we treat them as markerless objects
    const Result ret = AddMarkerlessObject(p, ObjectType::CliffDetection);
    return ret;
  }

  Result BlockWorld::AddProxObstacle(const Pose3d& p)
  {
    // add markerless object
    const Result ret = AddMarkerlessObject(p, ObjectType::ProxObstacle);
    return ret;
  }
  
  Result BlockWorld::AddCollisionObstacle(const Pose3d& p)
  {
    const Result ret = AddMarkerlessObject(p, ObjectType::CollisionObstacle);
    return ret;
  }
  
  

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result BlockWorld::ProcessVisionOverheadEdges(const OverheadEdgeFrame& frameInfo)
  {
    Result ret = RESULT_OK;
    if ( frameInfo.groundPlaneValid ) {
      if ( !frameInfo.chains.empty() ) {
        ret = AddVisionOverheadEdges(frameInfo);
      } else {
        // we expect lack of borders to be reported as !isBorder chains
        DEV_ASSERT(false, "ProcessVisionOverheadEdges.ValidPlaneWithNoChains");
      }
    } else {
      // ground plane was invalid (atm we don't use this). It's probably only useful if we are debug-rendering
      // the ground plane
      _robot->GetContext()->GetVizManager()->EraseSegments("BlockWorld.AddVisionOverheadEdges");
    }
    return ret;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  namespace {

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    inline Point3f EdgePointToPoint3f(const OverheadEdgePoint& point, const Pose3d& pose, float z=0.0f) {
      Point3f ret = pose * Point3f(point.position.x(), point.position.y(), z);
      return ret;
    }
    
  } // anonymous namespace

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::ReviewInterestingEdges(const Quad2f& withinQuad)
  {
    // Note1: Not using withinQuad, but should. I plan on using once the memory map allows local queries and
    // modifications. Leave here for legacy purposes. We surely enable it soon, because performance needs
    // improvement.
    // Note2: Actually FindBorder is very fast compared to having to check each node against the quad, depending
    // on how many nodes of each type there are (interesting vs quads within 'withinQuad'), so it can potentially
    // be faster depending on the case. Unless profiling shows up for this, no need to listen to Note1

    // check if merge is enabled
    if ( !kReviewInterestingEdges ) {
      return;
    }
    
    // ask the memory map to do the merge
    // some implementations make require parameters like max distance to merge, but for now trust continuity
    INavMemoryMap* currentNavMemoryMap = GetNavMemoryMap();
    if( currentNavMemoryMap )
    {
      using ContentType = INavMemoryMap::EContentType;
      
      // interesting edges adjacent to any of these types will be deemed not interesting
      constexpr NavMemoryMapTypes::FullContentArray typesWhoseEdgesAreNotInteresting =
      {
        {NavMemoryMapTypes::EContentType::Unknown               , false},
        {NavMemoryMapTypes::EContentType::ClearOfObstacle       , false},
        {NavMemoryMapTypes::EContentType::ClearOfCliff          , false},
        {NavMemoryMapTypes::EContentType::ObstacleCube          , true },
        {NavMemoryMapTypes::EContentType::ObstacleCubeRemoved   , false},
        {NavMemoryMapTypes::EContentType::ObstacleCharger       , true },
        {NavMemoryMapTypes::EContentType::ObstacleChargerRemoved, true },
        {NavMemoryMapTypes::EContentType::ObstacleUnrecognized  , true },
        {NavMemoryMapTypes::EContentType::Cliff                 , false},
        {NavMemoryMapTypes::EContentType::InterestingEdge       , false},
        {NavMemoryMapTypes::EContentType::NotInterestingEdge    , true }
      };
      static_assert(NavMemoryMapTypes::IsSequentialArray(typesWhoseEdgesAreNotInteresting),
        "This array does not define all types once and only once.");

      // fill border in memory map
      currentNavMemoryMap->FillBorder(ContentType::InterestingEdge, typesWhoseEdgesAreNotInteresting, ContentType::NotInterestingEdge);
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result BlockWorld::AddVisionOverheadEdges(const OverheadEdgeFrame& frameInfo)
  {
    ANKI_CPU_PROFILE("BlockWorld::AddVisionOverheadEdges");
    _robot->GetContext()->GetVizManager()->EraseSegments("BlockWorld.AddVisionOverheadEdges");
    
    // check conditions to add edges
    DEV_ASSERT(!frameInfo.chains.empty(), "AddVisionOverheadEdges.NoEdges");
    DEV_ASSERT(frameInfo.groundPlaneValid, "AddVisionOverheadEdges.InvalidGroundPlane");
    
    // we are only processing edges for the memory map, so if there's no map, don't do anything
    INavMemoryMap* currentNavMemoryMap = GetNavMemoryMap();
    if( nullptr == currentNavMemoryMap && !kDebugRenderOverheadEdges )
    {
      return RESULT_OK;
    }
    const float kDebugRenderOverheadEdgesZ_mm = 31.0f;

    // grab the robot pose at the timestamp of this frame
    TimeStamp_t t;
    RobotPoseStamp* p = nullptr;
    HistPoseKey poseKey;
    const Result poseRet = _robot->GetPoseHistory()->ComputeAndInsertPoseAt(frameInfo.timestamp, t, &p, &poseKey, true);
    if(RESULT_FAIL_ORIGIN_MISMATCH == poseRet)
    {
      // "Failing" because of an origin mismatch is OK, so don't freak out, but don't
      // go any further either.
      return RESULT_OK;
    }
    
    const bool poseIsGood = ( RESULT_OK == poseRet ) && (p != nullptr);
    if ( !poseIsGood ) {
      // this can happen if robot status messages are lost
      PRINT_CH_INFO("BlockWorld", "BlockWorld.AddVisionOverheadEdges.HistoricalPoseNotFound",
                    "Pose not found for timestamp %u (hist: %u to %u). Edges ignored for this timestamp.",
                    frameInfo.timestamp,
                    _robot->GetPoseHistory()->GetOldestTimeStamp(),
                    _robot->GetPoseHistory()->GetNewestTimeStamp());
      return RESULT_OK;
    }
    
    // If we can't transfor the observedPose to the current origin, it's ok, that means that the timestamp
    // for the edges we just received is from before delocalizing, so we should discard it.
    Pose3d observedPose;
    if ( !p->GetPose().GetWithRespectTo( *_robot->GetWorldOrigin(), observedPose) ) {
      PRINT_CH_INFO("BlockWorld", "BlockWorld.AddVisionOverheadEdges.NotInThisWorld",
                    "Received timestamp %d, but could not translate that timestamp into current origin.", frameInfo.timestamp);
      return RESULT_OK;
    }
    
    const Point3f& cameraOrigin = observedPose.GetTranslation();
    
    // Ideally we would do clamping with quad in robot coordinates, but this is an optimization to prevent
    // having to transform segments twice. We transform the segments to world space so that we can
    // calculate variations in angles, in order to merge together small variations. Once we have transformed
    // the segments, we can clamp the merged segments. We could do this in 2D, but we would have to transform
    // those segments again into world space. As a minor optimization, transform ground-plane's near-plane instead
    Point2f nearPlaneLeft  = observedPose *
      Point3f(frameInfo.groundplane[Quad::BottomLeft].x(),
              frameInfo.groundplane[Quad::BottomLeft].y(),
              0.0f);
    Point2f nearPlaneRight = observedPose *
      Point3f(frameInfo.groundplane[Quad::BottomRight].x(),
              frameInfo.groundplane[Quad::BottomRight].y(),
              0.0f);

    // quads that are clear, either because there are not borders behind them or from the camera to that border
    std::vector<Quad2f> visionQuadsClear;
   
    // detected borders are simply lines
    struct Segment {
      Segment(const Point2f& f, const Point2f& t) : from(f), to(t) {}
      Point2f from;
      Point2f to;
    };
    std::vector<Segment> visionSegmentsWithInterestingBorders;
    
    // we store the closest detected edge every time in the whiteboard
    float closestPointDist_mm2 = std::numeric_limits<float>::quiet_NaN();
    
    // iterate every chain finding contiguous segments
    for( const auto& chain : frameInfo.chains )
    {

      // debug render
      if ( kDebugRenderOverheadEdges )
      {
        // renders every segment reported by vision
        for (size_t i=0; i<chain.points.size()-1; ++i)
        {
          Point3f start = EdgePointToPoint3f(chain.points[i], observedPose, kDebugRenderOverheadEdgesZ_mm);
          Point3f end   = EdgePointToPoint3f(chain.points[i+1], observedPose, kDebugRenderOverheadEdgesZ_mm);
          ColorRGBA color = ((i%2) == 0) ? NamedColors::YELLOW : NamedColors::ORANGE;
          VizManager* vizManager = _robot->GetContext()->GetVizManager();
          vizManager->DrawSegment("BlockWorld.AddVisionOverheadEdges", start, end, color, false);
        }
      }
      else if ( kDebugRenderOverheadEdgeBorderQuads || kDebugRenderOverheadEdgeClearQuads )
      {
        Point2f wrtOrigin2DTL = observedPose * Point3f(frameInfo.groundplane[Quad::TopLeft].x(),
                                                       frameInfo.groundplane[Quad::TopLeft].y(),
                                                       0.0f);
        Point2f wrtOrigin2DBL = observedPose * Point3f(frameInfo.groundplane[Quad::BottomLeft].x(),
                                                       frameInfo.groundplane[Quad::BottomLeft].y(),
                                                       0.0f);
        Point2f wrtOrigin2DTR = observedPose * Point3f(frameInfo.groundplane[Quad::TopRight].x(),
                                                       frameInfo.groundplane[Quad::TopRight].y(),
                                                       0.0f);
        Point2f wrtOrigin2DBR = observedPose * Point3f(frameInfo.groundplane[Quad::BottomRight].x(),
                                                       frameInfo.groundplane[Quad::BottomRight].y(),
                                                       0.0f);
        
        Quad2f groundPlaneWRTOrigin(wrtOrigin2DTL, wrtOrigin2DBL, wrtOrigin2DTR, wrtOrigin2DBR);
        VizManager* vizManager = _robot->GetContext()->GetVizManager();
        vizManager->DrawQuadAsSegments("BlockWorld.AddVisionOverheadEdges", groundPlaneWRTOrigin, kDebugRenderOverheadEdgesZ_mm, NamedColors::BLACK, false);
      }

      DEV_ASSERT(chain.points.size() > 2,"AddVisionOverheadEdges.ChainWithTooLittlePoints");
      
      // when we are processing a non-edge chain, points can be discarded. Variables to track segments
      bool hasValidSegmentStart = false;
      Point3f segmentStart;
      bool hasValidSegmentEnd   = false;
      Point3f segmentEnd;
      Vec3f segmentNormal;
      size_t curPointInChainIdx = 0;
      do
      {
        // get candidate point to merge into previous segment
        Point3f candidate3D = EdgePointToPoint3f(chain.points[curPointInChainIdx], observedPose);
        
        // this is to prevent vision clear quads that cross an object from clearing that object. This could be
        // optimized by passing in a flag to AddQuad that tells the QuadTree that it should not override
        // these types. However, if we have not seen an edge, and we crossed an object, it can potentially clear
        // behind that object, which is equaly wrong. Ideally, HasCollisionWithRay would provide the closest
        // collision point to "from", so that we can clear up to that point, and discard any information after.
        // Consider that in the future if performance-wise it's ok top have these checks here, and the memory
        // map can efficiently figure out the order in which to check for collision (there's a fast check
        // I can think of that involves simply knowing from which quadrant the ray starts)
        
        bool occludedBeforeNearPlane = false;
        bool occludedInsideROI = false;
        Vec2f innerRayFrom = cameraOrigin; // it will be updated with the intersection with the near plane (if found)

        // - calculate occlusion between camera and near plane of ROI
        {
          // check if we cross something between the camera and the near plane (outside of ROI plane)
          // from camera to candidate
          kmRay2 fullRay;
          kmVec2 kmFrom{cameraOrigin.x(), cameraOrigin.y()};
          kmVec2 kmTo  {candidate3D.x() , candidate3D.y() };
          kmRay2FillWithEndpoints(&fullRay, &kmFrom, &kmTo);
          
          // near plane segment
          kmRay2 nearPlaneSegment;
          kmVec2 kmNearL{nearPlaneLeft.x() , nearPlaneLeft.y() };
          kmVec2 kmNearR{nearPlaneRight.x(), nearPlaneRight.y()};
          kmRay2FillWithEndpoints(&nearPlaneSegment, &kmNearL, &kmNearR);
          
          // find the intersection between the two
          kmVec2 rayAtNearPlane;
          const kmBool foundNearPlane = kmSegment2WithSegmentIntersection(&fullRay, &nearPlaneSegment , &rayAtNearPlane);
          if ( foundNearPlane )
          {
            /*
              Note on occludedBeforeNearPlane vs occludedInsideROI:
              We want to check two different zones: one from cameraOrigin to nearPlane and another from nearPlane to candidate3D. 
              The first one, being out of the current ground ROI can be more restrictive (fail on borders), since we
              literally have no information to back up a ClearOfObstacle, however the second one can't fail on borders,
              since borders are exactly what we are detecting, so the point can't become invalid when a border is detected.
              That should be the main difference between typesThatOccludeValidInfoOutOfROI vs typesThatOccludeValidInfoInsideROI.
            */
            // typesThatOccludeValidInfoOutOfROI = types to check against outside ground ROI:
            constexpr NavMemoryMapTypes::FullContentArray typesThatOccludeValidInfoOutOfROI =
            {
              {NavMemoryMapTypes::EContentType::Unknown               , false},
              {NavMemoryMapTypes::EContentType::ClearOfObstacle       , false},
              {NavMemoryMapTypes::EContentType::ClearOfCliff          , false},
              {NavMemoryMapTypes::EContentType::ObstacleCube          , true },
              {NavMemoryMapTypes::EContentType::ObstacleCubeRemoved   , false},
              {NavMemoryMapTypes::EContentType::ObstacleCharger       , true },
              {NavMemoryMapTypes::EContentType::ObstacleChargerRemoved, true },
              {NavMemoryMapTypes::EContentType::ObstacleUnrecognized  , true },
              {NavMemoryMapTypes::EContentType::Cliff                 , true },
              {NavMemoryMapTypes::EContentType::InterestingEdge       , true },
              {NavMemoryMapTypes::EContentType::NotInterestingEdge    , true }
            };
            static_assert(NavMemoryMapTypes::IsSequentialArray(typesThatOccludeValidInfoOutOfROI),
              "This array does not define all types once and only once.");
            
            // check if it's occluded before the near plane
            const Vec2f& outerRayFrom = cameraOrigin;
            const Vec2f outerRayTo(rayAtNearPlane.x, rayAtNearPlane.y);
            occludedBeforeNearPlane = currentNavMemoryMap->HasCollisionRayWithTypes(outerRayFrom, outerRayTo, typesThatOccludeValidInfoOutOfROI);
            
            // update innerRayFrom so that the second ray (if needed) only checks the inside of the ROI plane
            innerRayFrom = outerRayTo; // start inner (innerFrom) where the outer ends (outerTo)
          }
        }
    
        // - calculate occlusion inside ROI
        if ( !occludedBeforeNearPlane )
        {
          /*
            Note on occludedBeforeNearPlane vs occludedInsideROI:
            We want to check two different zones: one from cameraOrigin to nearPlane and another from nearPlane to candidate3D. 
            The first one, being out of the current ground ROI can be more restrictive (fail on borders), since we
            literally have no information to back up a ClearOfObstacle, however the second one can't fail on borders,
            since borders are exactly what we are detecting, so the point can't become invalid when a border is detected.
            That should be the main difference between typesThatOccludeValidInfoOutOfROI vs typesThatOccludeValidInfoInsideROI.
          */
          // typesThatOccludeValidInfoInsideROI = types to check against inside ground ROI:
          constexpr NavMemoryMapTypes::FullContentArray typesThatOccludeValidInfoInsideROI =
          {
            {NavMemoryMapTypes::EContentType::Unknown               , false},
            {NavMemoryMapTypes::EContentType::ClearOfObstacle       , false},
            {NavMemoryMapTypes::EContentType::ClearOfCliff          , false},
            {NavMemoryMapTypes::EContentType::ObstacleCube          , true },
            {NavMemoryMapTypes::EContentType::ObstacleCubeRemoved   , false},
            {NavMemoryMapTypes::EContentType::ObstacleCharger       , true },
            {NavMemoryMapTypes::EContentType::ObstacleChargerRemoved, true },
            {NavMemoryMapTypes::EContentType::ObstacleUnrecognized  , true },
            {NavMemoryMapTypes::EContentType::Cliff                 , false},
            {NavMemoryMapTypes::EContentType::InterestingEdge       , false },
            {NavMemoryMapTypes::EContentType::NotInterestingEdge    , false }
          };
          static_assert(NavMemoryMapTypes::IsSequentialArray(typesThatOccludeValidInfoInsideROI),
            "This array does not define all types once and only once.");
          
          // Vec2f innerRayFrom: already calculated for us
          const Vec2f& innerRayTo = candidate3D;
          occludedInsideROI = currentNavMemoryMap->HasCollisionRayWithTypes(innerRayFrom, innerRayTo, typesThatOccludeValidInfoInsideROI);
        }
        
        // if we cross an object, ignore this point, regardless of whether we saw a border or not
        // this is because if we are crossing an object, chances are we are seeing its border, of we should have,
        // so the info is more often disrupting than helpful
        bool isValidPoint = !occludedBeforeNearPlane && !occludedInsideROI;
        
        // this flag is set by a point that can't merge into the previous segment and wants to start one
        // on its own
        bool shouldCreateNewSegment = false;

        // valid points have to be checked to see what to do with them
        if ( isValidPoint )
        {
          // this point is valid, check whether:
          // a) it's the first of a segment
          // b) it's the second of a segment, which defines the normal of the running segment
          // c) it can be merged into a running segment
          // d) it can't be merged into a running segment
          if ( !hasValidSegmentStart )
          {
            // it's the first of a segment
            segmentStart = candidate3D;
            hasValidSegmentStart = true;
          }
          else if ( !hasValidSegmentEnd )
          {
            // it's the second of a segment (defines normal)
            segmentEnd = candidate3D;
            hasValidSegmentEnd = true;
            // calculate normal now
            segmentNormal = (segmentEnd-segmentStart);
            segmentNormal.MakeUnitLength();
          }
          else
          {
            // there's a running segment already, check if we can merge into it
      
            // epsilon to merge points into the same edge segment. If adding a point to a segment creates
            // a deviation with respect to the first direction of the segment bigger than this epsilon,
            // then the point will not be added to that segment
            // cos(10deg) = 0.984807...
            // cos(20deg) = 0.939692...
            // cos(30deg) = 0.866025...
            // cos(40deg) = 0.766044...
            const float kDotBorderEpsilon = 0.7660f;
      
            // calculate the normal that this candidate would have with respect to the previous point
            Vec3f candidateNormal = candidate3D - segmentEnd;
            candidateNormal.MakeUnitLength();
        
            // check the dot product of that normal with respect to the running segment's normal
            const float dotProduct = DotProduct(segmentNormal, candidateNormal);
            bool canMerge = (dotProduct >= kDotBorderEpsilon); // if dotProduct is bigger, angle between is smaller
            if ( canMerge )
            {
              // it can merge into the previous point because the angle between the running normal and the new one
              // is within the threshold. Make this point the new end and update the running normal
              segmentEnd = candidate3D;
              segmentNormal = candidateNormal;
            }
            else
            {
              // it can't merge into the previous segment, set the flag that we want a new segment
              shouldCreateNewSegment = true;
            }
          }
          
          // store distance to valid points that belong to a detected border for behaviors quick access
          if ( chain.isBorder )
          {
            // compute distance from current robot position to the candidate3D, because whiteboard likes to know
            // the closest border at all times
            const Pose3d& currentRobotPose = _robot->GetPose();
            const float candidateDist_mm2 = (currentRobotPose.GetTranslation() - candidate3D).LengthSq();
            if ( std::isnan(closestPointDist_mm2) || FLT_LT(candidateDist_mm2, closestPointDist_mm2) )
            {
              closestPointDist_mm2 = candidateDist_mm2;
            }
          }
        }

        // should we send the segment we have so far as a quad to the memory map?
        const bool isLastPoint = (curPointInChainIdx == (chain.points.size()-1));
        const bool sendSegmentToMap = shouldCreateNewSegment || isLastPoint || !isValidPoint;
        if ( sendSegmentToMap )
        {
          // check if we have a valid segment so far
          const bool isValidSegment = hasValidSegmentStart && hasValidSegmentEnd;
          if ( isValidSegment )
          {
            // min length of the segment so that we can discard noise
            const float segLenSQ_mm = (segmentStart - segmentEnd).LengthSq();
            const bool isLongSegment = FLT_GT(segLenSQ_mm, kOverheadEdgeSegmentNoiseLen_mm);
            if ( isLongSegment )
            {
              // we have a valid and long segment add clear from camera to segment
              Quad2f clearQuad = { segmentStart, cameraOrigin, segmentEnd, cameraOrigin }; // TL, BL, TR, BR
              bool success = GroundPlaneROI::ClampQuad(clearQuad, nearPlaneLeft, nearPlaneRight);
              DEV_ASSERT(success, "AddVisionOverheadEdges.FailedQuadClamp");
              if ( success ) {
                visionQuadsClear.emplace_back(clearQuad);
              }
              // if it's a detected border, add the segment
              if ( chain.isBorder ) {
                visionSegmentsWithInterestingBorders.emplace_back(segmentStart, segmentEnd);
              }
            }
          }
          // else { not enough points in the segment to send. That's ok, just do not send }
          
          // if it was a valid point but could not merge, it wanted to start a new segment
          if ( shouldCreateNewSegment )
          {
            // if we can reuse the last point from the previous segment
            if ( hasValidSegmentEnd )
            {
              // then that one becomes the start
              // and we become the end of the new segment
              segmentStart = segmentEnd;
              hasValidSegmentStart = true;
              segmentEnd = candidate3D;
              hasValidSegmentEnd = true;
              // and update normal for this new segment
              segmentNormal = (segmentEnd-segmentStart);
              segmentNormal.MakeUnitLength();
            }
            else
            {
              // need to create a new segment, and there was not a valid one previously
              // this should be impossible, since we would have become either start or end
              // of a segment, and shouldCreateNewSegment would have been false
              DEV_ASSERT(false, "AddVisionOverheadEdges.NewSegmentCouldNotFindPreviousEnd");
            }
          }
          else
          {
            // we don't want to start a new segment, either we are the last point or we are not a valid point
            DEV_ASSERT(!isValidPoint || isLastPoint, "AddVisionOverheadEdges.ValidPointNotStartingSegment");
            hasValidSegmentStart = false;
            hasValidSegmentEnd   = false;
          }
        } // sendSegmentToMap?
          
        // move to next point
        ++curPointInChainIdx;
      } while (curPointInChainIdx < chain.points.size()); // while we still have points
    }

    // send clear quads/triangles to memory map
    // clear information should be quads, since the ground plane has a min dist that truncates the cone that spans
    // from the camera to the max ground plane distance. If the quad close segment is short though, it becomes
    // narrower, thus more similar to a triangle. As a performance optimization we are going to consider narrow
    // quads as triangles. This should be ok since the information is currently stored in quads that have a minimum
    // size, so the likelyhood that a quad that should be covered with the quads won't be hit by the triangles is
    // very small, and we are willing to take that risk to optimize this.
    /*
          Quad as it should be         Quad split into 3 quads with
                                          narrow close segment
                  v                                 v
               ________                         __     --
               \      /                         \ |---| /
                \    /                           \ | | /
                 \__/                             \_|_/
     
    */
    // for the same reason, if the far edge is too small, the triangle is very narrow, so it can be turned into a line,
    // and thanks to intersection we will probably not notice anything
    
    // Any quad whose closest edge is smaller than kOverheadEdgeNarrowCone_mm will be considered a triangle
    // const float minFarLenToBeReportedSq = kOverheadEdgeFarMinLenForClearReport_mm*kOverheadEdgeFarMinLenForClearReport_mm;
    const float maxFarLenToBeLineSq = kOverheadEdgeFarMaxLenForLine_mm*kOverheadEdgeFarMaxLenForLine_mm;
    const float maxCloseLenToBeTriangleSq = kOverheadEdgeCloseMaxLenForTriangle_mm*kOverheadEdgeCloseMaxLenForTriangle_mm;
    for ( const auto& potentialClearQuad2D : visionQuadsClear )
    {
    
// rsam note: I want to filter out small ones, but there were instances were this was not very good, for example if it
// happened at the border, since we would completely miss it. I will leave this out unless profiling says we need
// more optimizations
//      // quads that are too small are discarded. This is because in the general case they will be covered by nearby
//      // borders. If we discarded all because detection was too fine-grained, then we could run this loop again
//      // without this min restriction, but I don't think it will be an issue based on my tests, and the fact that we
//      // care more about big detectable borders, rather than small differences in the image (for example, we want
//      // to detect objects in real life that are similar to Cozmo's size)
//      const float farLenSq   = (potentialClearQuad2D.GetTopLeft() - potentialClearQuad2D.GetTopRight()).LengthSq();
//      const bool isTooSmall = FLT_LE(farLenSq, minFarLenToBeReportedSq);
//      if ( isTooSmall ) {
//        if ( kDebugRenderOverheadEdgeClearQuads )
//        {
//          ColorRGBA color = Anki::NamedColors::DARKGRAY;
//          VizManager* vizManager = _robot->GetContext()->GetVizManager();
//          vizManager->DrawQuadAsSegments("BlockWorld.AddVisionOverheadEdges", potentialClearQuad2D, kDebugRenderOverheadEdgesZ_mm, color, false);
//        }
//        continue;
//      }
      const float farLenSq   = (potentialClearQuad2D.GetTopLeft() - potentialClearQuad2D.GetTopRight()).LengthSq();
      
      // test whether we can report as Line, Triangle or Quad
      const bool isLine = FLT_LE(farLenSq, maxFarLenToBeLineSq);
      if ( isLine )
      {
        // far segment is small enough that a single line would be fine
        const Point2f& clearFrom = (potentialClearQuad2D.GetBottomLeft() + potentialClearQuad2D.GetBottomRight()) * 0.5f;
        const Point2f& clearTo   = (potentialClearQuad2D.GetTopLeft()    + potentialClearQuad2D.GetTopRight()   ) * 0.5f;
      
        if ( kDebugRenderOverheadEdgeClearQuads )
        {
          ColorRGBA color = Anki::NamedColors::CYAN;
          VizManager* vizManager = _robot->GetContext()->GetVizManager();
          vizManager->DrawSegment("BlockWorld.AddVisionOverheadEdges",
            Point3f(clearFrom.x(), clearFrom.y(), kDebugRenderOverheadEdgesZ_mm),
            Point3f(clearTo.x(), clearTo.y(), kDebugRenderOverheadEdgesZ_mm),
            color, false);
        }

        // add clear info to map
        if ( currentNavMemoryMap ) {
          currentNavMemoryMap->AddLine(clearFrom, clearTo, INavMemoryMap::EContentType::ClearOfObstacle);
        }
      }
      else
      {
        const float closeLenSq = (potentialClearQuad2D.GetBottomLeft() - potentialClearQuad2D.GetBottomRight()).LengthSq();
        const bool isTriangle = FLT_LE(closeLenSq, maxCloseLenToBeTriangleSq);
        if ( isTriangle )
        {
          // far segment is big, but close one is small enough that a triangle would be fine
          const Point2f& triangleClosePoint = (potentialClearQuad2D.GetBottomLeft() + potentialClearQuad2D.GetBottomRight()) * 0.5f;
          
          Triangle2f clearTri2D(triangleClosePoint, potentialClearQuad2D.GetTopLeft(), potentialClearQuad2D.GetTopRight());
          if ( kDebugRenderOverheadEdgeClearQuads )
          {
            ColorRGBA color = Anki::NamedColors::DARKGREEN;
            VizManager* vizManager = _robot->GetContext()->GetVizManager();
            vizManager->DrawSegment("BlockWorld.AddVisionOverheadEdges",
              Point3f(clearTri2D[0].x(), clearTri2D[0].y(), kDebugRenderOverheadEdgesZ_mm),
              Point3f(clearTri2D[1].x(), clearTri2D[1].y(), kDebugRenderOverheadEdgesZ_mm),
              color, false);
            vizManager->DrawSegment("BlockWorld.AddVisionOverheadEdges",
              Point3f(clearTri2D[1].x(), clearTri2D[1].y(), kDebugRenderOverheadEdgesZ_mm),
              Point3f(clearTri2D[2].x(), clearTri2D[2].y(), kDebugRenderOverheadEdgesZ_mm),
              color, false);
            vizManager->DrawSegment("BlockWorld.AddVisionOverheadEdges",
              Point3f(clearTri2D[2].x(), clearTri2D[2].y(), kDebugRenderOverheadEdgesZ_mm),
              Point3f(clearTri2D[0].x(), clearTri2D[0].y(), kDebugRenderOverheadEdgesZ_mm),
              color, false);
          }

          // add clear info to map
          if ( currentNavMemoryMap ) {
            currentNavMemoryMap->AddTriangle(clearTri2D, INavMemoryMap::EContentType::ClearOfObstacle);
          }
        }
        else
        {
          // segments are too big, we need to report as quad
          if ( kDebugRenderOverheadEdgeClearQuads )
          {
            ColorRGBA color = Anki::NamedColors::GREEN;
            VizManager* vizManager = _robot->GetContext()->GetVizManager();
            vizManager->DrawQuadAsSegments("BlockWorld.AddVisionOverheadEdges", potentialClearQuad2D, kDebugRenderOverheadEdgesZ_mm, color, false);
          }

          // add clear info to map
          if ( currentNavMemoryMap ) {
            currentNavMemoryMap->AddQuad(potentialClearQuad2D, INavMemoryMap::EContentType::ClearOfObstacle);
          }
        }
      }
      
      // also notify behavior whiteboard.
      // rsam: should this information be in the map instead of the whiteboard? It seems a stretch that
      // blockworld knows now about behaviors, maybe all this processing of quads should be done in a separate
      // robot component, like a VisualInformationProcessingComponent
      // Note: we always consider quad here since whiteboard does not need the triangle optiomization
      _robot->GetAIComponent().GetWhiteboard().ProcessClearQuad(potentialClearQuad2D);
    }
    
    // send border segments to memory map
    for ( const auto& borderSegment : visionSegmentsWithInterestingBorders )
    {
      if ( kDebugRenderOverheadEdgeBorderQuads )
      {
        ColorRGBA color = Anki::NamedColors::BLUE;
        VizManager* vizManager = _robot->GetContext()->GetVizManager();
        vizManager->DrawSegment("BlockWorld.AddVisionOverheadEdges",
          Point3f(borderSegment.from.x(), borderSegment.from.y(), kDebugRenderOverheadEdgesZ_mm),
          Point3f(borderSegment.to.x(), borderSegment.to.y(), kDebugRenderOverheadEdgesZ_mm),
          color, false);
      }
    
      // add interesting edge
      if ( currentNavMemoryMap ) {
        currentNavMemoryMap->AddLine(borderSegment.from, borderSegment.to, INavMemoryMap::EContentType::InterestingEdge);
      }
    }
    
    // now merge interesting edges into non-interesting
    const bool addedEdges = !visionSegmentsWithInterestingBorders.empty();
    if ( addedEdges )
    {
      // TODO Optimization, build bounding box from detected edges, rather than doing the whole ground plane
      Point2f wrtOrigin2DTL = observedPose * Point3f(frameInfo.groundplane[Quad::TopLeft].x(),
                                                     frameInfo.groundplane[Quad::TopLeft].y(),
                                                     0.0f);
      Point2f wrtOrigin2DBL = observedPose * Point3f(frameInfo.groundplane[Quad::BottomLeft].x(),
                                                     frameInfo.groundplane[Quad::BottomLeft].y(),
                                                     0.0f);
      Point2f wrtOrigin2DTR = observedPose * Point3f(frameInfo.groundplane[Quad::TopRight].x(),
                                                     frameInfo.groundplane[Quad::TopRight].y(),
                                                     0.0f);
      Point2f wrtOrigin2DBR = observedPose * Point3f(frameInfo.groundplane[Quad::BottomRight].x(),
                                                     frameInfo.groundplane[Quad::BottomRight].y(),
                                                     0.0f);
      
      Quad2f groundPlaneWRTOrigin(wrtOrigin2DTL, wrtOrigin2DBL, wrtOrigin2DTR, wrtOrigin2DBR);
      ReviewInterestingEdges(groundPlaneWRTOrigin);
    }
    
    // notify the whiteboard we just processed edge information from a frame
    const float closestPointDist_mm = std::isnan(closestPointDist_mm2) ?
      std::numeric_limits<float>::quiet_NaN() : sqrt(closestPointDist_mm2);
    _robot->GetAIComponent().GetWhiteboard().SetLastEdgeInformation(frameInfo.timestamp, closestPointDist_mm);
    
    return RESULT_OK;
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


  Result BlockWorld::Update(std::list<Vision::ObservedMarker>& currentObsMarkers)
  {
    ANKI_CPU_PROFILE("BlockWorld::Update");

    const f32 currentTimeSec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if (_lastPlayAreaSizeEventSec + _playAreaSizeEventIntervalSec < currentTimeSec) {
      _lastPlayAreaSizeEventSec = currentTimeSec;
      const INavMemoryMap* currentNavMemoryMap = GetNavMemoryMap();
      const double areaM2 = currentNavMemoryMap->GetExploredRegionAreaM2();
      Anki::Util::sEventF("robot.play_area_size", {}, "%.2f", areaM2);
    }
    
    // clear the change list and start tracking them
    _objectPoseChangeList.clear();
    _trackPoseChanges = true;
    
    if(!currentObsMarkers.empty())
    {
      const TimeStamp_t atTimestamp = currentObsMarkers.front().GetTimeStamp();
      _currentObservedMarkerTimestamp = atTimestamp;
      
      // Sanity check
      if(ANKI_DEVELOPER_CODE)
      {
        for(auto const& marker : currentObsMarkers)
        {
          if(marker.GetTimeStamp() != atTimestamp)
          {
            PRINT_NAMED_ERROR("BlockWorld.Update.MisMatchedTimestamps", "Expected t=%u, Got t=%u",
                              atTimestamp, marker.GetTimeStamp());
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

// rsam: this code was for Mat localization. Now that localization happens by cubes, this code makes no sense
//      // Only update robot's poses using VisionMarkers while not on a ramp
//      if(!_robot->IsOnRamp()) {
//        if (!_robot->IsPhysical() || !SKIP_PHYS_ROBOT_LOCALIZATION) {
//          // Note that this removes markers from the list that it uses
//          UpdateRobotPose(currentObsMarkers, atTimestamp);
//        }
//      }
      
      // Reset the flag telling us objects changed here, before we update any objects:
      _didObjectsChange = false;

      Result updateResult;

      
      // TODO: Combine these into a single UpdateObjectPoses call for all families (COZMO-9319)
      
      //
      // Find any observed active blocks from the remaining markers.
      // Do these first because they can update our localization, meaning that
      // other objects found below will be more accurately localized.
      //
      // Note that this removes markers from the list that it uses
      updateResult = UpdateObjectPoses(currentObsMarkers, ObjectFamily::LightCube, atTimestamp);
      if(updateResult != RESULT_OK) {
        return updateResult;
      }
      
      //
      // Find any observed blocks from the remaining markers
      //
      // Note that this removes markers from the list that it uses
      updateResult = UpdateObjectPoses(currentObsMarkers, ObjectFamily::Block, atTimestamp);
      if(updateResult != RESULT_OK) {
        return updateResult;
      }
      
      //
      // Find any observed ramps from the remaining markers
      //
      // Note that this removes markers from the list that it uses
      updateResult = UpdateObjectPoses(currentObsMarkers, ObjectFamily::Ramp, atTimestamp);
      if(updateResult != RESULT_OK) {
        return updateResult;
      }
      
      //
      // Find any observed chargers from the remaining markers
      //
      // Note that this removes markers from the list that it uses
      updateResult = UpdateObjectPoses(currentObsMarkers, ObjectFamily::Charger, atTimestamp);
      if(updateResult != RESULT_OK) {
        return updateResult;
      }
      
      //
      // Find any observed customObjects from the remaining markers
      //
      // Note that this removes markers from the list that it uses
      updateResult = UpdateObjectPoses(currentObsMarkers, ObjectFamily::CustomObject, atTimestamp);
      if(updateResult != RESULT_OK) {
        return updateResult;
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
      
      
      const TimeStamp_t lastImgTimestamp = _robot->GetLastImageTimeStamp();
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
    if(!_robot->IsPickingOrPlacing())
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
        ObservableObject* objectOnTop = object;
        BOUNDED_WHILE(20, objectOnTop != nullptr) {
          _robot->GetObjectPoseConfirmer().MarkObjectDirty(objectOnTop);
          objectOnTop = FindLocatedObjectOnTopOf(*objectOnTop, STACKED_HEIGHT_TOL_MM);
        }
      };
      
      ModifyLocatedObjects(markAsDirty, unobservedCollidingObjectFilter);
    }
    
    
    // TODO: Deal with unknown markers?
    
    // Keep track of how many markers went unused by either robot or block
    // pose updating processes above
    const size_t numUnusedMarkers = currentObsMarkers.size();
    
    for(auto & unusedMarker : currentObsMarkers) {
      PRINT_CH_INFO("BlockWorld", "BlockWorld.Update.UnusedMarker",
                    "An observed %s marker went unused.",
                    unusedMarker.GetCodeName());
    }

    if(numUnusedMarkers > 0) {
      if (!_robot->IsPhysical() || !SKIP_PHYS_ROBOT_LOCALIZATION) {
        PRINT_NAMED_WARNING("BlockWorld.Update.UnusedMarkers",
                            "%zu observed markers did not match any known objects and went unused.",
                            numUnusedMarkers);
      }
    }
    
    //Update  block configurations now that all block poses have been updated
    _blockConfigurationManager->Update();
    
    
    Result lastResult = UpdateMarkerlessObjects(_robot->GetLastImageTimeStamp());
    
    /*
    Result lastResult = UpdateProxObstaclePoses();
    if(lastResult != RESULT_OK) {
      return lastResult;
    }
    */

    return lastResult;
    
  } // Update()
  
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
    const bool isCarryingObject = _robot->IsCarryingObject(objectID);
    const bool isDockingWithObject = _robot->GetDockObject() == objectID;
    if(isCarryingObject || isDockingWithObject) {
      return false;
    }
    
    // Check block's bounding box in same coordinates as this robot to
    // see if it intersects with the robot's bounding box. Also check to see
    // block and the robot are at overlapping heights.  Skip this check
    // entirely if the block isn't in the same coordinate tree as the
    // robot.
    Pose3d objectPoseWrtRobotOrigin;
    if(false == object->GetPose().GetWithRespectTo(*_robot->GetWorldOrigin(), objectPoseWrtRobotOrigin))
    {
      PRINT_NAMED_WARNING("BlockWorld.CheckForCollisionWithRobot.BadOrigin",
                          "Could not get %s %d pose (origin: %s[%p]) w.r.t. robot origin (%s[%p])",
                          EnumToString(object->GetType()), objectID.GetValue(),
                          object->GetPose().FindOrigin().GetName().c_str(),
                          &object->GetPose().FindOrigin(),
                          _robot->GetWorldOrigin()->GetName().c_str(),
                          _robot->GetWorldOrigin());
      return false;
    }
    
    // Check the that the object is in the same plane as the robot
    // TODO: Better check for being in the same plane that takes the
    //       vertical extent of the object (in its current pose) into account
    
    const f32 objectHeight = objectPoseWrtRobotOrigin.GetTranslation().z();
    const f32 robotBottom = _robot->GetPose().GetTranslation().z();
    const f32 robotTop    = robotBottom + ROBOT_BOUNDING_Z;
    
    const bool inSamePlane = (objectHeight >= robotBottom && objectHeight <= robotTop);
    if(!inSamePlane) {
      return false;
    }
    
    // Check if the object's bounding box intersects the robot's
    const Quad2f objectBBox = object->GetBoundingQuadXY(objectPoseWrtRobotOrigin);
    const Quad2f robotBBox = _robot->GetBoundingQuadXY(_robot->GetPose().GetWithRespectToOrigin(),
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
  
  Result BlockWorld::UpdateMarkerlessObjects(TimeStamp_t atTimestamp)
  {
    // Remove old obstacles or ones intersecting with robot (except cliffs)
    BlockWorldFilter filter;
    filter.AddAllowedFamily(ObjectFamily::MarkerlessObject);
    filter.AddIgnoreType(ObjectType::CliffDetection);
    
    const f32 robotBottom  = _robot->GetPose().GetTranslation().z();
    const f32 robotTop     = robotBottom + ROBOT_BOUNDING_Z;
    const Quad2f robotBBox = _robot->GetBoundingQuadXY(_robot->GetPose().GetWithRespectToOrigin());
    
    filter.AddFilterFcn([this, atTimestamp, robotBottom, robotTop, &robotBBox](const ObservableObject* object) -> bool
                        {
                          if(object->GetLastObservedTime() + kMarkerlessObjectExpirationTime_ms < atTimestamp)
                          {
                            // Object has expired
                            PRINT_CH_DEBUG("BlockWorld", "BlockWorld.UpdateMarkerlessObjects.RemovingExpired",
                                           "%s %d not seen since %d. Current time=%d",
                                           EnumToString(object->GetType()), object->GetID().GetValue(),
                                           object->GetLastObservedTime(), atTimestamp);
                            return true;
                          }
                          
                          if(object->GetLastObservedTime() >= atTimestamp)
                          {
                            // Don't remove by collision markerless objects that were _just_
                            // created/observed
                            return false;
                          }
                          
                          Pose3d objectPoseWrtRobotOrigin;
                          if(true == object->GetPose().GetWithRespectTo(*_robot->GetWorldOrigin(), objectPoseWrtRobotOrigin))
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
    
    std::vector<ObservableObject*> intersectingAndOldObjects;
    FindLocatedMatchingObjects(filter, intersectingAndOldObjects);
    
    for(ObservableObject* object : intersectingAndOldObjects)
    {
      DeleteLocatedObjectByIDInCurOrigin(object->GetID());
    }
    
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
    if(_robot->GetCarryingObject() == object->GetID()) {
      PRINT_CH_INFO("BlockWorld", "BlockWorld.ClearObjectHelper.ClearingCarriedObject",
                    "Clearing %s object %d which robot %d thinks it is carrying.",
                    ObjectTypeToString(object->GetType()),
                    object->GetID().GetValue(),
                    _robot->GetID());
      _robot->UnSetCarryingObjects();
    }
    
    if(_selectedObject == object->GetID()) {
      PRINT_CH_INFO("BlockWorld", "BlockWorld.ClearObjectHelper.ClearingSelectedObject",
                    "Clearing %s object %d which is currently selected.",
                    ObjectTypeToString(object->GetType()),
                    object->GetID().GetValue());
      _selectedObject.UnSet();
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
    _robotMsgTimeStampAtChange = _robot->GetLastMsgTimestamp();
  }
  
  ObservableObject* BlockWorld::FindObjectOnTopOrUnderneathHelper(const ObservableObject& referenceObject,
                                                                  f32 zTolerance,
                                                                  const BlockWorldFilter& filterIn,
                                                                  bool onTop) const
  {
    // Three checks:
    // 1. objects are within same coordinate frame
    // 2. centroid of candiate object projected onto "ground" (XY) plane must lie within
    //    the check object's projected bounding box
    // 3. (a) for "onTop": bottom of candidate object must be "near" top of reference object
    //    (b) for "!onTop": top of canddiate object must be "near" bottom of reference object
    
    const Pose3d refWrtOrigin = referenceObject.GetPose().GetWithRespectToOrigin();
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
        DEV_ASSERT(nullptr != refWrtOrigin.GetParent(), "BlockWorld.FindLocatedObjectOnTopOfUnderneathHelper.NullParent");
        
        Pose3d candidateWrtOrigin;
        const bool inSameFrame = candidateObject->GetPose().GetWithRespectTo(*refWrtOrigin.GetParent(), candidateWrtOrigin);
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
        Quad2f foundQuad = foundObject->GetBoundingQuadXY(foundObject->GetPose().GetWithRespectToOrigin());
        
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
        const Vec3f& zCoordRotation = foundObject->GetPose().GetWithRespectToOrigin().GetRotation().GetRotationMatrix().GetRow(2);
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
    TimeStamp_t bestTime = 0;
    
    BlockWorldFilter filter(filterIn);
    filter.AddFilterFcn([&bestTime](const ObservableObject* current)
    {
      const TimeStamp_t currentTime = current->GetLastObservedTime();
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
      const f32 objectCenter = object->GetPose().GetWithRespectToOrigin().GetTranslation().z();
        
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
    filter.SetIgnoreIDs(std::set<ObjectID>(_robot->GetCarryingObjects()));
    
    // Figure out height filters in world coordinates (because GetLocatedObjectBoundingBoxesXY()
    // uses heights of objects in world coordinates)
    const Pose3d robotPoseWrtOrigin = _robot->GetPose().GetWithRespectToOrigin();
    const f32 minHeight = robotPoseWrtOrigin.GetTranslation().z();
    const f32 maxHeight = minHeight + _robot->GetHeight();
    
    GetLocatedObjectBoundingBoxesXY(minHeight, maxHeight, padding, boundingBoxes, filter);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool BlockWorld::IsZombiePoseOrigin(const Pose3d* origin) const
  {
    // really, pass in an origin
    DEV_ASSERT(nullptr != origin && origin->IsOrigin(), "BlockWorld.IsZombiePoseOrigin.NotAnOrigin");
  
    // current world is not a zombie
    const bool isCurrent = (origin == _robot->GetWorldOrigin());
    if ( isCurrent ) {
      return false;
    }
    
    // check if there are any objects we can localize to
    const bool hasLocalizableObjects = AnyRemainingLocalizableObjects(origin);
    const bool isZombie = !hasLocalizableObjects;
    return isZombie;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool BlockWorld::AnyRemainingLocalizableObjects() const
  {
    return AnyRemainingLocalizableObjects(nullptr);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool BlockWorld::AnyRemainingLocalizableObjects(const Pose3d* origin) const
  {
    // Filter out anything that can't be used for localization 
    BlockWorldFilter filter;
    filter.SetFilterFcn([origin](const ObservableObject* obj) {
      return obj->CanBeUsedForLocalization();
    });
    
    // Allow all origins if origin is nullptr or allow only the specified origin
    filter.SetOriginMode(BlockWorldFilter::OriginMode::Custom);
    if(origin != nullptr) {
      filter.AddAllowedOrigin(origin);
    }
    
    if(nullptr != FindLocatedObjectHelper(filter, nullptr, true)) {
      return true;
    } else {
      return false;
    }
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::DeleteAllLocatedObjects()
  {
    // clear all objects in the current origin (other origins should not be necessary)
    ModifyLocatedObjects([this](ObservableObject* object) { ClearLocatedObjectHelper(object); });
    
    // clear the entire map of located instances (smart pointers will free)
    _locatedObjects.clear();    
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::DeleteLocatedObjectsByOrigin(const Pose3d* origin)
  {
    auto originIter = _locatedObjects.find(origin);
    if(originIter != _locatedObjects.end())
    {
      const bool isCurrentOrigin = (originIter->first == _robot->GetWorldOrigin());
      
      // cache objectIDs since we are going to destroy the objects
      std::set<ObjectID> objectsInOrigin;
      
      // iterate all families and types
      for(auto & objectsByFamily : originIter->second)
      {
        for(auto & objectsByType : objectsByFamily.second)
        {
          // for every object
          auto objectIter = objectsByType.second.begin();
          while(objectIter != objectsByType.second.end())
          {
            // cache its ID (for later usage)
            objectsInOrigin.insert(objectIter->second->GetID());
            
            // clear the object if it's in the current origin (other origins should not be necessary)
            if ( isCurrentOrigin ) {
              ClearLocatedObjectHelper(objectIter->second.get());
            }
            ++objectIter;
          }
        }
      }
      _locatedObjects.erase(originIter);
      
      // rsampedro: I don't know if this is useful anymore. Some of these things were due to active/unique objects
      // being destroyed, which now can be tracked separately due to the connected objects container
      
      // Find all objects in all _other_ origins and remove their IDs from the
      // set of object IDs we found in _this_ origin. If anything is left in the
      // set, then that object ID did not exist in any other frame and thus its
      // deletion should be broadcast below. Note that we can search all origins
      // now because we just erased "origin".
      BlockWorldFilter filter;
      filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
      filter.SetFilterFcn([&objectsInOrigin](const ObservableObject* object)
                          {
                            objectsInOrigin.erase(object->GetID());
                            return true;
                          });
      
      FilterLocatedObjects(filter);
      
      for(const ObjectID& objectID : objectsInOrigin)
      {
        PRINT_CH_DEBUG("BlockWorld", "BlockWorld.DeleteObjectsByOrigin.RemovedObjectFromAllFrames",
                       "Broadcasting deletion of object %d, which no longer exists in any frame",
                       objectID.GetValue());
        
        _robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotDeletedObject(_robot->GetID(),
                                                                                                       objectID)));
      }
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::DeleteLocatedObjectsByFamily(const ObjectFamily family)
  {
    // TODO what happens when we try to delete ActiveObjects? We do not remove their connected instance at the moment,
    // is that expected from this method?
  
    std::set<ObjectID> idsToBroadcast;
    for(auto & objectsByOrigin : _locatedObjects) {
      auto familyIter = objectsByOrigin.second.find(family);
      if(familyIter != objectsByOrigin.second.end()) {
        for(auto & objectsByType : familyIter->second) {
          for(auto & objectsByID : objectsByType.second) {
            idsToBroadcast.insert(objectsByID.first);
            
            // clear object in current origin (others should not be needed)
            const bool isCurrentOrigin = (&objectsByID.second->GetPose().FindOrigin() == _robot->GetWorldOrigin());
            if ( isCurrentOrigin ) {
              ClearLocatedObjectHelper(objectsByID.second.get());
            }
          }
        }
        //
        objectsByOrigin.second.erase(familyIter);
      }
    }
    
    for(const auto& id : idsToBroadcast) {
      _robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotDeletedObject(_robot->GetID(),
                                                                                                     id)));
    }
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::DeleteLocatedObjectsByType(ObjectType type)
  {
    std::set<ObjectID> idsToBroadcast;
    for(auto & objectsByOrigin : _locatedObjects) {
      for(auto & objectsByFamily : objectsByOrigin.second) {
        auto typeIter = objectsByFamily.second.find(type);
        if(typeIter != objectsByFamily.second.end()) {
          for(auto & objectsByID : typeIter->second) {
          
            idsToBroadcast.insert(objectsByID.first);
            const bool isCurrentOrigin = (&objectsByID.second->GetPose().FindOrigin() == _robot->GetWorldOrigin());
            if ( isCurrentOrigin ) {
              ClearLocatedObjectHelper(objectsByID.second.get());
            }
          }
          
          objectsByFamily.second.erase(typeIter);
        }
      }
    }
    
    for(auto id : idsToBroadcast) {
      _robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotDeletedObject(_robot->GetID(),
                                                                                                     id)));
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::DeleteLocatedObjectsByID(const ObjectID& withID)
  {
    BlockWorldFilter filter;
    filter.SetAllowedIDs({withID});
    filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
    
    std::vector<ObservableObject*> objects;
    FindLocatedMatchingObjects(filter, objects);

    for(auto* object : objects)
    {
      DEV_ASSERT(nullptr != object, "BlockWorld.DeleteLocatedObjectByID.NullObject");
      
      // Need to do all the same cleanup as Clear() calls
      ClearLocatedObjectHelper(object);
      
      // Actually delete the object we found
      const Pose3d* origin = &object->GetPose().FindOrigin();
      ObjectFamily inFamily = object->GetFamily();
      ObjectType   withType = object->GetType();
      
      // And remove it from the container
      // Note: we're using shared_ptr to store the objects, so erasing from
      //       the container will delete it if nothing else is referring to it
      const size_t numDeleted = _locatedObjects.at(origin).at(inFamily).at(withType).erase(object->GetID());
      DEV_ASSERT_MSG(numDeleted != 0,
                     "BlockWorld.DeleteObject.NoObjectsDeleted",
                     "Origin %p Type %s ID %u",
                     origin, EnumToString(withType), object->GetID().GetValue());
    }
    
    // notify something was deleted
    if ( !objects.empty() ) {
      _robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotDeletedObject(_robot->GetID(), withID)));
    }
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::DeleteLocatedObjectByIDInCurOrigin(const ObjectID& withID)
  {
    // find the object in the current origin
    ObservableObject* object = GetLocatedObjectByID(withID);
    if ( nullptr != object )
    {
      // clear and delete
      ClearLocatedObjectHelper(object);
      
      const PoseOrigin* origin    = &object->GetPose().FindOrigin();
      const ObjectFamily inFamily = object->GetFamily();
      const ObjectType   withType = object->GetType();
      
      // And remove it from the container (smart pointer will delete)
      const size_t numDeleted = _locatedObjects.at(origin).at(inFamily).at(withType).erase(object->GetID());
      DEV_ASSERT_MSG(numDeleted != 0,
                     "BlockWorld.DeleteObject.NoObjectsDeleted",
                     "Origin %p Type %s ID %u",
                     origin, EnumToString(withType), object->GetID().GetValue());

      // TODO we need to revise Deleted vs InstanceDeleted (old Unknown)
      // VIP ^ Deleted used to mean including not connected. This should probably broadcast set to Unknown instead
      // broadcast if there was a deletion
      _robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotDeletedObject(_robot->GetID(),
                                                                                                     withID)));
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  BlockWorld::ObjectsMapByID_t::iterator BlockWorld::DeleteLocatedObjectAt(const ObjectsMapByID_t::iterator objIter,
                                                                           const ObjectType&   withType,
                                                                           const ObjectFamily& fromFamily)
  {
    ObservableObject* object = objIter->second.get();
    
    // clear if in current origin
    const PoseOrigin* origin = &objIter->second->GetPose().FindOrigin();
    const bool isCurrentOrigin = (origin == _robot->GetWorldOrigin());
    if ( isCurrentOrigin ) {
      ClearLocatedObjectHelper(object);
    }
      
    // Erase from the container and return the iterator to the next element
    // Note: we're using a shared_ptr so this should delete the object as well.
    const ObjectsMapByID_t::iterator next = _locatedObjects.at(origin).at(fromFamily).at(withType).erase(objIter);
    return next;
  }

  void BlockWorld::DeselectCurrentObject()
  {
    if(_selectedObject.IsSet()) {
      ActionableObject* curSel = dynamic_cast<ActionableObject*>(GetLocatedObjectByID(_selectedObject));
      if(curSel != nullptr) {
        curSel->SetSelected(false);
      }
      _selectedObject.UnSet();
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

  bool BlockWorld::SelectObject(const ObjectID& objectID)
  {
    ActionableObject* newSelection = dynamic_cast<ActionableObject*>(GetLocatedObjectByID(objectID));
    
    if(newSelection != nullptr) {
      // Unselect current object of interest, if it still exists (Note that it may just get
      // reselected here, but I don't think we care.)
      // Mark new object of interest as selected so it will draw differently
      DeselectCurrentObject();
      
      newSelection->SetSelected(true);
      _selectedObject = objectID;
      PRINT_STREAM_INFO("BlockWorld.SelectObject", "Selected Object with ID=" << objectID.GetValue());
      
      return true;
    } else {
      PRINT_STREAM_INFO("BlockWorld.SelectObject.InvalidID",
                        "Object with ID=" << objectID.GetValue() << " not found. Not updating selected object.");
      return false;
    }
  } // SelectObject()
  
  void BlockWorld::CycleSelectedObject()
  {
    if(_selectedObject.IsSet()) {
      // Unselect current object of interest, if it still exists (Note that it may just get
      // reselected here, but I don't think we care.)
      // Mark new object of interest as selected so it will draw differently
      ActionableObject* object = dynamic_cast<ActionableObject*>(GetLocatedObjectByID(_selectedObject));
      if(object != nullptr) {
        object->SetSelected(false);
      }
    }
    
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
         object->HasPreActionPoses() &&
         !_robot->IsCarryingObject(object->GetID()))
      {
        //PRINT_INFO("currID: %d", block.first);
        if (currSelectedObjectFound) {
          // Current block of interest has been found.
          // Set the new block of interest to the next block in the list.
          _selectedObject = object->GetID();
          newSelectedObjectSet = true;
          //PRINT_INFO("new block found: id %d  type %d", block.first, blockType.first);
          break;
        } else if (object->GetID() == _selectedObject) {
          currSelectedObjectFound = true;
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
           object->HasPreActionPoses() &&
           !_robot->IsCarryingObject(object->GetID()))
        {
          firstObject = obj->GetID();
          break;
        }
      } // for each family
      
      
      if (firstObject == _selectedObject || !firstObject.IsSet()){
        //PRINT_INFO("Only one object in existence.");
      } else {
        //PRINT_INFO("Setting object of interest to first block");
        _selectedObject = firstObject;
      }
    }
    
    // Mark new object of interest as selected so it will draw differently
    ActionableObject* object = dynamic_cast<ActionableObject*>(GetLocatedObjectByID(_selectedObject));
    if (object != nullptr) {
      object->SetSelected(true);
      PRINT_STREAM_INFO("BlockWorld.CycleSelectedObject", "Object of interest: ID = " << _selectedObject.GetValue());
    } else {
      PRINT_STREAM_INFO("BlockWorld.CycleSelectedObject", "No object of interest found");
    }

  } // CycleSelectedObject()
  
  void BlockWorld::DrawAllObjects() const
  {
    const ObjectID& locObject = _robot->GetLocalizedTo();
    
    // Note: only drawing objects in current coordinate frame!
    BlockWorldFilter filter;
    filter.SetOriginMode(BlockWorldFilter::OriginMode::InRobotFrame);
    ModifierFcn visualizeHelper = [this,&locObject](ObservableObject* object)
    {
      if(object->GetID() == locObject) {
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
    };

    FindLocatedObjectHelper(filter, visualizeHelper, false);
    
    // (Re)Draw the selected object separately so we can get its pre-action poses
    if(GetSelectedObject().IsSet())
    {
      const ActionableObject* selectedObject = dynamic_cast<const ActionableObject*>(GetLocatedObjectByID(GetSelectedObject()));
      if(selectedObject == nullptr) {
        PRINT_NAMED_WARNING("BlockWorld.DrawAllObjects.NullSelectedObject",
                            "Selected object ID = %d, but it came back null.",
                            GetSelectedObject().GetValue());
      } else {
        if(selectedObject->IsSelected() == false) {
          PRINT_NAMED_WARNING("BlockWorld.DrawAllObjects.SelectionMisMatch",
                              "Object %d is selected in BlockWorld but does not have its "
                              "selection flag set.", GetSelectedObject().GetValue());
        }
        
        std::vector<std::pair<Quad2f,ObjectID> > obstacles;
        _robot->GetBlockWorld().GetObstacles(obstacles);
        selectedObject->VisualizePreActionPoses(obstacles, &_robot->GetPose());
      }
    } // if selected object is set

  } // DrawAllObjects()
  
  
  void BlockWorld::NotifyBlockConfigurationManagerObjectPoseChanged(const ObjectID& objectID) const
  {
    _blockConfigurationManager->SetObjectPoseChanged(objectID);
  }
  
} // namespace Cozmo
} // namespace Anki
