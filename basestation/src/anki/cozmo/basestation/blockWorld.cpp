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

// TODO: this include is shared b/w BS and Robot.  Move up a level.
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/common/shared/utilities_shared.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/rect_impl.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/customObject.h"
#include "anki/cozmo/basestation/mat.h"
#include "anki/cozmo/basestation/markerlessObject.h"
#include "anki/cozmo/basestation/potentialObjectsForLocalizingTo.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/navMemoryMap/navMemoryMapFactory.h"
#include "bridge.h"
#include "flatMat.h"
#include "platform.h"
#include "anki/cozmo/basestation/ramp.h"
#include "anki/cozmo/basestation/charger.h"
#include "anki/cozmo/basestation/humanHead.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/navMemoryMap/iNavMemoryMap.h"
#include "anki/cozmo/basestation/navMemoryMap/quadData/navMemoryMapQuadData_Cliff.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "anki/vision/basestation/visionMarker.h"
#include "anki/vision/basestation/observableObjectLibrary_impl.h"
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
    case ObjectFamily::Invalid:
    case ObjectFamily::Unknown:
    case ObjectFamily::Ramp:
    case ObjectFamily::Mat:
    case ObjectFamily::MarkerlessObject:
    case ObjectFamily::CustomObject:
    break;
  };

  return retType;
}

};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// CONSOLE VARS

// kEnableMemoryMap: if set to true Cozmo creates/uses memory maps
CONSOLE_VAR(bool, kEnableMemoryMap, "BlockWorld.MemoryMap", true);
// how often we request redrawing maps. Added because I think clad is getting overloaded with the amount of quads
CONSOLE_VAR(float, kMemoryMapRenderRate_sec, "BlockWorld.MemoryMap", 0.25f);

// kDebugRenderOverheadEdges: enables/disables debug render of points reported from vision
CONSOLE_VAR(bool, kDebugRenderOverheadEdges, "BlockWorld.MemoryMap", false);
// kDebugRenderOverheadEdgeClearQuads: enables/disables debug render of nonBorder quads from overhead detection (clear)
CONSOLE_VAR(bool, kDebugRenderOverheadEdgeClearQuads, "BlockWorld.MemoryMap", false);
// kDebugRenderOverheadEdgeBorderQuads: enables/disables debug render of border quads only (interesting edges)
CONSOLE_VAR(bool, kDebugRenderOverheadEdgeBorderQuads, "BlockWorld.MemoryMap", false);

// kReviewInterestingEdges: if set to true, interesting edges are reviewed after adding new ones to see whether they are still interesting
CONSOLE_VAR(bool, kReviewInterestingEdges, "BlockWorld.kReviewInterestingEdges", true);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  BlockWorld::BlockWorld(Robot* robot)
  : _robot(robot)
  , _didObjectsChange(false)
  , _canDeleteObjects(true)
  , _canAddObjects(true)
  , _currentNavMemoryMapOrigin(nullptr)
  , _isNavMemoryMapRenderEnabled(true)
  , _enableDraw(false)
  {
    CORETECH_ASSERT(_robot != nullptr);
    
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
    
    _objectLibrary[ObjectFamily::LightCube].AddObject(new ActiveCube(ObjectType::Block_LIGHTCUBE1));
    _objectLibrary[ObjectFamily::LightCube].AddObject(new ActiveCube(ObjectType::Block_LIGHTCUBE2));
    _objectLibrary[ObjectFamily::LightCube].AddObject(new ActiveCube(ObjectType::Block_LIGHTCUBE3));      
    
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
    _objectLibrary[ObjectFamily::Charger].AddObject(new Charger());
    
    if(_robot->HasExternalInterface())
    {
      SetupEventHandlers(*_robot->GetExternalInterface());
    }
          
  } // BlockWorld() Constructor

  void BlockWorld::DefineCustomObject(ObjectType type, f32 xSize_mm, f32 ySize_mm, f32 zSize_mm, f32 markerWidth_mm, f32 markerHeight_mm)
  {
      _objectLibrary[ObjectFamily::CustomObject].AddObject(new CustomObject(type, xSize_mm, ySize_mm, zSize_mm, markerWidth_mm, markerHeight_mm));
  }

  void BlockWorld::SetupEventHandlers(IExternalInterface& externalInterface)
  {
    using namespace ExternalInterface;
    auto helper = MakeAnkiEventUtil(externalInterface, *this, _eventHandles);
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ClearAllBlocks>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ClearAllObjects>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DeleteAllObjects>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DeleteAllCustomObjects>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetObjectAdditionAndDeletion>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SelectNextObject>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::CreateFixedCustomObject>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DefineCustomObject>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetMemoryMapRenderEnabled>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestKnownObjects>();
  }

  BlockWorld::~BlockWorld()
  {
    
  } // ~BlockWorld() Destructor
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::ClearAllBlocks& msg)
  {
    _robot->GetContext()->GetVizManager()->EraseAllVizObjects();
    ClearObjectsByFamily(ObjectFamily::Block);
    ClearObjectsByFamily(ObjectFamily::LightCube);
  };
  
  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::ClearAllObjects& msg)
  {
    _robot->GetContext()->GetVizManager()->EraseAllVizObjects();
    ClearAllExistingObjects();
  };
  
  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::DeleteAllObjects& msg)
  {
    _robot->GetContext()->GetVizManager()->EraseAllVizObjects();
    DeleteAllExistingObjects();
    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::RobotDeletedAllObjects>(_robot->GetID());
  };
  
  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::DeleteAllCustomObjects& msg)
  {
    _robot->GetContext()->GetVizManager()->EraseAllVizObjects();
    DeleteObjectsByFamily(ObjectFamily::CustomObject);
    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::RobotDeletedAllCustomObjects>(_robot->GetID());
  };
  
  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::SetObjectAdditionAndDeletion& msg)
  {
    EnableObjectAddition(msg.enableAddition);
    EnableObjectDeletion(msg.enableDeletion);
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
  void BlockWorld::HandleMessage(const ExternalInterface::DefineCustomObject& msg)
  {
    BlockWorld::DefineCustomObject(msg.objectType, msg.xSize_mm, msg.ySize_mm, msg.zSize_mm, msg.markerWidth_mm, msg.markerHeight_mm);
    
    _robot->GetContext()->GetExternalInterface()->BroadcastToGame<ExternalInterface::DefinedCustomObject>(_robot->GetID());
  };
  
  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::SetMemoryMapRenderEnabled& msg)
  {
    SetMemoryMapRenderEnabled(msg.enabled);
  };

  template<>
  void BlockWorld::HandleMessage(const ExternalInterface::RequestKnownObjects& msg)
  {
    BroadcastKnownObjects(msg.connectedObjectsOnly);
  };

  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  const BlockWorld::ObjectsMapByFamily_t& BlockWorld::GetAllExistingObjects() const
  {
    auto originIter = _existingObjects.find(_robot->GetWorldOrigin());
    if(originIter != _existingObjects.end()) {
      return originIter->second;
    } else {
      static const ObjectsMapByFamily_t EmptyObjectMapByFamily;
      return EmptyObjectMapByFamily;
    }
  }
  
  const BlockWorld::ObjectsMapByType_t& BlockWorld::GetExistingObjectsByFamily(const ObjectFamily whichFamily) const
  {
    auto originIter = _existingObjects.find(_robot->GetWorldOrigin());
    if(originIter != _existingObjects.end())
    {
      auto objectsWithFamilyIter = originIter->second.find(whichFamily);
      if(objectsWithFamilyIter != originIter->second.end()) {
        return objectsWithFamilyIter->second;
      }
    }

    static const BlockWorld::ObjectsMapByType_t EmptyObjectMapByType;
    return EmptyObjectMapByType;
  }
  
  const BlockWorld::ObjectsMapByID_t& BlockWorld::GetExistingObjectsByType(const ObjectType whichType) const
  {
    auto originIter = _existingObjects.find(_robot->GetWorldOrigin());
    if(originIter != _existingObjects.end())
    {
      for(auto & objectsByFamily : originIter->second) {
        auto objectsWithType = objectsByFamily.second.find(whichType);
        if(objectsWithType != objectsByFamily.second.end()) {
          return objectsWithType->second;
        }
      }
    }
    
    // Type not found!
    static const BlockWorld::ObjectsMapByID_t EmptyObjectMapByID;
    return EmptyObjectMapByID;
  }
  
  
  ObservableObject* BlockWorld::GetObjectByIdHelper(const ObjectID& objectID, ObjectFamily inFamily) const
  {
    // Find the object with the given ID with any pose state, in the current world origin
    BlockWorldFilter filter;
    filter.AddAllowedID(objectID);
    
    if(inFamily != ObjectFamily::Unknown) {
      filter.AddAllowedFamily(inFamily);
    }
    
    // Any pose state:
    filter.SetFilterFcn(nullptr);
    
    ObservableObject* match = FindObjectHelper(filter, nullptr, true);
    
    return match;
  } // GetObjectByIdHelper()
  
  
  ActiveObject* BlockWorld::GetActiveObjectByIdHelper(const ObjectID& objectID, ObjectFamily inFamily) const
  {
    BlockWorldFilter filter;
    filter.AddAllowedID(objectID);
    
    if(inFamily != ObjectFamily::Unknown) {
      filter.AddAllowedFamily(inFamily);
    }
    
    // Note: this replaces default filter fcn, so it will also search over any pose state
    filter.SetFilterFcn(&BlockWorldFilter::ActiveObjectsFilter);
    
    ObservableObject* object = FindObjectHelper(filter, nullptr, true);
    
    return dynamic_cast<ActiveObject*>(object);
  } // GetActiveObjectByIDHelper()
  
  
  ActiveObject* BlockWorld::GetActiveObjectByActiveIdHelper(const u32 activeID, const ObjectFamily inFamily) const
  {
    BlockWorldFilter filter;
    
    if(inFamily != ObjectFamily::Unknown) {
      filter.AddAllowedFamily(inFamily);
    }
    
    // Note: this replaces default filter fcn, so it will also search over any pose state
    filter.SetFilterFcn([activeID](const ObservableObject* object) {
                          return object->IsActive() && object->GetActiveID() == activeID;
                        });
   
    ObservableObject* match = FindObjectHelper(filter, nullptr, true);
    
    return dynamic_cast<ActiveObject*>(match);
  } // GetActiveObjectByActiveIDHelper()

  
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
  
  void BlockWorld::FindIntersectingObjects(const ObservableObject* objectSeen,
                                           std::vector<const ObservableObject*>& intersectingExistingObjects,
                                           f32 padding_mm,
                                           const BlockWorldFilter& filter) const
  {
    Quad2f quadSeen = objectSeen->GetBoundingQuadXY(objectSeen->GetPose(), padding_mm);
    FindMatchingObjects(GetIntersectingObjectsFilter(quadSeen, padding_mm, filter), intersectingExistingObjects);
  }
  
  void BlockWorld::FindIntersectingObjects(const ObservableObject* objectSeen,
                                           std::vector<ObservableObject*>& intersectingExistingObjects,
                                           f32 padding_mm,
                                           const BlockWorldFilter& filter)
  {
    Quad2f quadSeen = objectSeen->GetBoundingQuadXY(objectSeen->GetPose(), padding_mm);
    FindMatchingObjects(GetIntersectingObjectsFilter(quadSeen, padding_mm, filter), intersectingExistingObjects);
  }

  void BlockWorld::FindIntersectingObjects(const Quad2f& quad,
                                           std::vector<const ObservableObject *> &intersectingExistingObjects,
                                           f32 padding_mm,
                                           const BlockWorldFilter& filterIn) const
  {
    FindMatchingObjects(GetIntersectingObjectsFilter(quad, padding_mm, filterIn), intersectingExistingObjects);
  }
  
  
  void BlockWorld::FindIntersectingObjects(const Quad2f& quad,
                                           std::vector<ObservableObject *> &intersectingExistingObjects,
                                           f32 padding_mm,
                                           const BlockWorldFilter& filterIn)
  {
    FindMatchingObjects(GetIntersectingObjectsFilter(quad, padding_mm, filterIn), intersectingExistingObjects);
  }
  
  Result BlockWorld::BroadcastObjectObservation(const ObservableObject* observedObject,
                                                bool markersVisible)
  {    
    if(_robot->HasExternalInterface())
    {
      if(observedObject->IsExistenceConfirmed() || markersVisible)
      {
        // Project the observed object into the robot's camera, using its new pose
        std::vector<Point2f> projectedCorners;
        f32 observationDistance = 0;
        _robot->GetVisionComponent().GetCamera().ProjectObject(*observedObject, projectedCorners, observationDistance);
        
        Rectangle<f32> boundingBox(projectedCorners);
        
        Radians topMarkerOrientation(0);
        if(observedObject->IsActive()) {
          if (observedObject->GetFamily() == ObjectFamily::LightCube) {
            const ActiveCube* activeCube = dynamic_cast<const ActiveCube*>(observedObject);
            if(activeCube == nullptr) {
              PRINT_NAMED_ERROR("BlockWorld.BroadcastObjectObservation",
                                "ObservedObject %d with IsActive()==true could not be cast to ActiveCube.",
                                observedObject->GetID().GetValue());
              return RESULT_FAIL;
            } else {
              topMarkerOrientation = activeCube->GetTopMarkerOrientation();
              
              //PRINT_INFO("Object %d's rotation around Z = %.1fdeg\n", obsID.GetValue(),
              //           topMarkerOrientation.getDegrees());
            }
          }
        }
        
        using namespace ExternalInterface;
        
        RobotObservedObject observation(_robot->GetID(),
                                        observedObject->GetLastObservedTime(),
                                        observedObject->GetFamily(),
                                        observedObject->GetType(),
                                        observedObject->GetID(),
                                        boundingBox.GetX(),
                                        boundingBox.GetY(),
                                        boundingBox.GetWidth(),
                                        boundingBox.GetHeight(),
                                        observedObject->GetPose().ToPoseStruct3d(_robot->GetPoseOriginList()),
                                        topMarkerOrientation.ToFloat(),
                                        markersVisible,
                                        observedObject->IsActive());
        
        if( observedObject->IsExistenceConfirmed()) {
          _robot->Broadcast(MessageEngineToGame(std::move(observation)));
        }
        else if( markersVisible ) {
          // clear the object ID, since it isn't reliable until the existence is confirmed
          observation.objectID = -1;
          _robot->Broadcast(MessageEngineToGame(RobotObservedPossibleObject(std::move(observation))));
        }
      }
    } // if(_robot->HasExternalInterface())
    
    return RESULT_OK;
    
  } // BroadcastObjectObservation()
  
  
  void BlockWorld::BroadcastKnownObjects(bool connectedObjectsOnly)
  {
    if(_robot->HasExternalInterface())
    {
      using namespace ExternalInterface;
      
      std::vector<ObservableObject*> objects;
      
      // Create filter
      BlockWorldFilter filter;
      filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
      filter.SetFilterFcn(nullptr);  // To search for objects of all pose states
      if (connectedObjectsOnly) {
        filter.SetFilterFcn([](const ObservableObject* object) {
          return (object->GetActiveID() >= 0);
        });
      }
      
      FindMatchingObjects(filter, objects);
      
      for (auto obj : objects) {
        KnownObject objMsg(obj->GetID(),
                           obj->GetLastObservedTime(),
                           obj->GetFamily(),
                           obj->GetType(),
                           obj->GetPose().ToPoseStruct3d(_robot->GetPoseOriginList()),
                           obj->GetPoseState(),
                           true);
      
        _robot->Broadcast(MessageEngineToGame(KnownObject(std::move(objMsg))));
      }
      
      _robot->Broadcast(MessageEngineToGame(EndOfKnownObjects()));
    }
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
    filterOld.SetFilterFcn(nullptr); // Disable known-pose-state check
    filterOld.SetOriginMode(BlockWorldFilter::OriginMode::Custom);
    filterOld.AddAllowedOrigin(oldOrigin);
    
    // Use the modifier function to update matched objects to the new origin
    ModifierFcn originUpdater = [oldOrigin,newOrigin,&result,this](ObservableObject* oldObject){
      Pose3d newPose;
      if(false == oldObject->GetPose().GetWithRespectTo(*newOrigin, newPose)) {
        PRINT_NAMED_ERROR("BlockWorld.UpdateObjectOrigins.OriginFail",
                          "Could not get object %d w.r.t new origin %s",
                          oldObject->GetID().GetValue(),
                          newOrigin->GetName().c_str());
        
        result = RESULT_FAIL;
      } else {
        const Vec3f& T_old = oldObject->GetPose().GetTranslation();
        const Vec3f& T_new = newPose.GetTranslation();
        
        // Look for a matching object in the new origin (should have same family, type, and ID)
        BlockWorldFilter filterNew;
        filterNew.SetFilterFcn(nullptr);
        filterNew.SetOriginMode(BlockWorldFilter::OriginMode::Custom);
        filterNew.AddAllowedOrigin(newOrigin);
        filterNew.AddAllowedFamily(oldObject->GetFamily());
        filterNew.AddAllowedType(oldObject->GetType());
        filterNew.AddAllowedID(oldObject->GetID());
        ObservableObject* newObject = FindMatchingObject(filterNew);
        
        bool addNewObject = false;
        if(nullptr == newObject)
        {
          newObject = oldObject->CloneType();
          newObject->CopyID(oldObject);
          
          addNewObject = true;
        
          PRINT_NAMED_INFO("BlockWorld.UpdateObjectOrigins.NoMatchingObjectInNewFrame",
                           "Adding %s object with ID %d to new origin %s (%p)",
                           EnumToString(newObject->GetType()),
                           newObject->GetID().GetValue(),
                           newOrigin->GetName().c_str(),
                           newOrigin);
          
        }
        else
        {
          PRINT_NAMED_INFO("BlockWorld.UpdateObjectOrigins.ObjectOriginChanged",
                           "Updating object %d's origin from %s to %s. "
                           "T_old=(%.1f,%.1f,%.1f), T_new=(%.1f,%.1f,%.1f), "
                           "OldTimesObserved=%d, NewTimesObserved=%d",
                           oldObject->GetID().GetValue(),
                           oldOrigin->GetName().c_str(),
                           newOrigin->GetName().c_str(),
                           T_old.x(), T_old.y(), T_old.z(),
                           T_new.x(), T_new.y(), T_new.z(),
                           oldObject->GetNumTimesObserved(),
                           newObject->GetNumTimesObserved());
        }
        
        // Use all of oldObject's time bookkeeping, then update the pose and pose state
        // Note: SetPose will set pose state to known, but we want it to match oldObject's state
        newObject->SetObservationTimes(oldObject);
        newObject->SetPose(newPose, oldObject->GetLastPoseUpdateDistance());
        newObject->SetPoseState(oldObject->GetPoseState());

        if(addNewObject) {
          // Note: need to call SetPose first because that sets the origin which
          // controls which map the object gets added to
          AddNewObject(std::shared_ptr<ObservableObject>(newObject));
        }
        
        BroadcastObjectObservation(newObject, false);
      }
    };
    
    // Apply the filter and modify each object that matches
    FindObjectHelper(filterOld, originUpdater, false);
    
    if(RESULT_OK == result) {
      // Erase all the objects in the old frame now that their counterparts in the new
      // frame have had their poses updated. Note that we set clearFirst to false
      // because all the objects in the oldOrigin should now exist in the new origin
      DeleteObjectsByOrigin(oldOrigin, false);
    }
    
    // if memory maps are enabled, we can merge old into new
    if ( kEnableMemoryMap )
    {
      // oldOrigin is the pointer/id of the current map
      // worldOrigin is the pointer/id of the map we can merge into/from
      ASSERT_NAMED( _navMemoryMaps.find(oldOrigin) != _navMemoryMaps.end(), "BlockWorld.UpdateObjectOrigins.missingMapOriginOld");
      ASSERT_NAMED( _navMemoryMaps.find(newOrigin) != _navMemoryMaps.end(), "BlockWorld.UpdateObjectOrigins.missingMapOriginNew");
      ASSERT_NAMED( oldOrigin == _currentNavMemoryMapOrigin, "BlockWorld.UpdateObjectOrigins.updatingMapNotCurrent");

      // grab the underlying memory map and merge them
      INavMemoryMap* oldMap = _navMemoryMaps[oldOrigin].get();
      INavMemoryMap* newMap = _navMemoryMaps[newOrigin].get();
      Pose3d oldWrtNew;
      const bool success = oldOrigin->GetWithRespectTo(*newOrigin, oldWrtNew);
      ASSERT_NAMED(success, "BlockWorld.UpdateObjectOrigins.BadOldWrtNull");
      newMap->Merge(oldMap, oldWrtNew);
      
      // switch back to what is becoming the new map
      _currentNavMemoryMapOrigin = newOrigin;
      
      // now we can delete what is become the old map, since we have merged its data into the new one
      _navMemoryMaps.erase( oldOrigin ); // smart pointer will delete memory
    }
    
    return result;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  const INavMemoryMap* BlockWorld::GetNavMemoryMap() const
  {
    // current map (if any) must match current robot origin
    ASSERT_NAMED( (_currentNavMemoryMapOrigin == nullptr) || (_robot->GetWorldOrigin() == _currentNavMemoryMapOrigin),
      "BlockWorld.GetNavMemoryMap.BadOrigin");
    
    const INavMemoryMap* curMap = nullptr;
    if ( nullptr != _currentNavMemoryMapOrigin ) {
      auto matchPair = _navMemoryMaps.find(_currentNavMemoryMapOrigin);
      if ( matchPair != _navMemoryMaps.end() ) {
        curMap = matchPair->second.get();
      } else {
        ASSERT_NAMED(false, "BlockWorld.GetNavMemoryMap.MissingMap");
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
        ASSERT_NAMED(false, "BlockWorld.GetNavMemoryMap.MissingMap");
      }
    }
    return curMap;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::UpdateNavMemoryMap()
  {
    ANKI_CPU_PROFILE("BlockWorld::UpdateNavMemoryMap");
    
    INavMemoryMap* currentNavMemoryMap = GetNavMemoryMap();
    if ( nullptr != currentNavMemoryMap )
    {
      // cliff quad: clear or cliff
      {
        // TODO configure this size somethere else
        Point3f cliffSize = MarkerlessObject(ObjectType::ProxObstacle).GetSize() * 0.5f;
        
        const Pose3d robotPoseWrtOrigin = _robot->GetPose().GetWithRespectToOrigin();
        
        // cliff quad
        Quad3f cliffquad {
          {+cliffSize.x(), +cliffSize.y(), cliffSize.z()},  // up L
          {-cliffSize.x(), +cliffSize.y(), cliffSize.z()},  // lo L
          {+cliffSize.x(), -cliffSize.y(), cliffSize.z()},  // up R
          {-cliffSize.x(), -cliffSize.y(), cliffSize.z()}}; // lo R
        robotPoseWrtOrigin.ApplyTo(cliffquad, cliffquad);
        
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
      
      // forward sensor
      #define TRUST_FORWARD_SENSOR 0
      if( TRUST_FORWARD_SENSOR )
      {
        // - - -
        // ClearOfObstacle from 0                      to _forwardSensorValue_mm
        // Obstacle        from _forwardSensorValue_mm to MaxSensor
        // - - -
        const float sensorValue = ((float)_robot->GetForwardSensorValue());
        const float maxSensorValue = ((float)FORWARD_COLLISION_SENSOR_LENGTH_MM);
      
        // debug?
        const float kDebugRenderZOffset = 25.0f; // Z offset for render only so that it doesn't render underground
        const bool kDebugRenderForwardQuads = false;
        _robot->GetContext()->GetVizManager()->EraseSegments("BlockWorld::UpdateNavMemoryMap");
        
        // fetch vars
        const float kFrontCollisionSensorWidth = 1.0f; // TODO Should this be in CozmoEngineConfig.h ?
        const Pose3d& robotPoseWrtOrigin = _robot->GetPose().GetWithRespectToOrigin();

        // ray we cast from sensor
        const Vec3f forwardRay = robotPoseWrtOrigin.GetRotation() * X_AXIS_3D();
        
        // robot detection points
        Point3f robotForwardLeft  = robotPoseWrtOrigin * Vec3f{ 0.0f,  kFrontCollisionSensorWidth, 0.0f};
        Point3f robotForwardRight = robotPoseWrtOrigin * Vec3f{ 0.0f, -kFrontCollisionSensorWidth, 0.0f};
        const Point3f clearUntilLeft  = robotForwardLeft  + (forwardRay*sensorValue);
        const Point3f clearUntilRight = robotForwardRight + (forwardRay*sensorValue);
        
        // clear
        const bool hasClearInFront = Util::IsFltGTZero(sensorValue);
        if ( hasClearInFront )
        {
          // create quad for ClearOfObstacle
          const Point2f clearQuadBL( robotForwardLeft  );
          const Point2f clearQuadBR( robotForwardRight );
          const Point2f clearQuadTL( clearUntilLeft    );
          const Point2f clearQuadTR( clearUntilRight   );
          Quad2f clearCollisionQuad { clearQuadTL, clearQuadBL, clearQuadTR, clearQuadBR };
          currentNavMemoryMap->AddQuad(clearCollisionQuad, INavMemoryMap::EContentType::ClearOfObstacle);
          
          // also notify behavior whiteboard.
          // rsam: should this information be in the map instead of the whiteboard? It seems a stretch that
          // blockworld knows now about behaviors, maybe all this processing of quads should be done in a separate
          // robot component, like a VisualInformationProcessingComponent
          _robot->GetBehaviorManager().GetWhiteboard().ProcessClearQuad(clearCollisionQuad);
        
          // debug render detection lines
          if ( kDebugRenderForwardQuads )
          {
            _robot->GetContext()->GetVizManager()->DrawSegment("BlockWorld::UpdateNavMemoryMap",
              Point3f{clearQuadBL.x(),clearQuadBL.y(), kDebugRenderZOffset},
              Point3f{clearQuadTL.x(),clearQuadTL.y(), kDebugRenderZOffset},
              Anki::NamedColors::WHITE,
              false);
            _robot->GetContext()->GetVizManager()->DrawSegment("BlockWorld::UpdateNavMemoryMap",
              Point3f{clearQuadBR.x(),clearQuadBR.y(), kDebugRenderZOffset},
              Point3f{clearQuadTR.x(),clearQuadTR.y(), kDebugRenderZOffset},
              Anki::NamedColors::OFFWHITE,
              false);
          }
        }

        // obstacle
        const bool detectedObstacle = Util::IsFltLT(sensorValue, maxSensorValue);
        if ( detectedObstacle )
        {
          // TODO configure this elsewhere. Should not need to create an obstacle just to grab its size
          Point3f obstacleSize = MarkerlessObject(ObjectType::ProxObstacle).GetSize() * 0.5f;
          const float distance = obstacleSize.x();
          const Point3f obstacleUntilLeft  = clearUntilLeft  + (forwardRay*distance);
          const Point3f obstacleUntilRight = clearUntilRight + (forwardRay*distance);
          
          // create quad for ObstacleUnrecognized
          const Point2f obsQuadBL( clearUntilLeft     );
          const Point2f obsQuadBR( clearUntilRight    );
          const Point2f obsQuadTL( obstacleUntilLeft  );
          const Point2f obsQuadTR( obstacleUntilRight );
          Quad2f obsCollisionQuad { obsQuadTL, obsQuadBL, obsQuadTR, obsQuadBR };
          currentNavMemoryMap->AddQuad(obsCollisionQuad, INavMemoryMap::EContentType::ObstacleUnrecognized);
        
          // debug render detection lines
          if ( kDebugRenderForwardQuads )
          {
            _robot->GetContext()->GetVizManager()->DrawSegment("BlockWorld::UpdateNavMemoryMap",
              Point3f{obsQuadBL.x(),obsQuadBL.y(), kDebugRenderZOffset},
              Point3f{obsQuadTL.x(),obsQuadTL.y(), kDebugRenderZOffset},
              Anki::NamedColors::ORANGE,
              false);
            _robot->GetContext()->GetVizManager()->DrawSegment("BlockWorld::UpdateNavMemoryMap",
              Point3f{obsQuadBR.x(),obsQuadBR.y(), kDebugRenderZOffset},
              Point3f{obsQuadTR.x(),obsQuadTR.y(), kDebugRenderZOffset},
              Anki::NamedColors::RED,
              false);
          }
        }
      }
      
      currentNavMemoryMap->AddQuad(_robot->GetBoundingQuadXY(_robot->GetPose().GetWithRespectToOrigin()),
                                   INavMemoryMap::EContentType::ClearOfObstacle );
    }
    
    // also notify behavior whiteboard.
    // rsam: should this information be in the map instead of the whiteboard? It seems a stretch that
    // blockworld knows now about behaviors, maybe all this processing of quads should be done in a separate
    // robot component, like a VisualInformationProcessingComponent
    _robot->GetBehaviorManager().GetWhiteboard().ProcessClearQuad(_robot->GetBoundingQuadXY());
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::FlagQuadAsNotInterestingEdges(const Quad2f& quadWRTOrigin)
  {
    INavMemoryMap* currentNavMemoryMap = GetNavMemoryMap();
    if( currentNavMemoryMap )
    {
      currentNavMemoryMap->AddQuad(quadWRTOrigin, INavMemoryMap::EContentType::NotInterestingEdge);
    }
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::CreateLocalizedMemoryMap(const Pose3d* worldOriginPtr)
  {
    // can disable the feature completely
    if ( !kEnableMemoryMap ) {
      return;
    }

    // Since we are going to create a new memory map, check if any of the existing ones have become a zombie
    // This could happen if either the current map never saw a localizable object, or if objects in previous maps
    // have been moved or deactivated, which invalidates them as localizable
    NavMemoryMapTable::iterator iter = _navMemoryMaps.begin();
    while ( iter != _navMemoryMaps.end() )
    {
      const bool isZombie = IsZombiePoseOrigin( iter->first );
      if ( isZombie ) {
        PRINT_NAMED_DEBUG("BlockWorld.CreateLocalizedMemoryMap", "Deleted map (%p) because it was zombie", iter->first);
        iter = _navMemoryMaps.erase(iter);
      } else {
        PRINT_NAMED_DEBUG("BlockWorld.CreateLocalizedMemoryMap", "Map (%p) is still good", iter->first);
        ++iter;
      }
    }
    
    // clear all memory map rendering because indexHints are changing
    ClearNavMemoryMapRender();
    
    // if the origin is null, we would never merge the map, which could leak if a new one was created
    // do not support this by not creating one at all if the origin is null
    ASSERT_NAMED(nullptr != worldOriginPtr, "BlockWorld.CreateLocalizedMemoryMap.NullOrigin");
    if ( nullptr != worldOriginPtr )
    {
      // create a new memory map in the given origin
      VizManager* vizMgr = _robot->GetContext()->GetVizManager();
      INavMemoryMap* navMemoryMap = NavMemoryMapFactory::CreateDefaultNavMemoryMap(vizMgr);
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
  void BlockWorld::AddNewObject(ObjectsMapByType_t& existingFamily, const std::shared_ptr<ObservableObject>& object)
  {
    if(!_canAddObjects)
    {
      // TODO: Just remove this?
      PRINT_NAMED_WARNING("BlockWorld.AddAndUpdateObject.AddingDisabled",
                          "Saw a new %s%s object, but adding objects is disabled.",
                          object->IsActive() ? "active " : "",
                          ObjectTypeToString(object->GetType()));
      
      // Keep looking through objects we saw
      return;
    }
    
    if(!object->GetID().IsSet()) {
      object->SetID();
    }
    
    // Set the viz manager on this new object
    object->SetVizManager(_robot->GetContext()->GetVizManager());
    
    existingFamily[object->GetType()][object->GetID()] = object;
    
    PRINT_NAMED_INFO("BlockWorld.AddNewObject",
                     "Adding new %s%s object and ID=%d at (%.1f, %.1f, %.1f), in frame %s.",
                     object->IsActive() ? "active " : "",
                     EnumToString(object->GetType()),
                     object->GetID().GetValue(),
                     object->GetPose().GetTranslation().x(),
                     object->GetPose().GetTranslation().y(),
                     object->GetPose().GetTranslation().z(),
                     object->GetPose().FindOrigin().GetName().c_str());
  }
  
 
  
  
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
      // put this object directly into our existing objects if its new, in which case
      // it will not get deleted (since the existing objects will increase its refcount),
      // or it will simply be used to update an existing object's pose and/or for
      // localization, both done by potentialObjectsForLocalizingTo. In that case,
      // once potentialObjectsForLocalizingTo is done with it, and we have exited
      // this for loop, its refcount will be zero and it'll get deleted for us.
      std::shared_ptr<ObservableObject> objSeen(objSeenPair.second);
      
      // We use the distance to the observed object to decide (a) if the object is
      // close enough to do localization/identification and (b) to use only the
      // closest object in each coordinate frame for localization.
      const f32 distToObjSeen = ComputeDistanceBetween(_robot->GetPose(), objSeen->GetPose());
      
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
      filter.SetFilterFcn(nullptr);
      
      if (objSeen->IsActive())
      {
        // Can consider matches for active objects in other coordinate frames,
        // so use "Custom" mode and don't set any "allowed" or "ignored" origins,
        // meaning all will be considered
        filter.SetOriginMode(BlockWorldFilter::OriginMode::Custom);
        filter.AddFilterFcn(&BlockWorldFilter::ActiveObjectsFilter);
        
        // For active objects, just match based on type, since we assume there is only one
        // active object of each type
        std::vector<ObservableObject*> objectsFound;
        FindMatchingObjects(filter, objectsFound);
        
        if(objectsFound.empty()) {
          // We expect to have already heard from all (powered) active objects,
          // so if we see one we haven't heard from (and therefore added) yet, then
          // perhaps it isn't on?
          PRINT_NAMED_WARNING("BlockWorld.AddAndUpdateObjects.NoMatchForActiveObject",
                              "Observed active object of type %d does not match an existing object. "
                              "Is the battery plugged in?",
                              objSeen->GetType());
        }
        
        for(ObservableObject* objectFound : objectsFound)
        {
          assert(nullptr != objectFound);
          
          const Pose3d* origin = &objectFound->GetPose().FindOrigin();
          
          if(origin == currFrame)
          {
            // If this is the object we're carrying observed in the carry position,
            // do nothing and continue to the next observed object.
            // Otherwise, it must've been moved off the lift so unset its carry state.
            if (objectFound->GetID() == _robot->GetCarryingObject())
            {
              if (!objectFound->GetPose().IsSameAs(objSeen->GetPose(),
                                                   objSeen->GetSameDistanceTolerance(),
                                                   objSeen->GetSameAngleTolerance()))
              {
                _robot->UnSetCarryObject(objectFound->GetID());
              }
            }
          }
          
          // Check for duplicates (same type observed in the same coordinate frame)
          // and add to our map of matches by origin
          auto iter = matchingObjects.find(origin);
          if(iter != matchingObjects.end()) {
            PRINT_NAMED_WARNING("BlockWorld.AddAndUpdateObjects.MultipleMatchesForActiveObjectInSameFrame",
                                "Observed active object of type %s matches multiple existing objects of "
                                "same type and in the same frame (%p).",
                                EnumToString(objSeen->GetType()), origin);
          } else {
            matchingObjects[origin] = objectFound;
          }
        } // for each object found
    
      }
      else
      {
        // For passive objects, match based on pose (considering only objects in current frame)
        // Ignore objects we're carrying
        const ObjectID& carryingObjectID = _robot->GetCarryingObject();
        filter.AddFilterFcn([&carryingObjectID](const ObservableObject* candidate) {
          const bool isObjectBeingCarried = (candidate->GetID() == carryingObjectID);
          return !isObjectBeingCarried;
        });
        
        ObservableObject* matchingObject = FindClosestMatchingObject(*objSeen,
                                                                     objSeen->GetSameDistanceTolerance(),
                                                                     objSeen->GetSameAngleTolerance(),
                                                                     filter);
        if (nullptr != matchingObject) {
          ASSERT_NAMED(&matchingObject->GetPose().FindOrigin() == currFrame,
                       "BlockWorld.AddAndUpdateObjects.MatchedPassiveObjectInOtherCoordinateFrame");
          matchingObjects[currFrame] = matchingObject;
        }
        
      } // if/else object is active

      // As of now objSeen will be w.r.t. the robot's origin.  If we
      // observed it to be on a mat in the current frame, however, make it relative to that mat.
      std::vector<ObservableObject*> matsInCurrentFrame;
      BlockWorldFilter matFilter;
      matFilter.AddAllowedFamily(ObjectFamily::Mat);
      matFilter.SetOriginMode(BlockWorldFilter::OriginMode::InRobotFrame);
      FindMatchingObjects(matFilter, matsInCurrentFrame);
      ObservableObject* parentMat = nullptr;
      if(!matsInCurrentFrame.empty())
      {
        const f32 objectDiagonal = objSeen->GetSameDistanceTolerance().Length();
        for(auto object : matsInCurrentFrame)
        {
          MatPiece* mat = dynamic_cast<MatPiece*>(object);
          assert(mat != nullptr);
          
          // Don't make this mat the parent of any objects until it has been
          // seen enough time
          if(mat->GetNumTimesObserved() >= MIN_TIMES_TO_OBSERVE_OBJECT) {
            Pose3d newPoseWrtMat;
            // TODO: Better height tolerance approach
            if(mat->IsPoseOn(objSeen->GetPose(), objectDiagonal*.5f, objectDiagonal*.5f, newPoseWrtMat)) {
              objSeen->SetPose(newPoseWrtMat);
              parentMat = mat;
            }
          }
        }
      } // if(!matsInCurrentFrame.empty())
      
      // unless we match the observed object to an existing object, it was not in the memory map
      Pose3d oldPoseInMemMap;
      PoseState oldPoseStateInMemMap = PoseState::Unknown;

      // After the if/else below, this should point to either the new object we
      // create in the current frame, or to an existing matched object in the current
      // frame
      ObservableObject* observedObject = nullptr;
      
      auto currFrameMatchIter = matchingObjects.find(currFrame);
      if(currFrameMatchIter == matchingObjects.end())
      {
        // We did not find a match for the object in the current coordinate frame
        // If we didn't find a match in the current frame,
        
        // If we _did_ find matches for this object in _other_ coordinate frames...
        if(!matchingObjects.empty())
        {
          // ...keep the same ID(s)
          // NOTE: This kinda breaks the whole point of the ObjectID class that only one object can
          //  exist with each ID :-/
          objSeen->CopyID(matchingObjects.begin()->second);
          objSeen->SetActiveID(matchingObjects.begin()->second->GetActiveID()); // NOTE: also marks as "identified"
        }
        
        // Add the object in the current coordinate frame, initially with Known pose
        AddNewObject(_existingObjects[_robot->GetWorldOrigin()][inFamily], objSeen);
        
        if(_robot->GetMoveComponent().IsMoving() || distToObjSeen > MAX_LOCALIZATION_AND_ID_DISTANCE_MM) {
          // If we're seeing this object from too far away or while moving,
          // set it to have "dirty" pose, since we don't really trust it.
          // We want to guarantee we'll see the object from close enough and
          // while still before we use it for localization
          objSeen->SetPoseState(PoseState::Dirty);
        }
        
        // Let "observedObject" used below refer to the new object
        observedObject = objSeen.get();
      }
      else
      {
        // We found a match in the current frame, so update it:
        ObservableObject* matchingObject = currFrameMatchIter->second;
        
        // Check if there are objects on top of this object that need to be moved since the
        // object it's resting on has moved.
        UpdateRotationOfObjectsStackedOn(matchingObject, objSeen.get());
        
        // TODO: Do the same adjustment for blocks that are _below_ observed blocks? Does this make sense?
        
        // Update lastObserved times of this object
        // (Do this before possibly attempting to localize to the object below!)
        matchingObject->SetLastObservedTime(objSeen->GetLastObservedTime());
        matchingObject->UpdateMarkerObservationTimes(*objSeen);
        
        // Let "observedObject" used below refer to the object we matched in the
        // current frame
        observedObject = matchingObject;
        
        // copy old variables to mimic those of the map as of now
        oldPoseInMemMap = matchingObject->GetPose();
        oldPoseStateInMemMap = matchingObject->GetPoseState();
        
      } // if/else (currFrameMatchIter == matchingObjects.end())
      
      // Add all match pairs as potentials for localization. We will decide which
      // object(s) to localize to from this list after we've processed all objects
      // seen in this update.
      for(auto & match : matchingObjects)
      {
        potentialObjectsForLocalizingTo.Insert(objSeen, match.second, distToObjSeen);
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
      ASSERT_NAMED(&observedObject->GetPose().FindOrigin() == currFrame,
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
      ASSERT_NAMED(obsID.IsSet(), "BlockWorld.AddAndUpdateObjects.IDnotSet");
      
      
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
      BroadcastObjectObservation(observedObject, true);
      
      // new poses can be set during "potentialObjectsForLocalizingTo.Insert" (before) or
      // during "potentialObjectsForLocalizingTo.LocalizeRobot()". We can't trust the poseState we currently
      // have, since we don't know if this object is going to be used for localization. This is the
      // best estimate we can do here
      const bool isNewKnown = (!_robot->GetMoveComponent().IsMoving() && distToObjSeen <= MAX_LOCALIZATION_AND_ID_DISTANCE_MM);

      // oldPoseInMemMap      : set before
      // oldPoseStateInMemMap : set before
      const Pose3d& newPoseInMemMap = observedObject->GetPose();
      const PoseState newPoseStateInMemMap = isNewKnown ? PoseState::Known : PoseState::Dirty;
      OnObjectPoseChanged(observedObject->GetID(), observedObject->GetFamily(), oldPoseInMemMap, oldPoseStateInMemMap, newPoseInMemMap, newPoseStateInMemMap );
      
      _didObjectsChange = true;
      _currentObservedObjects.push_back(observedObject);
      
    } // for each object seen
    
    // NOTE: This will be a no-op if we no objects got inserted above as being potentially
    //       useful for localization
    Result localizeResult = potentialObjectsForLocalizingTo.LocalizeRobot();
    
    return localizeResult;
    
  } // AddAndUpdateObjects()
  
  
  void BlockWorld::UpdateRotationOfObjectsStackedOn(const ObservableObject* existingObjectOnBottom,
                                                    ObservableObject* objSeen)
  {
    // Updates poses of stacks of objects by finding the difference between old object poses and applying that
    // to the new observed poses. Has to use the old object to find the object on top
    ObservableObject* objectOnTop = FindObjectOnTopOf(*existingObjectOnBottom, STACKED_HEIGHT_TOL_MM);
    ObservableObject* newObjectOnBottom = objSeen;
    ObservableObject* oldObjectOnBottom = existingObjectOnBottom->CloneType();
    oldObjectOnBottom->SetPose(existingObjectOnBottom->GetPose());
    
    // If the object was already updated this timestamp then don't bother doing this.
    while(objectOnTop != nullptr && objectOnTop->GetLastObservedTime() != objSeen->GetLastObservedTime()) {
      
      // Get difference in position between top object's pose and the previous pose of the observed bottom object.
      // Apply difference to the new observed pose to get the new top object pose.
      Pose3d topPose = objectOnTop->GetPose();
      Pose3d bottomPose = oldObjectOnBottom->GetPose();
      Vec3f diff = topPose.GetTranslation() - bottomPose.GetTranslation();
      
      Radians zDiff = topPose.GetWithRespectToOrigin().GetRotation().GetAngleAroundZaxis() - bottomPose.GetWithRespectToOrigin().GetRotation().GetAngleAroundZaxis();
      topPose.SetRotation(Rotation3d(RotationVector3d(zDiff, Z_AXIS_3D())) * newObjectOnBottom->GetPose().GetRotation());
      
      topPose.SetTranslation( newObjectOnBottom->GetPose().GetTranslation() + diff );
      Util::SafeDelete(oldObjectOnBottom);
      oldObjectOnBottom = objectOnTop->CloneType();
      oldObjectOnBottom->SetPose(objectOnTop->GetPose());
      objectOnTop->SetPose(topPose);
      
      // COZMO-3384: we probably don't need to notify here, since the bottom quad will properly
      // mark the memory map. This is the reason why we probably should not add top cubes to the memory map at all
      
      // See if there's an object above this object
      newObjectOnBottom = objectOnTop;
      objectOnTop = FindObjectOnTopOf(*oldObjectOnBottom, STACKED_HEIGHT_TOL_MM);
    }
    
    Util::SafeDelete(oldObjectOnBottom);
    
  } // UpdateRotationOfObjectsStackedOn()
  
  u32 BlockWorld::CheckForUnobservedObjects(TimeStamp_t atTimestamp)
  {
    u32 numVisibleObjects = 0;
    
    // Don't bother if the robot is picked up or if it was moving too fast to
    // have been able to see the markers on the objects anyway.
    // NOTE: Just using default speed thresholds, which should be conservative.
    if(_robot->IsPickedUp() ||
       _robot->GetVisionComponent().WasMovingTooFast(atTimestamp))
    {
      return numVisibleObjects;
    }
    
    // Create a list of unobserved objects for further consideration below.
    struct UnobservedObjectContainer {
      ObjectFamily family;
      ObjectType   type;
      ObservableObject*      object;
      
      UnobservedObjectContainer(ObjectFamily family_, ObjectType type_, ObservableObject* object_)
      : family(family_), type(type_), object(object_) { }
    };
    std::vector<UnobservedObjectContainer> unobservedObjects;
    
    auto originIter = _existingObjects.find(_robot->GetWorldOrigin());
    if(originIter == _existingObjects.end()) {
      // No objects relative to this origin: Nothing to do
      return numVisibleObjects;
    }
    
    for(auto & objectFamily : originIter->second)
    {
      for(auto & objectsByType : objectFamily.second)
      {
        ObjectsMapByID_t& objectIdMap = objectsByType.second;
        for(auto objectIter = objectIdMap.begin();
            objectIter != objectIdMap.end(); )
        {
          ObservableObject* object = objectIter->second.get();
         
          // Look for blocks not seen atTimestamp, but skip objects
          // - with unknown pose state
          // - that are currently being carried
          // - whose pose origin does not match the robot's
          // - who are a charger (since those stay around)
          if(object->GetPoseState() != PoseState::Unknown &&
             object->GetLastObservedTime() < atTimestamp &&
             _robot->GetCarryingObject() != object->GetID() &&
             &object->GetPose().FindOrigin() == _robot->GetWorldOrigin() &&
             object->GetFamily() != ObjectFamily::Charger)
          {
            if(object->GetNumTimesObserved() < MIN_TIMES_TO_OBSERVE_OBJECT) {
              // If this object has only been seen once and that was too long ago,
              // just delete it, but only if this is a non-active object or radio
              // connection has not been established yet
              if (!object->IsActive() || object->GetActiveID() < 0) {
                PRINT_NAMED_INFO("BlockWorld.CheckForUnobservedObjects",
                                 "Deleting %s object %d that was only observed %d time(s).",
                                 ObjectTypeToString(object->GetType()),
                                 object->GetID().GetValue(),
                                 object->GetNumTimesObserved());
                objectIter = DeleteObject(objectIter, objectsByType.first, objectFamily.first);
              } else {
                ++objectIter;
              }
            } else if(object->IsActive() &&
                      ActiveIdentityState::WaitingForIdentity == object->GetIdentityState() &&
                      object->GetLastObservedTime() < atTimestamp - BLOCK_IDENTIFICATION_TIMEOUT_MS) {

              // If this is an active object and identification has timed out
              // delete it if radio connection has not been established yet.
              // Otherwise, retry identification.
              if (object->GetActiveID() < 0) {
                PRINT_NAMED_INFO("BlockWorld.CheckForUnobservedObjects.IdentifyTimedOut",
                                 "Deleting unobserved %s active object %d that has "
                                 "not completed identification in %dms",
                                 EnumToString(object->GetType()),
                                 object->GetID().GetValue(), BLOCK_IDENTIFICATION_TIMEOUT_MS);
                
                objectIter = DeleteObject(objectIter, objectsByType.first, objectFamily.first);
              } else {
                // Don't delete objects that are still in radio communication. Retrigger Identify?
                //PRINT_NAMED_WARNING("BlockWorld.CheckForUnobservedObjects.RetryIdentify", "Re-attempt identify on object %d (%s)", object->GetID().GetValue(), EnumToString(object->GetType()));
                //object->Identify();
                ++objectIter;
              }

            } else {
              // Otherwise, add it to the list for further checks below to see if
              // we "should" have seen the object

              if(_unidentifiedActiveObjects.count(object->GetID()) == 0) {
                //AddToOcclusionMaps(object, robotMgr_); // TODO: Used to do this too, put it back?
                unobservedObjects.emplace_back(objectFamily.first, objectsByType.first, objectIter->second.get());
              }
              ++objectIter;
              
            }
          } else {
            // Object _was_ observed or does not share an origin with the robot,
            // so skip it for analyzing below whether we *should* have seen it
            ++objectIter;
          } // if/else object was not observed
          
        } // for object IDs of this type
      } // for each object type
    } // for each object family
    
    // TODO: Don't bother with this if the robot is docking? (picking/placing)??
    // Now that the occlusion maps are complete, check each unobserved object's
    // visibility in each camera
    const Vision::Camera& camera = _robot->GetVisionComponent().GetCamera();
    ASSERT_NAMED(camera.IsCalibrated(), "BlockWorld.CheckForUnobservedObjects.CameraNotCalibrated");
    for(const auto& unobserved : unobservedObjects) {
      
      // Remove objects that should have been visible based on their last known
      // location, but which must not be there because we saw something behind
      // that location:
      const u16 xBorderPad = static_cast<u16>(0.05*static_cast<f32>(camera.GetCalibration()->GetNcols()));
      const u16 yBorderPad = static_cast<u16>(0.05*static_cast<f32>(camera.GetCalibration()->GetNrows()));
      bool hasNothingBehind = false;
        const bool shouldBeVisible = unobserved.object->IsVisibleFrom(camera,
                                                                      MAX_MARKER_NORMAL_ANGLE_FOR_SHOULD_BE_VISIBLE_CHECK_DEG,
                                                                      MIN_MARKER_SIZE_FOR_SHOULD_BE_VISIBLE_CHECK_PIX,
                                                                    xBorderPad, yBorderPad,
                                                                    hasNothingBehind);
      
      // NOTE: We expect a docking block to disappear from view!
      const bool isNotDockingObject = (_robot->GetDockObject() != unobserved.object->GetID());
      
      const bool isDirtyPoseState = (PoseState::Dirty == unobserved.object->GetPoseState());
      
      // If the object should _not_ be visible, but the reason was only that
      // it has nothing behind it to confirm that, _and_ the object has already
      // been marked dirty (e.g., by being moved), then increment the number of
      // times it has gone unobserved while dirty.
      if(!shouldBeVisible && hasNothingBehind && isDirtyPoseState)
      {
        unobserved.object->IncrementNumTimesUnobserved();
      }
      
      // Once we haven't observed a dirty object enough times, clear it
      const bool dirtyAndUnobservedTooManyTimes = (unobserved.object->GetNumTimesUnobserved() >
                                                   MIN_TIMES_TO_NOT_OBSERVE_DIRTY_OBJECT);
      
      if(isNotDockingObject && (shouldBeVisible || dirtyAndUnobservedTooManyTimes))
      {
        // Make sure there are no currently-observed, (just-)identified objects
        // with the same active ID present. (If there are, we'll reassign IDs
        // on the next update instead of clearing the existing object now.)
        bool matchingActiveIdFound = false;
        if(unobserved.object->IsActive()) {
          for(auto object : _currentObservedObjects) {
            if(ActiveIdentityState::Identified == object->GetIdentityState() &&
               object->GetActiveID() == unobserved.object->GetActiveID()) {
              matchingActiveIdFound = true;
              break;
            }
          }
        }
        
        if(!matchingActiveIdFound) {
          // We "should" have seen the object! Clear it.
          PRINT_NAMED_INFO("BlockWorld.CheckForUnobservedObjects.RemoveUnobservedObject",
                           "Clearing object %d, which should have been seen, but wasn't. "
                           "(shouldBeVisible:%d hasNothingBehind:%d isDirty:%d numTimesUnobserved:%d",
                           unobserved.object->GetID().GetValue(),
                           shouldBeVisible, hasNothingBehind, isDirtyPoseState,
                           unobserved.object->GetNumTimesUnobserved());
          
          ClearObject(unobserved.object);
        }
      }
      else if(unobserved.family != ObjectFamily::Mat &&
              _robot->GetCarryingObjects().count(unobserved.object->GetID()) == 0)
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
                                   _robot->GetLastMsgTimestamp() - unobserved.object->GetLastObservedTime() < seenWithin_sec*1000);
        
        // How far away is the object from our current position? Again, to be
        // conservative, we are only going to use this feature if the object is
        // pretty close to the robot.
        // TODO: Expose / remove / fine-tune this setting
        const f32 distThreshold_mm = -1.f; // 150.f; // Set to <0 to disable
        const bool closeEnough = (distThreshold_mm < 0.f ||
                                  (_robot->GetPose().GetTranslation() -
                                  unobserved.object->GetPose().GetTranslation()).LengthSq() < distThreshold_mm*distThreshold_mm);
        
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
        for(auto & marker : unobserved.object->GetMarkers()) {
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
          _robot->GetVisionComponent().GetCamera().ProjectObject(*unobserved.object, projectedCorners, distance);
          
          if(distance > 0.f) { // in front of camera?
            for(auto & corner : projectedCorners) {
              
              if(camera.IsWithinFieldOfView(corner)) {
                
                BroadcastObjectObservation(unobserved.object, false);
                ++numVisibleObjects;
                
              } // if(IsWithinFieldOfView)
            } // for(each projectedCorner)
          } // if(distance > 0)
        }
      }
      
    } // for each unobserved object
    
    return numVisibleObjects;
  } // CheckForUnobservedObjects()
  
  void BlockWorld::GetObsMarkerList(const PoseKeyObsMarkerMap_t& poseKeyObsMarkerMap,
                                    std::list<Vision::ObservedMarker*>& lst)
  {
    lst.clear();
    for(auto & poseKeyMarkerPair : poseKeyObsMarkerMap)
    {
      lst.push_back((Vision::ObservedMarker*)(&(poseKeyMarkerPair.second)));
    }
  }

  void BlockWorld::RemoveUsedMarkers(PoseKeyObsMarkerMap_t& poseKeyObsMarkerMap)
  {
    for(auto poseKeyMarkerPair = poseKeyObsMarkerMap.begin(); poseKeyMarkerPair != poseKeyObsMarkerMap.end();)
    {
      if (poseKeyMarkerPair->second.IsUsed()) {
        poseKeyMarkerPair = poseKeyObsMarkerMap.erase(poseKeyMarkerPair);
      } else {
        ++poseKeyMarkerPair;
      }
    }
  }

  Result BlockWorld::AddMarkerlessObject(const Pose3d& p)
  {
    TimeStamp_t lastTimestamp = _robot->GetLastMsgTimestamp();

    // Create an instance of the detected object
    auto markerlessObject = std::make_shared<MarkerlessObject>(ObjectType::ProxObstacle);

    // Raise origin of object above ground.
    // NOTE: Assuming detected obstacle is at ground level no matter what angle the head is at.
    Pose3d raiseObject(0, Z_AXIS_3D(), Vec3f(0,0,0.5f*markerlessObject->GetSize().z()));
    Pose3d obsPose = p * raiseObject;
    markerlessObject->SetPose(obsPose);
    markerlessObject->SetPoseParent(_robot->GetPose().GetParent());
    
    // Check if this prox obstacle already exists
    std::vector<ObservableObject*> existingObjects;
    auto originIter = _existingObjects.find(_robot->GetWorldOrigin());
    if(originIter != _existingObjects.end())
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
    FindIntersectingObjects(markerlessObject.get(), existingObjects, 0, filter);
    if (!existingObjects.empty()) {
      return RESULT_OK;
    }

    // HACK: to make it think it was observed enough times so as not to get immediately deleted.
    //       We'll do something better after we figure out how other non-cliff prox obstacles will work.
    for (u8 i=0; i<MIN_TIMES_TO_OBSERVE_OBJECT; ++i) {
      markerlessObject->SetLastObservedTime(lastTimestamp);
    }

    AddNewObject(markerlessObject);
    _didObjectsChange = true;
    _currentObservedObjects.push_back(markerlessObject.get());
    
    return RESULT_OK;
  }

  ObjectID BlockWorld::CreateFixedCustomObject(const Pose3d& p, const f32 xSize_mm, const f32 ySize_mm, const f32 zSize_mm)
  {
    //TODO share common code with AddMarkerlessObject
    TimeStamp_t lastTimestamp = _robot->GetLastMsgTimestamp();
    
    // Create an instance of the custom object
    auto customObject = std::make_shared<CustomObject>(ObjectType::Custom_Fixed, xSize_mm, ySize_mm, zSize_mm);
    
    // Raise origin of object above ground.
    // NOTE: Assuming detected obstacle is at ground level no matter what angle the head is at.
    Pose3d raiseObject(0, Z_AXIS_3D(), Vec3f(0,0,0.5f*customObject->GetSize().z()));
    Pose3d obsPose = p * raiseObject;
    customObject->SetPose(obsPose);
    customObject->SetPoseParent(_robot->GetPose().GetParent());
    
    // HACK: to make it think it was observed enough times so as not to get immediately deleted.
    //       We'll do something better after we figure out how other non-cliff prox obstacles will work.
    for (u8 i=0; i<MIN_TIMES_TO_OBSERVE_OBJECT; ++i) {
      customObject->SetLastObservedTime(lastTimestamp);
    }
    
    AddNewObject(customObject);
    _didObjectsChange = true;
    _currentObservedObjects.push_back(customObject.get());
    
    return customObject->GetID();
  }

  void BlockWorld::GetObstacles(std::vector<std::pair<Quad2f,ObjectID> >& boundingBoxes, const f32 padding) const
  {
    BlockWorldFilter filter;
    filter.SetIgnoreIDs(std::set<ObjectID>(_robot->GetCarryingObjects()));
    
    // If the robot is localized, check to see if it is "on" the mat it is
    // localized to. If so, ignore the mat as an obstacle.
    // Note that the reason for checking IsPoseOn is that it's possible the
    // robot is localized to a mat it sees but is not on because it has not
    // yet seen the mat it is on. (For example, robot see side of platform
    // and localizes to it because it hasn't seen a marker on the flat mat
    // it is driving on.)
    if(_robot->GetLocalizedTo().IsSet())
    {
      const ObservableObject* object = GetObjectByID(_robot->GetLocalizedTo(), ObjectFamily::Mat);
      if(nullptr != object) // If the object localized to exists in the Mat family
      {
        const MatPiece* mat = dynamic_cast<const MatPiece*>(object);
        if(mat != nullptr) {
          if(mat->IsPoseOn(_robot->GetPose(), 0.f, .25*ROBOT_BOUNDING_Z)) {
            // Ignore the ID of the mat we're on
            filter.AddIgnoreID(_robot->GetLocalizedTo());
            
            // Add any "unsafe" regions this mat has
            mat->GetUnsafeRegions(boundingBoxes, padding);
          }
        } else {
          PRINT_NAMED_WARNING("BlockWorld.GetObstacles.DynamicCastFail",
                              "Could not dynamic cast localization object %d to a Mat",
                              _robot->GetLocalizedTo().GetValue());
        }
      }
    }
    
    // Figure out height filters in world coordinates (because GetObjectBoundingBoxesXY()
    // uses heights of objects in world coordinates)
    const Pose3d robotPoseWrtOrigin = _robot->GetPose().GetWithRespectToOrigin();
    const f32 minHeight = robotPoseWrtOrigin.GetTranslation().z();
    const f32 maxHeight = minHeight + _robot->GetHeight();
    
    GetObjectBoundingBoxesXY(minHeight, maxHeight, padding, boundingBoxes, filter);
    
  } // GetObstacles()
  
  void BlockWorld::FindMatchingObjects(const BlockWorldFilter& filter, std::vector<ObservableObject*>& result)
  {
    // slight abuse of the FindObjectHelper, I just use it for filtering, then I add everything that passes
    // the filter to the result vector
    ModifierFcn addToResult = [&result](ObservableObject* candidateObject) {
      result.push_back(candidateObject);
    };
    
    // ignore return value, since the findLambda stored everything in result
    FindObjectHelper(filter, addToResult, false);
  }
  
  void BlockWorld::FindMatchingObjects(const BlockWorldFilter& filter, std::vector<const ObservableObject*>& result) const
  {
    // slight abuse of the FindObjectHelper, I just use it for filtering, then I add everything that passes
    // the filter to the result vector
    ModifierFcn addToResult = [&result](ObservableObject* candidateObject) {
      result.push_back(candidateObject);
    };
    
    // ignore return value, since the findLambda stored everything in result
    FindObjectHelper(filter, addToResult, false);
  }

  void BlockWorld::GetObjectBoundingBoxesXY(const f32 minHeight,
                                            const f32 maxHeight,
                                            const f32 padding,
                                            std::vector<std::pair<Quad2f,ObjectID> >& rectangles,
                                            const BlockWorldFilter& filterIn) const
  {
    BlockWorldFilter filter(filterIn);
    filter.AddFilterFcn([minHeight, maxHeight, padding, &rectangles](const ObservableObject* object)
    {
      if(object == nullptr) {
        PRINT_NAMED_WARNING("BlockWorld.GetObjectBoundingBoxesXY.NullObjectPointer",
                            "ObjectID %d corresponds to NULL ObservableObject pointer.",
                            object->GetID().GetValue());
      } else if(object->GetNumTimesObserved() >= MIN_TIMES_TO_OBSERVE_OBJECT
                && !object->IsPoseStateUnknown()) {
        const f32 objectHeight = object->GetPose().GetWithRespectToOrigin().GetTranslation().z();
        if( (objectHeight >= minHeight) && (objectHeight <= maxHeight) )
        {
          rectangles.emplace_back(object->GetBoundingQuadXY(padding), object->GetID());
          return true;
        }
      }
      return false;
    });
    
    FindObjectHelper(filter);
    
  } // GetObjectBoundingBoxesXY()
  
  
  bool BlockWorld::DidObjectsChange() const {
    return _didObjectsChange;
  }

  
  bool BlockWorld::UpdateRobotPose(PoseKeyObsMarkerMap_t& obsMarkersAtTimestamp, const TimeStamp_t atTimestamp)
  {
    bool wasPoseUpdated = false;
    
    // Extract only observed markers from obsMarkersAtTimestamp
    std::list<Vision::ObservedMarker*> obsMarkersListAtTimestamp;
    GetObsMarkerList(obsMarkersAtTimestamp, obsMarkersListAtTimestamp);
    
    // Get all mat objects *seen by this robot's camera*
    std::multimap<f32, ObservableObject*> matsSeen;
    _objectLibrary[ObjectFamily::Mat].CreateObjectsFromMarkers(obsMarkersListAtTimestamp, matsSeen,
                                                                (_robot->GetVisionComponent().GetCamera().GetID()));

    // Remove used markers from map container
    RemoveUsedMarkers(obsMarkersAtTimestamp);
    
    if(not matsSeen.empty()) {
      /*
      // TODO: False mat marker localization causes the mat to be created in weird places which is messing up game dev.
      //       Seems to happen particular when looking at the number 1. Disabled for now.
      PRINT_NAMED_WARNING("UpdateRobotPose.TempIgnore", "Ignoring mat marker. Robot localization disabled.");
      return false;
      */
      
      // Is the robot "on" any of the mats it sees?
      // TODO: What to do if robot is "on" more than one mat simultaneously?
      MatPiece* onMat = nullptr;
      for(const auto& objectPair : matsSeen) {
        Vision::ObservableObject* object = objectPair.second;
        
        // ObservedObjects are w.r.t. the arbitrary historical origin of the camera
        // that observed them.  Hook them up to the current robot origin now:
        CORETECH_ASSERT(object->GetPose().GetParent() != nullptr &&
                        object->GetPose().GetParent()->IsOrigin());
        object->SetPoseParent(_robot->GetWorldOrigin());
        
        MatPiece* mat = dynamic_cast<MatPiece*>(object);
        CORETECH_ASSERT(mat != nullptr);
        
        // Does this mat pose make sense? I.e., is the top surface flat enough
        // that we could drive on it?
        Vec3f rotAxis;
        Radians rotAngle;
        mat->GetPose().GetRotationVector().GetAngleAndAxis(rotAngle, rotAxis);
        if(std::abs(rotAngle.ToFloat()) > DEG_TO_RAD(5) &&                // There's any rotation to speak of
           !AreUnitVectorsAligned(rotAxis, Z_AXIS_3D(), DEG_TO_RAD(45)))  // That rotation's axis more than 45 degrees from vertical
        {
          PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose",
                           "Refusing to localize to %s mat with rotation %.1f degrees around (%.1f,%.1f,%.1f) axis.",
                           ObjectTypeToString(mat->GetType()),
                           rotAngle.getDegrees(),
                           rotAxis.x(), rotAxis.y(), rotAxis.z());
        }else if(mat->IsPoseOn(_robot->GetPose(), 0, 15.f)) { // TODO: get heightTol from robot
          if(onMat != nullptr) {
            PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.OnMultiplMats",
                                "Robot is 'on' multiple mats at the same time. Will just use the first for now.");
          } else {
            onMat = mat;
          }
        }
      }
      
      // This will point to the mat we decide to localize to below (or will
      // remain null if we choose not to localize to any mat we see)
      MatPiece* matToLocalizeTo = nullptr;
      
      if(onMat != nullptr)
      {
        
        PRINT_LOCALIZATION_INFO("BlockWorld.UpdateRobotPose.OnMatLocalization",
                                "Robot %d is on a %s mat and will localize to it.",
                                _robot->GetID(), onMat->GetType().GetName().c_str());
        
        // If robot is "on" one of the mats it is currently seeing, localize
        // the robot to that mat
        matToLocalizeTo = onMat;
      }
      else {
        // If the robot is NOT "on" any of the mats it is seeing...
        
        if(_robot->GetLocalizedTo().IsSet()) {
          // ... and the robot is already localized, then see if it is
          // localized to one of the mats it is seeing (but not "on")
          // Note that we must match seen and existing objects by their pose
          // here, and not by ID, because "seen" objects have not ID assigned
          // yet.

          ObservableObject* existingMatLocalizedTo = GetObjectByID(_robot->GetLocalizedTo());
          if(existingMatLocalizedTo == nullptr) {
            PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.ExistingMatLocalizedToNull",
                              "Robot %d is localized to mat with ID=%d, but that mat does not exist in the world.",
                              _robot->GetID(), _robot->GetLocalizedTo().GetValue());
            return false;
          }
          
          std::vector<ObservableObject*> overlappingMatsSeen;
          FindOverlappingObjects(existingMatLocalizedTo, matsSeen, overlappingMatsSeen);
          
          if(overlappingMatsSeen.empty()) {
            // The robot is localized to a mat it is not seeing (and is not "on"
            // any of the mats it _is_ seeing.  Just update the poses of the
            // mats it is seeing, but don't localize to any of them.
            PRINT_LOCALIZATION_INFO("BlockWorld.UpdateRobotPose.NotOnMatNoLocalize",
                                    "Robot %d is localized to a mat it doesn't see, and will not localize to any of the %lu mats it sees but is not on.",
                                    _robot->GetID(), (unsigned long)matsSeen.size());
          }
          else {
            if(overlappingMatsSeen.size() > 1) {
              PRINT_STREAM_WARNING("BlockWorld.UpdateRobotPose.MultipleOverlappingMats",
                                  "Robot " << _robot->GetID() << " is seeing " << overlappingMatsSeen.size() << " (i.e. more than one) mats "
                                  "overlapping with the existing mat it is localized to. "
                                  "Will use first.");
            }
            
            PRINT_LOCALIZATION_INFO("BlockWorld.UpdateRobotPose.NotOnMatLocalization",
                                    "Robot %d will re-localize to the %s mat it is not on, but already localized to.",
                                    _robot->GetID(), overlappingMatsSeen[0]->GetType().GetName().c_str());
            
            // The robot is localized to one of the mats it is seeing, even
            // though it is not _on_ that mat.  Remain localized to that mat
            // and update any others it is also seeing
            matToLocalizeTo = dynamic_cast<MatPiece*>(overlappingMatsSeen[0]);
            CORETECH_ASSERT(matToLocalizeTo != nullptr);
          }
          
          
        } else {
          // ... and the robot is _not_ localized, choose the observed mat
          // with the closest observed marker (since that is likely to be the
          // most accurate) and localize to that one.
          f32 minDistSq = -1.f;
          MatPiece* closestMat = nullptr;
          for(const auto& matPair : matsSeen) {
            Vision::ObservableObject* mat = matPair.second;
            
            std::vector<const Vision::KnownMarker*> observedMarkers;
            mat->GetObservedMarkers(observedMarkers, atTimestamp);
            if(observedMarkers.empty()) {
              PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.ObservedMatWithNoObservedMarkers",
                                "We saw a mat piece but it is returning no observed markers for "
                                "the current timestamp.");
              CORETECH_ASSERT(false); // TODO: handle this situation
            }
            
            Pose3d markerWrtRobot;
            for(auto obsMarker : observedMarkers) {
              if(obsMarker->GetPose().GetWithRespectTo(_robot->GetPose(), markerWrtRobot) == false) {
                PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.ObsMarkerPoseOriginMisMatch",
                                  "Could not get the pose of an observed marker w.r.t. the robot that "
                                  "supposedly observed it.");
                CORETECH_ASSERT(false); // TODO: handle this situation
              }
              
              const f32 markerDistSq = markerWrtRobot.GetTranslation().LengthSq();
              if(closestMat == nullptr || markerDistSq < minDistSq) {
                closestMat = dynamic_cast<MatPiece*>(mat);
                CORETECH_ASSERT(closestMat != nullptr);
                minDistSq = markerDistSq;
              }
            } // for each observed marker
          } // for each mat seen
          
          PRINT_LOCALIZATION_INFO("BLockWorld.UpdateRobotPose.NotOnMatLocalizationToClosest",
                                  "Robot %d is not on a mat but will localize to %s mat ID=%d, which is the closest.",
                                  _robot->GetID(), closestMat->GetType().GetName().c_str(), closestMat->GetID().GetValue());
          
          matToLocalizeTo = closestMat;
          
        } // if/else robot is localized
      } // if/else (onMat != nullptr)
      
      ObjectsMapByType_t& existingMatPieces = _existingObjects[_robot->GetWorldOrigin()][ObjectFamily::Mat];
      
      // Keep track of markers we saw on existing/instantiated mats, to use
      // for occlusion checking
      std::vector<const Vision::KnownMarker *> observedMarkers;

      std::shared_ptr<MatPiece> existingMatPiece = nullptr;
      
      // If we found a suitable mat to localize to, and we've seen it enough
      // times, then use it for localizing
      if(matToLocalizeTo != nullptr) {
        
        if(existingMatPieces.empty()) {
          // If this is the first mat piece, add it to the world using the world
          // origin as its pose
          PRINT_STREAM_INFO("BlockWorld.UpdateRobotPose.CreatingFirstMatPiece",
                     "Instantiating first mat piece in the world.");
          
          existingMatPiece.reset( dynamic_cast<MatPiece*>(matToLocalizeTo->CloneType()) );
          assert(existingMatPiece != nullptr);
          AddNewObject(existingMatPieces, existingMatPiece);
          
          existingMatPiece->SetPose( Pose3d() ); // Not really necessary, but ensures the ID makes it into the pose name, which is helpful for debugging
          assert(existingMatPiece->GetPose().GetParent() == nullptr);
          
        }
        else {
          // We can't look up the existing piece by ID because the matToLocalizeTo
          // is just a mat we _saw_, not one we've instantiated.  So look for
          // one in approximately the same position, of those with the same
          // type:
          //ObservableObject* existingObject = GetObjectByID(matToLocalizeTo->GetID());
          std::vector<ObservableObject*> existingObjects;
          FindOverlappingObjects(matToLocalizeTo, existingMatPieces, existingObjects);
        
          if(existingObjects.empty())
          {
            // If the mat we are about to localize to does not exist yet,
            // but it's not the first mat piece in the world, add it to the
            // world, and give it a new origin, relative to the current
            // world origin.
            Pose3d poseWrtWorldOrigin = matToLocalizeTo->GetPose().GetWithRespectToOrigin();
            
            existingMatPiece.reset( dynamic_cast<MatPiece*>(matToLocalizeTo->CloneType()) );
            assert(existingMatPiece != nullptr);
            AddNewObject(existingMatPieces, existingMatPiece);
            existingMatPiece->SetPose(poseWrtWorldOrigin); // Do after AddNewObject, once ID is set
            
            PRINT_STREAM_INFO("BlockWorld.UpdateRobotPose.LocalizingToNewMat",
                       "Robot " << _robot->GetID() << " localizing to new "
                              << ObjectTypeToString(existingMatPiece->GetType()) << " mat with ID=" << existingMatPiece->GetID().GetValue() << ".");
            
          } else {
            if(existingObjects.size() > 1) {
              PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.MultipleExistingObjectMatches",
                            "Robot %d found multiple existing mats matching the one it "
                            "will localize to - using first.", _robot->GetID());
            }
            
            // We are localizing to an existing mat piece: do not attempt to
            // update its pose (we can't both update the mat's pose and use it
            // to update the robot's pose at the same time!)
            existingMatPiece.reset( dynamic_cast<MatPiece*>(existingObjects.front()) );
            CORETECH_ASSERT(existingMatPiece != nullptr);
            
            PRINT_LOCALIZATION_INFO("BlockWorld.UpdateRobotPose.LocalizingToExistingMat",
                                    "Robot %d localizing to existing %s mat with ID=%d.",
                                    _robot->GetID(), existingMatPiece->GetType().GetName().c_str(),
                                    existingMatPiece->GetID().GetValue());
          }
        } // if/else (existingMatPieces.empty())
        
        existingMatPiece->SetLastObservedTime(matToLocalizeTo->GetLastObservedTime());
        existingMatPiece->UpdateMarkerObservationTimes(*matToLocalizeTo);
        existingMatPiece->GetObservedMarkers(observedMarkers, atTimestamp);
        
        if(existingMatPiece->GetNumTimesObserved() >= MIN_TIMES_TO_OBSERVE_OBJECT) {
          // Now localize to that mat
          //wasPoseUpdated = LocalizeRobotToMat(robot, matToLocalizeTo, existingMatPiece);
          if(_robot->LocalizeToMat(matToLocalizeTo, existingMatPiece.get()) == RESULT_OK) {
            wasPoseUpdated = true;
          }
        }
        
      } // if(matToLocalizeTo != nullptr)
      
      // Update poses of any other mats we saw (but did not localize to),
      // just like they are any "regular" object, unless that mat is the
      // robot's current "world" origin, [TODO:] in which case we will update the pose
      // of the mat we are on w.r.t. that world.
      for(const auto& matSeenPair : matsSeen) {
        ObservableObject* matSeen = matSeenPair.second;
        
        if(matSeen != matToLocalizeTo) {
          
          // TODO: Make this w.r.t. whatever the robot is currently localized to?
          Pose3d poseWrtOrigin = matSeen->GetPose().GetWithRespectToOrigin();
          
          // Does this mat pose make sense? I.e., is the top surface flat enough
          // that we could drive on it?
          Vec3f rotAxis;
          Radians rotAngle;
          poseWrtOrigin.GetRotationVector().GetAngleAndAxis(rotAngle, rotAxis);
          if(std::abs(rotAngle.ToFloat()) > DEG_TO_RAD(5) &&                // There's any rotation to speak of
             !AreUnitVectorsAligned(rotAxis, Z_AXIS_3D(), DEG_TO_RAD(45)))  // That rotation's axis more than 45 degrees from vertical
          {
            PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose",
                             "Ignoring observation of %s mat with rotation %.1f degrees around (%.1f,%.1f,%.1f) axis.",
                             ObjectTypeToString(matSeen->GetType()),
                             rotAngle.getDegrees(),
                             rotAxis.x(), rotAxis.y(), rotAxis.z());
            continue;
          }
          
          // Store pointers to any existing objects that overlap with this one
          std::vector<ObservableObject*> overlappingObjects;
          FindOverlappingObjects(matSeen, existingMatPieces, overlappingObjects);
          
          if(overlappingObjects.empty()) {
            // no existing mats overlapped with the mat we saw, so add it
            // as a new mat piece, relative to the world origin
            std::shared_ptr<ObservableObject> newMatPiece(matSeen->CloneType());
            AddNewObject(existingMatPieces, newMatPiece);
            newMatPiece->SetPose(poseWrtOrigin); // do after AddNewObject, once ID is set
            
            // TODO: Make clone copy the observation times
            newMatPiece->SetLastObservedTime(matSeen->GetLastObservedTime());
            newMatPiece->UpdateMarkerObservationTimes(*matSeen);
            
            PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose",
                             "Adding new %s mat with ID=%d at (%.1f, %.1f, %.1f)",
                             ObjectTypeToString(newMatPiece->GetType()),
                             newMatPiece->GetID().GetValue(),
                             newMatPiece->GetPose().GetTranslation().x(),
                             newMatPiece->GetPose().GetTranslation().y(),
                             newMatPiece->GetPose().GetTranslation().z());
            
            // Add observed mat markers to the occlusion map of the camera that saw
            // them, so we can use them to delete objects that should have been
            // seen between that marker and the robot
            newMatPiece->GetObservedMarkers(observedMarkers, atTimestamp);

          }
          else {
            if(overlappingObjects.size() > 1) {
              PRINT_LOCALIZATION_INFO("BlockWorld.UpdateRobotPose",
                                      "More than one overlapping mat found -- will use first.");
              // TODO: do something smarter here?
            }
            
            if(&(overlappingObjects[0]->GetPose()) != _robot->GetWorldOrigin()) {
              // The overlapping mat object is NOT the world origin mat, whose
              // pose we don't want to update.
              // Update existing observed mat we saw but are not on w.r.t.
              // the robot's current world origin
              
              // TODO: better way of merging existing/observed object pose
              overlappingObjects[0]->SetPose( poseWrtOrigin );
              
            } else {
              /* PUNT - not sure this is workign, nor we want to bother with this for now...
              CORETECH_ASSERT(robot->IsLocalized());
              
              // Find the mat the robot is currently localized to
              MatPiece* localizedToMat = nullptr;
              for(auto & objectsByType : existingMatPieces) {
                auto objectsByIdIter = objectsByType.second.find(robot->GetLocalizedTo());
                if(objectsByIdIter != objectsByType.second.end()) {
                  localizedToMat = dynamic_cast<MatPiece*>(objectsByIdIter->second);
                }
              }

              CORETECH_ASSERT(localizedToMat != nullptr);
              
              // Update the mat we are localized to (but may not have seen) w.r.t. the existing
              // observed world origin mat we did see from it.  This should in turn
              // update the pose of everything on that mat.
              Pose3d newPose;
              if(localizedToMat->GetPose().GetWithRespectTo(matSeen->GetPose(), newPose) == false) {
                PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.FailedToUpdateWrtObservedOrigin",
                                  "Robot %d failed to get pose of existing %s mat it is on w.r.t. observed world origin mat.",
                                  robot->GetID(), existingMatPiece->GetType().GetName().c_str());
              }
              newPose.SetParent(robot->GetWorldOrigin());
              // TODO: Switch new pose to be w.r.t. whatever robot is localized to??
              localizedToMat->SetPose(newPose);
               */
            }
            
            overlappingObjects[0]->SetLastObservedTime(matSeen->GetLastObservedTime());
            overlappingObjects[0]->UpdateMarkerObservationTimes(*matSeen);
            overlappingObjects[0]->GetObservedMarkers(observedMarkers, atTimestamp);
            
          } // if/else overlapping existing mats found
        } // if matSeen != matToLocalizeTo
        
        delete matSeen;
      }
      
      // Add observed mat markers to the occlusion map of the camera that saw
      // them, so we can use them to delete objects that should have been
      // seen between that marker and the robot
      for(auto obsMarker : observedMarkers) {
        /*
        PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.AddingMatMarkerOccluder",
                         "Adding mat marker '%s' as an occluder for robot %d.",
                         Vision::MarkerTypeStrings[obsMarker->GetCode()],
                         robot->GetID());
         */
        _robot->GetVisionComponent().GetCamera().AddOccluder(*obsMarker);
      }
      
      /* Always re-drawing everything now
      // If the robot just re-localized, trigger a draw of all objects, since
      // we may have seen things while de-localized whose locations can now be
      // snapped into place.
      if(!wasLocalized && robot->IsLocalized()) {
        PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.RobotRelocalized",
                         "Robot %d just localized after being de-localized.", robot->GetID());
        DrawAllObjects();
      }
      */
    } // IF any mat piece was seen

    if(wasPoseUpdated) {
      PRINT_LOCALIZATION_INFO("BlockWorld.UpdateRobotPose.RobotPoseChain", "%s",
                              _robot->GetPose().GetNamedPathToOrigin(true).c_str());
    }
    
    return wasPoseUpdated;
    
  } // UpdateRobotPose()
  
  Result BlockWorld::UpdateObjectPoses(PoseKeyObsMarkerMap_t& obsMarkersAtTimestamp,
                                       const ObjectFamily& inFamily,
                                       const TimeStamp_t atTimestamp)
  {
    // Sanity checks for robot's origin
    ASSERT_NAMED(_robot->GetPose().GetParent() == _robot->GetWorldOrigin(),
                 "BlockWorld.UpdateObjectPoses.RobotParentShouldBeOrigin");
    ASSERT_NAMED(&_robot->GetPose().FindOrigin() == _robot->GetWorldOrigin(),
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
    if(not _obsMarkers.empty()) {
      
      // Extract only observed markers from obsMarkersAtTimestamp
      std::list<Vision::ObservedMarker*> obsMarkersListAtTimestamp;
      GetObsMarkerList(obsMarkersAtTimestamp, obsMarkersListAtTimestamp);
      
      objectLibrary.CreateObjectsFromMarkers(obsMarkersListAtTimestamp, objectsSeen);
      
      // Remove used markers from map
      RemoveUsedMarkers(obsMarkersAtTimestamp);
    
      for(const auto& objectPair : objectsSeen) {
        Vision::ObservableObject* object = objectPair.second;
        
        // ObservedObjects should be directly w.r.t. an origin by now, and that
        // origin should be the robot's. (We should not have queued markers that
        // did not meet that criteria.)
        ASSERT_NAMED(object->GetPose().GetParent() != nullptr &&
                     object->GetPose().GetParent()->IsOrigin(),
                     "BlockWorld.UpdateObjectPoses.ObservedObjectParentNotAnOrigin");
        ASSERT_NAMED(object->GetPose().GetParent() == _robot->GetWorldOrigin(),
                     "BlockWorld.UpdateObjectPoses.ObservedObjectParentNotRobotOrigin");

        // we are creating a quad projected on the ground from the robot to every marker we see. In order to do so
        // the bottom corners of the ground quad match the forward ones of the robot (robotQuad::xLeft). The names
        // cornerBR, cornerBL are the corners in the ground quads (BottomRight and BottomLeft).
        // Later on we will generate the top corners for the ground quad (cornerTL, corner TR)
        const Quad2f& robotQuad = _robot->GetBoundingQuadXY(_robot->GetPose().GetWithRespectToOrigin());
        Point3f cornerBR{ robotQuad[Quad::TopLeft   ].x(), robotQuad[Quad::TopLeft ].y(), 0};
        Point3f cornerBL{ robotQuad[Quad::BottomLeft].x(), robotQuad[Quad::BottomLeft].y(), 0};
      
        INavMemoryMap* currentNavMemoryMap = GetNavMemoryMap();
        
        std::vector<const Vision::KnownMarker *> observedMarkers;
        object->GetObservedMarkers(observedMarkers);
        for ( const auto& observedMarkerIt : observedMarkers )
        {
          // An observerd marker. Assign the marker's bottom corners as the top corners for the ground quad
          // The names of the corners (cornerTL and cornerTR) are those of the ground quad: TopLeft and TopRight
          ASSERT_NAMED(_robot->GetWorldOrigin() == &observedMarkerIt->GetPose().FindOrigin(),
                       "BlockWorld.UpdateObjectPoses.MarkerOriginShouldBeRobotOrigin");
          const Quad3f& markerCorners = observedMarkerIt->Get3dCorners(observedMarkerIt->GetPose().GetWithRespectToOrigin());
          Point3f cornerTL = markerCorners[Quad::BottomLeft];
          Point3f cornerTR = markerCorners[Quad::BottomRight];
          
          // Create a quad between the bottom corners of a marker and the robot forward corners, and tell
          // the navmesh that it should be clear, since we saw the marker
          Quad2f clearVisionQuad { cornerTL, cornerBL, cornerTR, cornerBR };
          
          // update navmesh with a quadrilateral between the robot and the seen object
          if ( nullptr != currentNavMemoryMap ) {
            currentNavMemoryMap->AddQuad(clearVisionQuad, INavMemoryMap::EContentType::ClearOfObstacle);
          }
          
          // also notify behavior whiteboard.
          // rsam: should this information be in the map instead of the whiteboard? It seems a stretch that
          // blockworld knows now about behaviors, maybe all this processing of quads should be done in a separate
          // robot component, like a VisualInformationProcessingComponent
          _robot->GetBehaviorManager().GetWhiteboard().ProcessClearQuad(clearVisionQuad);
        }
        
      }
      
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
        FindOverlappingObjects(m, _existingObjects[ObjectFamily::MarkerlessObject], existingObjects);
        
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
      }
    } // end for all prox sensors
    
    // Delete any existing prox objects that are too old.
    // Note that we use find() here because there may not be any markerless objects
    // yet, and using [] indexing will create things.
    auto markerlessFamily = _existingObjects.find(ObjectFamily::MarkerlessObject);
    if(markerlessFamily != _existingObjects.end())
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


  ObjectID BlockWorld::AddActiveObject(ActiveID activeID,
                                       FactoryID factoryID,
                                       ActiveObjectType activeObjectType,
                                       const ObservableObject* objToCopyId)
  {
    if (activeID >= (int)ActiveObjectConstants::MAX_NUM_ACTIVE_OBJECTS) {
      PRINT_NAMED_WARNING("BlockWorld.AddActiveObject.InvalidActiveID", "activeID %d", activeID);
      return ObjectID();
    }
    
    if (activeID < 0)
    {
      PRINT_NAMED_INFO("BlockWorld.AddActiveObject","Adding object with negative ActiveID %d FactoryID %d", activeID, factoryID);
    }
    
    // Is there an active object with the same activeID that already exists?
    ObjectType objType = ActiveObject::GetTypeFromActiveObjectType(activeObjectType);
    const char* objTypeStr = EnumToString(objType);
    ActiveObject* matchingObject = GetActiveObjectByActiveID(activeID);
    if (matchingObject == nullptr) {
      // If no match found, find one of the same type with an invalid activeID and assume it's that
      const ObjectsMapByID_t& objectsOfSameType = GetExistingObjectsByType(objType);
      for (auto& objIt : objectsOfSameType) {
        ObservableObject* sameTypeObject = objIt.second.get();
        if (sameTypeObject->GetActiveID() < 0) {
          sameTypeObject->SetActiveID(activeID);
          sameTypeObject->SetFactoryID(factoryID);
          PRINT_NAMED_INFO("BlockWorld.AddActiveObject.FoundMatchingObjectWithNoActiveID",
                           "objectID %d, activeID %d, type %s",
                           sameTypeObject->GetID().GetValue(), sameTypeObject->GetActiveID(), objTypeStr);
          return sameTypeObject->GetID();
        } else {
          // If found an existing object of the same type but not same factoryID then ignore it
          // until we figure out how to deal with multiple objects of same type.
          if ( sameTypeObject->GetFactoryID() != factoryID ) {
            PRINT_NAMED_WARNING("BlockWorld.AddActiveObject.FoundOtherActiveObjectOfSameType",
                                "ActiveID %d (factoryID 0x%x) is same type as another existing object (objectID %d, activeID %d, factoryID 0x%x, type %s). Multiple objects of same type not supported!",
                                activeID, factoryID,
                                sameTypeObject->GetID().GetValue(), sameTypeObject->GetActiveID(), sameTypeObject->GetFactoryID(), objTypeStr);
            return ObjectID();
          } else {
            PRINT_NAMED_INFO("BlockWorld.AddActiveObject.FoundIdenticalObjectOnDifferentSlot",
                             "Updating activeID of block with factoryID 0x%x from %d to %d",
                             sameTypeObject->GetFactoryID(), sameTypeObject->GetActiveID(), activeID);
            sameTypeObject->SetActiveID(activeID);
            return sameTypeObject->GetID();
          }
        }
      }
    } else {
      // A match was found but does it have the same factory ID?
      if(matchingObject->GetFactoryID() == factoryID) {
        PRINT_NAMED_INFO("BlockWorld.AddActiveObject.FoundMatchingActiveObject",
                         "objectID %d, activeID %d, type %s, factoryID 0x%x",
                         matchingObject->GetID().GetValue(), matchingObject->GetActiveID(), objTypeStr, matchingObject->GetFactoryID());
        return matchingObject->GetID();
      } else if (matchingObject->GetFactoryID() == 0) {
        // Existing object was only previously observed, never connected, so its factoryID is 0
        PRINT_NAMED_WARNING("BlockWorld.AddActiveObject.FoundMatchingActiveObjectThatWasNeverConnected",
                         "objectID %d, activeID %d, type %s, factoryID 0x%x",
                         matchingObject->GetID().GetValue(), matchingObject->GetActiveID(), objTypeStr, matchingObject->GetFactoryID());
        return matchingObject->GetID();
      } else {
        // FactoryID mismatch. Delete the current object and fall through to add a new one
        PRINT_NAMED_WARNING("BlockWorld.AddActiveObject.MismatchedFactoryID",
                            "objectID %d, activeID %d, type %s, factoryID 0x%x (expected 0x%x)",
                            matchingObject->GetID().GetValue(), matchingObject->GetActiveID(), objTypeStr, factoryID, matchingObject->GetFactoryID());
        DeleteObject(matchingObject->GetID());
      }
    }

    // An existing object with activeID was not found so add it
    ObservableObject* newObjectPtr = nullptr;
    switch(objType) {
      case ObjectType::Block_LIGHTCUBE1:
      case ObjectType::Block_LIGHTCUBE2:
      case ObjectType::Block_LIGHTCUBE3:
      {
        newObjectPtr = new ActiveCube(activeID, factoryID, activeObjectType);
        break;
      }
      case ObjectType::Charger_Basic:
      {
        newObjectPtr = new Charger(activeID, factoryID, activeObjectType);
        break;
      }
      default:
        PRINT_NAMED_WARNING("BlockWorld.AddActiveObject.UnsupportedActiveObjectType",
                            "%s (ActiveObjectType: 0x%hx)", objTypeStr, activeObjectType);
        return ObjectID();
    }
    
    std::shared_ptr<ObservableObject> newObject(newObjectPtr);
    newObject->SetPoseParent(_robot->GetWorldOrigin());
    newObject->SetPoseState(PoseState::Unknown);
    AddNewObject(newObject);
    PRINT_NAMED_INFO("BlockWorld.AddActiveObject.AddedNewObject",
                     "objectID %d, type %s, activeID %d, factoryID 0x%x",
                     newObject->GetID().GetValue(), objTypeStr, newObject->GetActiveID(), newObject->GetFactoryID());
    
    if(objToCopyId != nullptr)
    {
      newObject->CopyID(objToCopyId);
    }
    return newObject->GetID();
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::OnObjectPoseChanged(const ObjectID& objectID, ObjectFamily family,
    const Pose3d& oldPose, PoseState oldPoseState,
    const Pose3d& newPose, PoseState newPoseState)
  {
    ASSERT_NAMED(objectID.IsSet(), "BlockWorld.OnObjectPoseChanged.InvalidObjectID");

    // find the object
    const ObservableObject* object = GetObjectByID(objectID, family);
    if ( nullptr == object )
    {
      PRINT_CH_INFO("BlockWorld",
        "BlockWorld.OnObjectPoseChanged.NotAnObject",
        "Could not find object ID '%d' in BlockWorld", objectID.GetValue() );
      return;
    }

    // get current memory map
    INavMemoryMap* const currentNavMemoryMap = GetNavMemoryMap();
    if( nullptr == currentNavMemoryMap ) {
      return;
    }
    
    // ---- Remove from old if it was not unknown
    if ( oldPoseState != PoseState::Unknown )
    {
      // get the type we use for removal
      const NavMemoryMapTypes::EContentType removalContent = ObjectFamilyToMemoryMapContentType(family, false);
      if ( removalContent != NavMemoryMapTypes::EContentType::Unknown )
      {
        // Related to COZMO-3383: Consider removing cube quads from maps not in current origin
        // get old bounding quad wrt current origin
        Pose3d oldPoseWrtCurrentWorld;
        if ( oldPose.GetWithRespectTo(*_robot->GetWorldOrigin(), oldPoseWrtCurrentWorld) )
        {
          const Quad2f& newQuad = object->GetBoundingQuadXY(oldPoseWrtCurrentWorld);
          currentNavMemoryMap->AddQuad(newQuad, removalContent);
        }
        else
        {
          // until COZMO-3383 is done, it should be ok to find an old pose in another origin
//          PRINT_NAMED_WARNING("BlockWorld.OnObjectPoseChanged.InvalidNewPose",
//            "New pose not valid in this world for ID %d",
//            objectID.GetValue());
        }
      } else {
        PRINT_CH_INFO("BlockWorld",
          "BlockWorld.OnObjectPoseChanged.NonMapFamilyForRemoval",
          "Family '%s' does not have a removal type in memory map", ObjectFamilyToString(family) );
      }
    }
    
    // ---- Add to new if new pose is not unknown
    if ( newPoseState != PoseState::Unknown )
    {
      // get the type we use for addition
      const NavMemoryMapTypes::EContentType addedContent = ObjectFamilyToMemoryMapContentType(family, true);
      if ( addedContent != NavMemoryMapTypes::EContentType::Unknown )
      {
        // get new bounding quad wrt current origin
        Pose3d newPoseWrtWorld;
        if ( newPose.GetWithRespectTo(*_robot->GetWorldOrigin(), newPoseWrtWorld) )
        {
          const Quad2f& newQuad = object->GetBoundingQuadXY(newPoseWrtWorld);
          currentNavMemoryMap->AddQuad(newQuad, addedContent);
          
          
          // since we added an obstacle, any borders we saw while dropping it should not be interesting
          const float kScaledQuadToIncludeEdges = 2.0f;
          // kScaledQuadToIncludeEdges: the resulting edge quad should include the interesting edges that
          // are susceptible of being filled as not interesting. In other words: because we want to
          // consider interesting edges around this obstacle, to see if we want them to be flagged as non-interesting,
          // the quad to search for these edges has to be equal to the obstacle quad plus the margin
          // in which we would find edges. For example, a good tight limit would be the size of the smallest
          // quad in the memory map, since edges should be adjacent to the cube. This quad however is merely
          // to limit the search for interesting edges, so it being bigger than the tightest threshold it's only a
          // negligible performance hit (since the current quad tree processor caches nodes anyway for faster processing)
          const Quad2f& edgeQuad = newQuad.GetScaled(kScaledQuadToIncludeEdges);
          ReviewInterestingEdges(edgeQuad);
        }
        else
        {
          // how is this a known or dirty pose in another origin?
          PRINT_NAMED_WARNING("BlockWorld.OnObjectPoseChanged.InvalidNewPose",
            "New pose not valid in this world for ID %d",
            objectID.GetValue());
        }
      } else {
        PRINT_CH_INFO("BlockWorld",
          "BlockWorld.OnObjectPoseChanged.NonMapFamilyForAddition",
          "Family '%s' is not known in memory map", ObjectFamilyToString(family) );
      }
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool BlockWorld::IsZombiePoseOrigin(const Pose3d* origin) const
  {
    // really, pass in an origin
    ASSERT_NAMED(nullptr != origin && origin->IsOrigin(), "BlockWorld.IsZombiePoseOrigin.NotAnOrigin");
  
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

  Result BlockWorld::AddCliff(const Pose3d& p)
  {
    // at the moment we treat them as markerless objects
    const Result ret = AddMarkerlessObject(p);
    return ret;
  }

  Result BlockWorld::AddProxObstacle(const Pose3d& p)
  {
    // add markerless object
    const Result ret = AddMarkerlessObject(p);
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
        ASSERT_NAMED(false, "ProcessVisionOverheadEdges.ValidPlaneWithNoChains");
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
    // Note: Not using withinQuad, but should. I plan on using once the memory map allows local queries and
    // modifications. Leave here for legacy purposes. We surely enable it soon, because performance needs
    // improvement

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
      static_assert(NavMemoryMapTypes::ContentValueEntry::IsValidArray(typesWhoseEdgesAreNotInteresting),
        "This array does not define all types once and only once.");

      // fill border in memory map
      currentNavMemoryMap->FillBorder(ContentType::InterestingEdge, typesWhoseEdgesAreNotInteresting, ContentType::NotInterestingEdge);
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result BlockWorld::AddVisionOverheadEdges(const OverheadEdgeFrame& frameInfo)
  {
    _robot->GetContext()->GetVizManager()->EraseSegments("BlockWorld.AddVisionOverheadEdges");
    
    // check conditions to add edges
    ASSERT_NAMED(!frameInfo.chains.empty(), "AddVisionOverheadEdges.NoEdges");
    ASSERT_NAMED(frameInfo.groundPlaneValid, "AddVisionOverheadEdges.InvalidGroundPlane");
    
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
    const bool poseIsGood = ( RESULT_OK == poseRet ) && (p != nullptr);
    if ( !poseIsGood ) {
      PRINT_NAMED_ERROR("BlockWorld.AddVisionOverheadEdges.PoseNotGood", "Pose not good for timestamp %d", frameInfo.timestamp);
      return RESULT_FAIL;
    }
    
    // If we can't transfor the observedPose to the current origin, it's ok, that means that the timestamp
    // for the edges we just received is from before delocalizing, so we should discard it.
    Pose3d observedPose;
    if ( !p->GetPose().GetWithRespectTo( *_robot->GetWorldOrigin(), observedPose) ) {
      PRINT_CH_INFO("BlockWorld",
        "BlockWorld.AddVisionOverheadEdges.NotInThisWorld",
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

    const float kBorderDepth = 1.0f;
    
    // TODO reserve some quads in each vector (what makes sense?)
    std::vector<Quad2f> visionQuadsClear;
    std::vector<Quad2f> visionQuadsWithInterestingBorders;
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

      ASSERT_NAMED(chain.points.size() > 2,"AddVisionOverheadEdges.ChainWithTooLittlePoints");
      
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
        
        // check line towards candidate point
        const Vec2f& rayFrom  = cameraOrigin;
        const Vec2f& rayTo    = candidate3D;
    
        // these are types that if the ray crosses them, we should have seen a border. The fact that
        // we didn't see the border is not proof that the object is not there. Ideally that's exactly
        // what we would want to deduct, but we can't trust failure to detect edges so reliably
        constexpr NavMemoryMapTypes::FullContentArray typesThatOccludeValidInfo =
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
          {NavMemoryMapTypes::EContentType::NotInterestingEdge    , false}
        };
        static_assert(NavMemoryMapTypes::ContentValueEntry::IsValidArray(typesThatOccludeValidInfo),
          "This array does not define all types once and only once.");
          
        const bool crossesObject = currentNavMemoryMap->HasCollisionRayWithTypes(rayFrom , rayTo, typesThatOccludeValidInfo);
        
        // if we cross an object, ignore this point, regardless of whether we saw a border or not
        // this is because if we are crossing an object, chances are we are seeing its border, of we should have,
        // so the info is more often disrupting than helpful
        bool isValidPoint = !crossesObject;
        
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
            // we have a valid segment add clear from camera to segment
            Quad2f clearQuad = { segmentStart, cameraOrigin, segmentEnd, cameraOrigin }; // TL, BL, TR, BR
            bool success = GroundPlaneROI::ClampQuad(clearQuad, nearPlaneLeft, nearPlaneRight);
            ASSERT_NAMED(success, "AddVisionOverheadEdges.FailedQuadClamp");
            if ( success ) {
              visionQuadsClear.emplace_back(clearQuad);
            }
            // if it's a detected border, add from segment on with kBorderDepth
            if ( chain.isBorder ) {
              Vec3f segStartDepthDir = (segmentStart - cameraOrigin);
              segStartDepthDir.MakeUnitLength();
              Vec3f segEndDepthDir = (segmentEnd - cameraOrigin);
              segEndDepthDir.MakeUnitLength();
              // TL, BL, TR, BR
              Quad2f borderQuad = { segmentStart + segStartDepthDir*kBorderDepth, segmentStart,
                                    segmentEnd   + segEndDepthDir*kBorderDepth  ,   segmentEnd };
              visionQuadsWithInterestingBorders.emplace_back(borderQuad);
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
              ASSERT_NAMED(false, "AddVisionOverheadEdges.NewSegmentCouldNotFindPreviousEnd");
            }
          }
          else
          {
            // we don't want to start a new segment, either we are the last point or we are not a valid point
            ASSERT_NAMED(!isValidPoint || isLastPoint, "AddVisionOverheadEdges.ValidPointNotStartingSegment");
            hasValidSegmentStart = false;
            hasValidSegmentEnd   = false;
          }
        } // sendSegmentToMap?
          
        // move to next point
        ++curPointInChainIdx;
      } while (curPointInChainIdx < chain.points.size()); // while we still have points
    }
    
    // send quads to memory map
    for ( const auto& clearQuad2D : visionQuadsClear )
    {
      if ( kDebugRenderOverheadEdgeClearQuads )
      {
        ColorRGBA color = Anki::NamedColors::GREEN;
        VizManager* vizManager = _robot->GetContext()->GetVizManager();
        vizManager->DrawQuadAsSegments("BlockWorld.AddVisionOverheadEdges", clearQuad2D, kDebugRenderOverheadEdgesZ_mm, color, false);
      }

      // add clear info to map
      if ( currentNavMemoryMap ) {
        currentNavMemoryMap->AddQuad(clearQuad2D, INavMemoryMap::EContentType::ClearOfObstacle);
      }
      
      // also notify behavior whiteboard.
      // rsam: should this information be in the map instead of the whiteboard? It seems a stretch that
      // blockworld knows now about behaviors, maybe all this processing of quads should be done in a separate
      // robot component, like a VisualInformationProcessingComponent
      _robot->GetBehaviorManager().GetWhiteboard().ProcessClearQuad(clearQuad2D);
    }
    
    // send quads to memory map
    for ( const auto& borderQuad2D : visionQuadsWithInterestingBorders )
    {
      if ( kDebugRenderOverheadEdgeBorderQuads )
      {
        ColorRGBA color = Anki::NamedColors::BLUE;
        VizManager* vizManager = _robot->GetContext()->GetVizManager();
        vizManager->DrawQuadAsSegments("BlockWorld.AddVisionOverheadEdges", borderQuad2D, kDebugRenderOverheadEdgesZ_mm, color, false);
      }
    
      // add interesting edge
      if ( currentNavMemoryMap ) {
        currentNavMemoryMap->AddQuad(borderQuad2D, INavMemoryMap::EContentType::InterestingEdge);
      }
    }
    
    // now merge interesting edges into non-interesting
    const bool addedEdges = !visionQuadsWithInterestingBorders.empty();
    if ( addedEdges )
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
      ReviewInterestingEdges(groundPlaneWRTOrigin);
    }
    
    return RESULT_OK;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BlockWorld::RemoveMarkersWithinMarkers(PoseKeyObsMarkerMap_t& currentObsMarkers)
  {
    for(auto markerIter1 = currentObsMarkers.begin(); markerIter1 != currentObsMarkers.end(); ++markerIter1)
    {
      const Vision::ObservedMarker& marker1    = markerIter1->second;
      const TimeStamp_t             timestamp1 = markerIter1->first;
      
      for(auto markerIter2 = currentObsMarkers.begin(); markerIter2 != currentObsMarkers.end(); /* incrementing decided in loop */ )
      {
        const Vision::ObservedMarker& marker2    = markerIter2->second;
        const TimeStamp_t             timestamp2 = markerIter2->first;
        
        // These two markers must be different and observed at the same time
        if(markerIter1 != markerIter2 && timestamp1 == timestamp2) {
          
          // See if #2 is inside #1
          bool marker2isInsideMarker1 = true;
          for(auto & corner : marker2.GetImageCorners()) {
            if(marker1.GetImageCorners().Contains(corner) == false) {
              marker2isInsideMarker1 = false;
              break;
            }
          }
          
          if(marker2isInsideMarker1) {
            PRINT_NAMED_INFO("BlockWorld.Update",
                             "Removing %s marker completely contained within %s marker.\n",
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


  Result BlockWorld::Update()
  {
    ANKI_CPU_PROFILE("BlockWorld::Update");
    
    // New timestep, new set of occluders.  Get rid of anything registered as
    // an occluder with the robot's camera
    _robot->GetVisionComponent().GetCamera().ClearOccluders();
    
    // New timestep, clear list of observed object bounding boxes
    //_obsProjectedObjects.clear();
    //_currentObservedObjectIDs.clear();
    
    static TimeStamp_t lastObsMarkerTime = 0;
    
    _currentObservedObjects.clear();
    
    // Now we're going to process all the observed messages, grouped by
    // timestamp
    size_t numUnusedMarkers = 0;
    for(auto obsMarkerListMapIter = _obsMarkers.begin();
        obsMarkerListMapIter != _obsMarkers.end();
        ++obsMarkerListMapIter)
    {
      PoseKeyObsMarkerMap_t& currentObsMarkers = obsMarkerListMapIter->second;
      const TimeStamp_t atTimestamp = obsMarkerListMapIter->first;
      
      lastObsMarkerTime = std::max(lastObsMarkerTime, atTimestamp);
      
      //
      // Localize robots using mat observations
      //
      
      // Remove observed markers whose historical poses have become invalid.
      // This shouldn't happen! If it does, robotStateMsgs may be buffering up somewhere.
      // Increasing history time window would fix this, but it's not really a solution.
      for(auto poseKeyMarkerPair = currentObsMarkers.begin(); poseKeyMarkerPair != currentObsMarkers.end();) {
        if ((poseKeyMarkerPair->second.GetSeenBy().GetID() == _robot->GetVisionComponent().GetCamera().GetID()) &&
            !_robot->IsValidPoseKey(poseKeyMarkerPair->first)) {
          PRINT_NAMED_WARNING("BlockWorld.Update.InvalidHistPoseKey", "key=%d", poseKeyMarkerPair->first);
          poseKeyMarkerPair = currentObsMarkers.erase(poseKeyMarkerPair);
        } else {
          ++poseKeyMarkerPair;
        }
      }
      
      // Optional: don't allow markers seen enclosed in other markers
      //RemoveMarkersWithinMarkers(currentObsMarkers);
      
      // Only update robot's poses using VisionMarkers while not on a ramp
      if(!_robot->IsOnRamp()) {
        if (!_robot->IsPhysical() || !SKIP_PHYS_ROBOT_LOCALIZATION) {
          // Note that this removes markers from the list that it uses
          UpdateRobotPose(currentObsMarkers, atTimestamp);
        }
      }
      
      // Reset the flag telling us objects changed here, before we update any objects:
      _didObjectsChange = false;

      Result updateResult;

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

      // TODO: Deal with unknown markers?
      
      // Keep track of how many markers went unused by either robot or block
      // pose updating processes above
      numUnusedMarkers += currentObsMarkers.size();
      
      for(auto & unusedMarker : currentObsMarkers) {
        PRINT_NAMED_INFO("BlockWorld.Update.UnusedMarker",
                         "An observed %s marker went unused.",
                         unusedMarker.second.GetCodeName());
      }
      
      // Delete any objects that should have been observed but weren't,
      // visualize objects that were observed:
      CheckForUnobservedObjects(atTimestamp);
      
    } // for element in _obsMarkers
    
    if(_obsMarkers.empty()) {
      // Even if there were no markers observed, check to see if there are
      // any previously-observed objects that are partially visible (some part
      // of them projects into the image even if none of their markers fully do)
      CheckForUnobservedObjects(_robot->GetLastImageTimeStamp());
    }
    
    //PRINT_NAMED_INFO("BlockWorld.Update.NumBlocksObserved", "Saw %d blocks", numBlocksObserved);
    
    auto originIter = _existingObjects.find(_robot->GetWorldOrigin());
    
    // Don't worry about collision with the robot while picking or placing since we
    // are trying to get close to objects in these modes.
    // TODO: Enable collision checking while picking and placing too
    // (I feel like we should be able to always do this, since we _do_ check that the
    //  object is not the docking object below, and we do height checks as well. But
    //  I'm wary of changing it now either...)
    if(!_robot->IsPickingOrPlacing() && originIter != _existingObjects.end())
    {
      // Check for unobserved, uncarried objects that overlap with any robot's position
      // TODO: better way of specifying which objects are obstacles and which are not
      // TODO: Move this giant loop to its own method
      for(auto & objectsByFamily : originIter->second)
      {
        // For now, look for collision with anything other than Mat objects
        // NOTE: This assumes all other objects are DockableObjects below!!! (Becuase of IsBeingCarried() check)
        // TODO: How can we delete Mat objects (like platforms) whose positions we drive through
        if(objectsByFamily.first != ObjectFamily::Mat &&
           objectsByFamily.first != ObjectFamily::MarkerlessObject &&
           objectsByFamily.first != ObjectFamily::CustomObject)
        {
          for(auto & objectsByType : objectsByFamily.second)
          {
            for(auto & objectIdPair : objectsByType.second)
            {
              ActionableObject* object = dynamic_cast<ActionableObject*>(objectIdPair.second.get());
              if(object == nullptr) {
                PRINT_NAMED_ERROR("BlockWorld.Update.ExpectingActionableObject",
                                  "In robot/object collision check, can currently only "
                                  "handle ActionableObjects.");
                continue;
              }
              
              // Collision check objects that
              // - were not observed in the last image,
              // - are not being carried
              // - still have known pose state (not dirtied already, nor unknown)
              // - are not the obect we are docking with (since we are _trying_ to get close)
              // - can intersect the robot (e.g. not charger)
              if(object->GetLastObservedTime() < _robot->GetLastImageTimeStamp() &&
                 !object->IsBeingCarried() &&
                 object->IsPoseStateKnown() &&
                 object->GetID() != _robot->GetDockObject() &&
                 !object->CanIntersectWithRobot())
              {
                // Check block's bounding box in same coordinates as this robot to
                // see if it intersects with the robot's bounding box. Also check to see
                // block and the robot are at overlapping heights.  Skip this check
                // entirely if the block isn't in the same coordinate tree as the
                // robot.
                Pose3d objectPoseWrtRobotOrigin;
                if(true == object->GetPose().GetWithRespectTo(*_robot->GetWorldOrigin(), objectPoseWrtRobotOrigin))
                {
                  const Quad2f objectBBox = object->GetBoundingQuadXY(objectPoseWrtRobotOrigin);
                  const f32    objectHeight = objectPoseWrtRobotOrigin.GetTranslation().z();
                  /*
                   const f32    blockSize   = 0.5f*object->GetSize().Length();
                   const f32    blockTop    = objectHeight + blockSize;
                   const f32    blockBottom = objectHeight - blockSize;
                   */
                  const f32 robotBottom = _robot->GetPose().GetTranslation().z();
                  const f32 robotTop    = robotBottom + ROBOT_BOUNDING_Z;
                  
                  // TODO: Better check for being in the same plane that takes the
                  //       vertical extent of the object (in its current pose) into account
                  const bool inSamePlane = (objectHeight >= robotBottom && objectHeight <= robotTop);
                  /*
                   const bool topIntersects    = (((blockTop >= robotBottom) && (blockTop <= robotTop)) ||
                   ((robotTop >= blockBottom) && (robotTop <= blockTop)));
                   
                   const bool bottomIntersects = (((blockBottom >= robotBottom) && (blockBottom <= robotTop)) ||
                   ((robotBottom >= blockBottom) && (robotBottom <= blockTop)));
                   */
                  
                  const Quad2f robotBBox = _robot->GetBoundingQuadXY(_robot->GetPose().GetWithRespectToOrigin(),
                                                                     ROBOT_BBOX_PADDING_FOR_OBJECT_COLLISION);
                  
                  const bool bboxIntersects = robotBBox.Intersects(objectBBox);
                  
                  if( inSamePlane && bboxIntersects )
                  {
                    PRINT_NAMED_INFO("BlockWorld.Update",
                                     "Marking object %d as 'dirty', because it intersects robot %d's bounding quad.",
                                     object->GetID().GetValue(), _robot->GetID());
                    
                    // Mark object and everything on top of it as "dirty". Next time we look
                    // for it and don't see it, we will fully clear it and mark it as "unknown"
                    ObservableObject* objectOnTop = object;
                    BOUNDED_WHILE(20, objectOnTop != nullptr) {
                      objectOnTop->SetPoseState(PoseState::Dirty);
                      objectOnTop = FindObjectOnTopOf(*objectOnTop, STACKED_HEIGHT_TOL_MM);
                    }
                  } // if quads intersect
                }
                else
                {
                  // We should not get here because we are only looping over objects that are
                  // in the robot's current frame
                  PRINT_NAMED_WARNING("BlockWorld.Update.BadOrigin", "");
                } // if we got block pose wrt robot origin
                
              } // if block was not observed
              
            } // for each object of this type
          } // for each object type
        } // if not in the Mat family
      } // for each object family
    } // if robot is not picking or placing
    
    if(numUnusedMarkers > 0) {
      if (!_robot->IsPhysical() || !SKIP_PHYS_ROBOT_LOCALIZATION) {
        PRINT_NAMED_WARNING("BlockWorld.Update.UnusedMarkers",
                            "%zu observed markers did not match any known objects and went unused.",
                            numUnusedMarkers);
      }
    }
   
    // Toss any remaining markers?
    ClearAllObservedMarkers();      
    
    /*
    Result lastResult = UpdateProxObstaclePoses();
    if(lastResult != RESULT_OK) {
      return lastResult;
    }
    */

    return RESULT_OK;
    
  } // Update()
  
  
  Result BlockWorld::QueueObservedMarker(HistPoseKey& poseKey, Vision::ObservedMarker& marker)
  {
    Result lastResult = RESULT_OK;
    
    // Finally actually queue the marker
    _obsMarkers[marker.GetTimeStamp()].emplace(poseKey, marker);
          
    
    return lastResult;
    
  } // QueueObservedMarker()
  
  void BlockWorld::ClearAllObservedMarkers()
  {
    _obsMarkers.clear();
  }
  
  void BlockWorld::ClearAllExistingObjects()
  {
    if(_canDeleteObjects) {
      BlockWorldFilter filter;
      filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
      filter.SetFilterFcn(nullptr);
      
      ModifyObjects([this](ObservableObject* object) { ClearObjectHelper(object); });
      
    }  else {
      PRINT_NAMED_WARNING("BlockWorld.ClearAllExistingObjects.DeleteDisabled",
                          "Will not clear all objects because object deletion is disabled.");
    }
  }

  void BlockWorld::DeleteAllExistingObjects()
  {
    // TODO Fix the lightcube calls which rely on these objects still existing
    if(_canDeleteObjects) {
      
      // Note: no need to explicitly delete objects because they are now stored
      // as shared pointers, so just call clear on all of them individually,
      // and then clear the container.
      ClearAllExistingObjects();
      
      //finally clear the entire map, now that everything inside has been deleted
      _existingObjects.clear();
      
    }  else {
      PRINT_NAMED_WARNING("BlockWorld.DeleteAllExistingObjects.DeleteDisabled",
                          "Will not delete all objects because object deletion is disabled.");
    }
  }

  void BlockWorld::ClearObjectHelper(ObservableObject* object)
  {
    if(object == nullptr) {
      PRINT_NAMED_WARNING("BlockWorld.ClearObjectHelper.NullObjectPointer",
                          "BlockWorld asked to clear a null object pointer.");
    } else {
      // Check to see if this object is the one the robot is localized to.
      // If so, the robot needs to be marked as localized to nothing.
      if(_robot->GetLocalizedTo() == object->GetID()) {
        PRINT_NAMED_INFO("BlockWorld.ClearObjectHelper.LocalizeRobotToNothing",
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
        PRINT_NAMED_INFO("BlockWorld.ClearObjectHelper.ClearingCarriedObject",
                         "Clearing %s object %d which robot %d thinks it is carrying.",
                         ObjectTypeToString(object->GetType()),
                         object->GetID().GetValue(),
                         _robot->GetID());
        _robot->UnSetCarryingObjects();
      }
      
      if(_selectedObject == object->GetID()) {
        PRINT_NAMED_INFO("BlockWorld.ClearObjectHelper.ClearingSelectedObject",
                         "Clearing %s object %d which is currently selected.",
                         ObjectTypeToString(object->GetType()),
                         object->GetID().GetValue());
        _selectedObject.UnSet();
      }


      // Setting pose to unknown makes the object no longer "existence confirmed", so save value now
      bool wasExistenceConfirmed = object->IsExistenceConfirmed();
      
      object->SetPoseState(PoseState::Unknown);
      
      ObservableObject* objectOnTop = FindObjectOnTopOf(*object, STACKED_HEIGHT_TOL_MM);
      if(objectOnTop != nullptr)
      {
        ClearObject(objectOnTop);
      }
      
      // Notify any listeners that this object no longer has a valid Pose
      // (Only notify for objects that were broadcast in the first place, meaning
      //  they must have been seen the minimum number of times and not be in the
      //  process of being identified)
      if(wasExistenceConfirmed)
      {
        using namespace ExternalInterface;
        _robot->Broadcast(MessageEngineToGame(RobotMarkedObjectPoseUnknown(
          _robot->GetID(), object->GetID().GetValue()
        )));
      }
      
      // Flag that we removed an object
      _didObjectsChange = true;
    }
  }
  
  ObservableObject* BlockWorld::FindObjectHelper(const BlockWorldFilter& filter, const ModifierFcn& modifierFcn, bool returnFirstFound) const
  {
    ObservableObject* matchingObject = nullptr;
    
    if(filter.IsOnlyConsideringLatestUpdate()) {
      
      for(auto candidate_nonconst : _currentObservedObjects)
      {
        const ObservableObject* candidate = candidate_nonconst;
        
        if(filter.ConsiderOrigin(&candidate->GetPose().FindOrigin(), _robot->GetWorldOrigin()) &&
           filter.ConsiderFamily(candidate->GetFamily()) &&
           filter.ConsiderType(candidate->GetType()) &&
           filter.ConsiderObject(candidate))
        {
          matchingObject = candidate_nonconst;
          
          if(nullptr != modifierFcn) {
            modifierFcn(matchingObject);
          }
          
          if(returnFirstFound) {
            return matchingObject;
          }
        }
      }
      
    } else {
      for(auto & objectsByOrigin : _existingObjects) {
        if(filter.ConsiderOrigin(objectsByOrigin.first, _robot->GetWorldOrigin())) {
          for(auto & objectsByFamily : objectsByOrigin.second) {
            if(filter.ConsiderFamily(objectsByFamily.first)) {
              for(auto & objectsByType : objectsByFamily.second) {
                if(filter.ConsiderType(objectsByType.first)) {
                  for(auto & objectsByID : objectsByType.second) {
                    ObservableObject* object_nonconst = objectsByID.second.get();
                    const ObservableObject* object = object_nonconst;
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
    }
    
    return matchingObject;
  }
  
  ObservableObject* BlockWorld::FindObjectOnTopOfHelper(const ObservableObject& objectOnBottom,
                                                        f32 zTolerance,
                                                        const BlockWorldFilter& filterIn) const
  {
    Point3f sameDistTol(objectOnBottom.GetSize());
    sameDistTol.x() *= 0.5f;  // An object should only be considered to be on top if it's midpoint is actually on top of the bottom object's top surface.
    sameDistTol.y() *= 0.5f;
    sameDistTol.z() = zTolerance;
    sameDistTol = objectOnBottom.GetPose().GetRotation() * sameDistTol;
    sameDistTol.Abs();
    
    // Find the point at the top middle of the object on bottom
    Point3f rotatedBtmSize(objectOnBottom.GetPose().GetRotation() * objectOnBottom.GetSize());
    Point3f topOfObjectOnBottom(objectOnBottom.GetPose().GetTranslation());
    topOfObjectOnBottom.z() += 0.5f*std::abs(rotatedBtmSize.z());
    
    BlockWorldFilter filter(filterIn);
    filter.AddIgnoreID(objectOnBottom.GetID());
    filter.AddFilterFcn([&topOfObjectOnBottom, &sameDistTol](const ObservableObject* candidateObject)
                        {
                          // Find the point at bottom middle of the object we're checking to be on top
                          Point3f rotatedTopSize(candidateObject->GetPose().GetRotation() * candidateObject->GetSize());
                          Point3f bottomOfCandidateObject(candidateObject->GetPose().GetTranslation());
                          bottomOfCandidateObject.z() -= 0.5f*std::abs(rotatedTopSize.z());
                          
                          // If the top of the bottom object and the bottom the candidate top object are
                          // close enough together, return this as the object on top
                          Point3f dist(topOfObjectOnBottom);
                          dist -= bottomOfCandidateObject;
                          dist.Abs();
                          
                          if(dist < sameDistTol) {
                            return true;
                          } else {
                            return false;
                          }
                        });
    
    return FindObjectHelper(filter, nullptr, true);
  }
  
  
  ObservableObject* BlockWorld::FindObjectUnderneathHelper(const ObservableObject& objectOnTop,
                                                           f32 zTolerance,
                                                           const BlockWorldFilter& filterIn) const
  {
    Point3f sameDistTol(objectOnTop.GetSize());
    sameDistTol.x() *= 0.5f;  // An object should only be considered to be on top if it's midpoint is actually on top of the bottom object's top surface.
    sameDistTol.y() *= 0.5f;
    sameDistTol.z() = zTolerance;
    sameDistTol = objectOnTop.GetPose().GetRotation() * sameDistTol;
    sameDistTol.Abs();
    
    // Find the point at the top middle of the object on bottom
    Point3f rotatedBtmSize(objectOnTop.GetPose().GetRotation() * objectOnTop.GetSize());
    Point3f bottomOfObjectOnTop(objectOnTop.GetPose().GetTranslation());
    bottomOfObjectOnTop.z() -= 0.5f*std::abs(rotatedBtmSize.z());
    
    BlockWorldFilter filter(filterIn);
    filter.AddIgnoreID(objectOnTop.GetID());
    filter.AddFilterFcn([&bottomOfObjectOnTop, &sameDistTol](const ObservableObject* candidateObject)
                        {
                          // Find the point at top middle of the object we're checking to be underneath
                          Point3f rotatedBtmSize(candidateObject->GetPose().GetRotation() * candidateObject->GetSize());
                          Point3f topOfCandidateObject(candidateObject->GetPose().GetTranslation());
                          topOfCandidateObject.z() += 0.5f*std::abs(rotatedBtmSize.z());
                          
                          // If the top of the bottom object and the bottom the candidate top object are
                          // close enough together, return this as the object on top
                          Point3f dist(bottomOfObjectOnTop);
                          dist -= topOfCandidateObject;
                          dist.Abs();
                          
                          if(dist < sameDistTol) {
                            return true;
                          } else {
                            return false;
                          }
                        });
    
    return FindObjectHelper(filter, nullptr, true);
  }

  
  ObservableObject* BlockWorld::FindObjectClosestToHelper(const Pose3d& pose,
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
    
    return FindObjectHelper(filter);
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool BlockWorld::AnyRemainingLocalizableObjects() const
  {
    return AnyRemainingLocalizableObjects(nullptr);
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
    
    if(nullptr != FindObjectHelper(filter, nullptr, true)) {
      return true;
    } else {
      return false;
    }
  }

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
    
    return FindObjectHelper(filter);
  }

  ObservableObject* BlockWorld::FindClosestMatchingObject(const ObservableObject& object,
                                                          const Vec3f& distThreshold,
                                                          const Radians& angleThreshold,
                                                          const BlockWorldFilter& filterIn)
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
    
    ObservableObject* closestObject = FindObjectHelper(filter);
    return closestObject;
  } // FindClosestMatchingObject(given object)

  ObservableObject* BlockWorld::FindClosestMatchingObject(ObjectType withType,
                                                          const Pose3d& pose,
                                                          const Vec3f& distThreshold,
                                                          const Radians& angleThreshold,
                                                          const BlockWorldFilter& filterIn)
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
    
    ObservableObject* closestObject = FindObjectHelper(filter);
    return closestObject;
  } // FindClosestMatchingObject(given pose)

  void BlockWorld::ClearObjectsByFamily(const ObjectFamily family)
  {
    if(_canDeleteObjects) {
      
      BlockWorldFilter filter;
      filter.AddAllowedFamily(family);
      filter.SetFilterFcn(nullptr);
      
      ModifierFcn clearObjectFcn = [this](ObservableObject* object) { ClearObjectHelper(object); };
      
      FindObjectHelper(filter, clearObjectFcn, false);
      
    } else {
      PRINT_NAMED_WARNING("BlockWorld.ClearObjectsByFamily.ClearDisabled",
                          "Will not clear family %d objects because object deletion is disabled.",
                          family);
    }
  }
  
  void BlockWorld::ClearObjectsByType(const ObjectType type)
  {
    if(_canDeleteObjects) {
      BlockWorldFilter filter;
      filter.AddAllowedType(type);
      filter.SetFilterFcn(nullptr);
      
      ModifierFcn clearObjectFcn = [this](ObservableObject* object) { ClearObjectHelper(object); };
      
      FindObjectHelper(filter, clearObjectFcn, false);
      
    } else {
      PRINT_NAMED_WARNING("BlockWorld.ClearObjectsByType.DeleteDisabled",
                          "Will not clear %s objects because object deletion is disabled.",
                          ObjectTypeToString(type));

    }
  } // ClearBlocksByType()

  void BlockWorld::DeleteObjectsByOrigin(const Pose3d* origin, bool clearFirst)
  {
    if(_canDeleteObjects) {
      auto originIter = _existingObjects.find(origin);
      if(originIter != _existingObjects.end()) {
        if(clearFirst)
        {
          for(auto & objectsByFamily : originIter->second) {
            for(auto & objectsByType : objectsByFamily.second) {
              for(auto & objectsByID : objectsByType.second) {
                ClearObjectHelper(objectsByID.second.get());
              }
            }
          }
        }
        _existingObjects.erase(originIter);
      }
    } else {
      PRINT_NAMED_WARNING("BlockWorld.DeleteObjectsByOrigin.DeleteDisabled",
                          "Will not delete origin %p objects because object deletion is disabled.",
                          origin);
    }
  }

  void BlockWorld::DeleteObjectsByFamily(const ObjectFamily family)
  {
    if(_canDeleteObjects) {
      for(auto & objectsByOrigin : _existingObjects) {
        auto familyIter = objectsByOrigin.second.find(family);
        if(familyIter != objectsByOrigin.second.end()) {
          for(auto & objectsByType : familyIter->second) {
            for(auto & objectsByID : objectsByType.second) {
              ClearObjectHelper(objectsByID.second.get());
            }
          }
          objectsByOrigin.second.erase(familyIter);
        }
      }
    } else {
      PRINT_NAMED_WARNING("BlockWorld.DeleteObjectsByFamily.DeleteDisabled",
                          "Will not delete family %d objects because object deletion is disabled.",
                          family);
    }
  }
  
  void BlockWorld::DeleteObjectsByType(const ObjectType type) {
    if(_canDeleteObjects) {
      for(auto & objectsByOrigin : _existingObjects) {
        for(auto & objectsByFamily : objectsByOrigin.second) {
          auto typeIter = objectsByFamily.second.find(type);
          if(typeIter != objectsByFamily.second.end()) {
            for(auto & objectsByID : typeIter->second) {
              ClearObjectHelper(objectsByID.second.get());
            }
            
            objectsByFamily.second.erase(typeIter);
            
            // Types are unique.  No need to keep looking
            return;
          }
        }
      }
    } else {
      PRINT_NAMED_WARNING("BlockWorld.DeleteObjectsByType.DeleteDisabled",
                          "Will not delete %s objects because object deletion is disabled.",
                          ObjectTypeToString(type));
      
    }
  }

  bool BlockWorld::DeleteObject(const ObjectID& withID)
  {
    ObservableObject* object = GetObjectByID(withID);
    
    return DeleteObject(object);
  }

  bool BlockWorld::DeleteObject(ObservableObject* object)
  {
    bool retval = false;
    if(nullptr != object)
    {
      // Inform caller that we found the requested ID:
      retval = true;
      
      // Need to do all the same cleanup as Clear() calls
      ClearObjectHelper(object);
      
      // Actually delete the object we found
      const Pose3d* origin = &object->GetPose().FindOrigin();
      ObjectFamily inFamily = object->GetFamily();
      ObjectType   withType = object->GetType();
      
      // And remove it from the container
      // Note: we're using shared_ptr to store the objects, so erasing from
      //       the container will delete it if nothing else is referring to it
      _existingObjects.at(origin).at(inFamily).at(withType).erase(object->GetID());
    }
    return retval;
  } // DeleteObject()

  bool BlockWorld::ClearObject(ObservableObject* object)
  {
    if(nullptr == object) {
      return false;
    } else if(_canDeleteObjects || object->GetNumTimesObserved() < MIN_TIMES_TO_OBSERVE_OBJECT) {
      ClearObjectHelper(object);
      return true;
    } else {
      PRINT_NAMED_WARNING("BlockWorld.ClearObject.DeleteDisabled",
                          "Will not clear object %d because object deletion is disabled.",
                          object->GetID().GetValue());
      return false;
    }
  }

  bool BlockWorld::ClearObject(const ObjectID& withID)
  {
    // Note: In current frame only!
    return ClearObject(GetObjectByID(withID));
  } // ClearObject()
  

  BlockWorld::ObjectsMapByID_t::iterator BlockWorld::DeleteObject(const ObjectsMapByID_t::iterator objIter,
                                                                  const ObjectType&   withType,
                                                                  const ObjectFamily& fromFamily)
  {
    ObservableObject* object = objIter->second.get();
    
    if(_canDeleteObjects || object->GetNumTimesObserved() < MIN_TIMES_TO_OBSERVE_OBJECT)
    {
      const Pose3d* origin = &objIter->second->GetPose().FindOrigin();
      
      ClearObjectHelper(object);
      
      // Erase from the container and return the iterator to the next element
      // Note: we're using a shared_ptr so this should delete the object as well.
      return _existingObjects.at(origin).at(fromFamily).at(withType).erase(objIter);
    } else {
      PRINT_NAMED_WARNING("BlockWorld.DeleteObject.DeleteDisabled",
                          "Will not delete object %d because object deletion is disabled.",
                          object->GetID().GetValue());
      auto retIter(objIter);
      return ++retIter;
    }
  }

  void BlockWorld::DeselectCurrentObject()
  {
    if(_selectedObject.IsSet()) {
      ActionableObject* curSel = dynamic_cast<ActionableObject*>(GetObjectByID(_selectedObject));
      if(curSel != nullptr) {
        curSel->SetSelected(false);
      }
      _selectedObject.UnSet();
    }
  }


  bool BlockWorld::SelectObject(const ObjectID& objectID)
  {
    ActionableObject* newSelection = dynamic_cast<ActionableObject*>(GetObjectByID(objectID));
    
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
      PRINT_STREAM_WARNING("BlockWorld.SelectObject.InvalidID",
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
      ActionableObject* object = dynamic_cast<ActionableObject*>(GetObjectByID(_selectedObject));
      if(object != nullptr) {
        object->SetSelected(false);
      }
    }
    
    bool currSelectedObjectFound = false;
    bool newSelectedObjectSet = false;
    
    // Iterate through all the objects
    auto const & allObjects = GetAllExistingObjects();
    for(auto const & objectsByFamily : allObjects)
    {
      // Markerless objects are not Actionable, so ignore them for selection
      if(objectsByFamily.first != ObjectFamily::MarkerlessObject)
      {
        for (auto const & objectsByType : objectsByFamily.second){
          
          //PRINT_INFO("currType: %d", blockType.first);
          for (auto const & objectsByID : objectsByType.second) {
            
            ActionableObject* object = dynamic_cast<ActionableObject*>(objectsByID.second.get());
            if(object != nullptr && object->HasPreActionPoses() && !object->IsBeingCarried() &&
               object->IsExistenceConfirmed())
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
          } // for each ID
          
          if (newSelectedObjectSet) {
            break;
          }
          
        } // for each type
        
        if(newSelectedObjectSet) {
          break;
        }
      } // if family != MARKERLESS_OBJECTS
    } // for each family
    
    // If the current object of interest was found, but a new one was not set
    // it must have been the last block in the map. Set the new object of interest
    // to the first object in the map as long as it's not the same object.
    if (!currSelectedObjectFound || !newSelectedObjectSet) {
      
      // Find first object
      ObjectID firstObject; // initialized to un-set
      for(auto const & objectsByFamily : allObjects) {
        for (auto const & objectsByType : objectsByFamily.second) {
          for (auto const & objectsByID : objectsByType.second) {
            const ActionableObject* object = dynamic_cast<ActionableObject*>(objectsByID.second.get());
            if(object != nullptr && object->HasPreActionPoses() && !object->IsBeingCarried() &&
              object->IsExistenceConfirmed())
            {
              firstObject = objectsByID.first;
              break;
            }
          }
          if (firstObject.IsSet()) {
            break;
          }
        }
        
        if (firstObject.IsSet()) {
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
    ActionableObject* object = dynamic_cast<ActionableObject*>(GetObjectByID(_selectedObject));
    if (object != nullptr) {
      object->SetSelected(true);
      PRINT_STREAM_INFO("BlockWorld.CycleSelectedObject", "Object of interest: ID = " << _selectedObject.GetValue());
    } else {
      PRINT_STREAM_INFO("BlockWorld.CycleSelectedObject", "No object of interest found");
    }

  } // CycleSelectedObject()
  
  
  void BlockWorld::EnableDraw(bool on)
  {
    _enableDraw = on;
  }
  
  void BlockWorld::DrawObsMarkers() const
  {
    if (_enableDraw) {
      for (const auto& poseKeyMarkerMapAtTimestamp : _obsMarkers) {
        for (const auto& poseKeyMarkerMap : poseKeyMarkerMapAtTimestamp.second) {
          const Quad2f& q = poseKeyMarkerMap.second.GetImageCorners();
          f32 scaleF = 1.0f;
          switch(IMG_STREAM_RES) {
            case ImageResolution::CVGA:
            case ImageResolution::QVGA:
              break;
            case ImageResolution::QQVGA:
              scaleF *= 0.5;
              break;
            case ImageResolution::QQQVGA:
              scaleF *= 0.25;
              break;
            case ImageResolution::QQQQVGA:
              scaleF *= 0.125;
              break;
            default:
              printf("WARNING (DrawObsMarkers): Unsupported streaming res %d\n", (int)IMG_STREAM_RES);
              break;
          }
          _robot->GetContext()->GetVizManager()->SendTrackerQuad(q[Quad::TopLeft].x()*scaleF,     q[Quad::TopLeft].y()*scaleF,
                                                                 q[Quad::TopRight].x()*scaleF,    q[Quad::TopRight].y()*scaleF,
                                                                 q[Quad::BottomRight].x()*scaleF, q[Quad::BottomRight].y()*scaleF,
                                                                 q[Quad::BottomLeft].x()*scaleF,  q[Quad::BottomLeft].y()*scaleF);
        }
      }
    }
  }
  
  void BlockWorld::DrawAllObjects() const
  {
    const ObjectID& locObject = _robot->GetLocalizedTo();
    
    // Note: only drawing objects in current coordinate frame!
    BlockWorldFilter filter;
    filter.SetOriginMode(BlockWorldFilter::OriginMode::InRobotFrame);
    filter.SetFilterFcn(nullptr);
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
      else if(object->GetPoseState() == PoseState::Unknown) {
        // Draw unknown objects in a special color
        object->Visualize(NamedColors::UNKNOWN_OBJECT);
      }
      else {
        // Draw "regular" objects in current frame in their internal color
        object->Visualize();
      }
    };

    FindObjectHelper(filter, visualizeHelper, false);
    
    // (Re)Draw the selected object separately so we can get its pre-action poses
    if(GetSelectedObject().IsSet())
    {
      const ActionableObject* selectedObject = dynamic_cast<const ActionableObject*>(GetObjectByID(GetSelectedObject()));
      if(selectedObject == nullptr) {
        PRINT_NAMED_ERROR("BlockWorld.DrawAllObjects.NullSelectedObject",
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
  
} // namespace Cozmo
} // namespace Anki
