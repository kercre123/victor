/**
 * File: behaviorOCD.cpp
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Implements Cozmo's "OCD" behavior, which likes to keep blocks neat n' tidy.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/behaviors/behaviorOCD.h"

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoActions.h"

#include "anki/cozmo/basestation/events/ankiEventMgr.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

#define DEBUG_OCD_BEHAVIOR 0

#define PLACEMENT_OFFSET_MM 88.f  // Twice block width

namespace Anki {
namespace Cozmo {
  
  BehaviorOCD::BehaviorOCD(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _lastHandlerResult(RESULT_OK)
  , _currentArrangement(Arrangement::StacksOfTwo)
  , _irritationLevel(0)
  {
    _name = "OCD";
    // Register callbacks:
    
    // Create a callback for running EventHandler
    auto eventHandlerCallback = [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event) {
      this->EventHandler(event);
    };
    
    // Register that callback to be run for any of the events we are interested in
    static const std::vector<ExternalInterface::MessageEngineToGameTag> EventTagsOfInterest = {
      ExternalInterface::MessageEngineToGameTag::RobotCompletedAction,
      ExternalInterface::MessageEngineToGameTag::RobotObservedObject,
      ExternalInterface::MessageEngineToGameTag::RobotDeletedObject,
      ExternalInterface::MessageEngineToGameTag::ActiveObjectMoved
    };
    
    // Note we may not have an external interface when running Unit tests
    if (robot.HasExternalInterface())
    {
      for(auto tag : EventTagsOfInterest) {
        _eventHandles.push_back(robot.GetExternalInterface()->Subscribe(tag, eventHandlerCallback));
      }
    }
    
  }
  
  void BehaviorOCD::EventHandler(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
  {
    switch(event.GetData().GetTag())
    {
      case ExternalInterface::MessageEngineToGameTag::RobotCompletedAction:
        _lastHandlerResult= HandleActionCompleted(event.GetData().Get_RobotCompletedAction());
        break;
        
      case ExternalInterface::MessageEngineToGameTag::RobotObservedObject:
        _lastHandlerResult= HandleObservedObject(event.GetData().Get_RobotObservedObject());
        break;
        
      case ExternalInterface::MessageEngineToGameTag::RobotDeletedObject:
        _lastHandlerResult= HandleDeletedObject(event.GetData().Get_RobotDeletedObject());
        break;
        
      case ExternalInterface::MessageEngineToGameTag::ActiveObjectMoved:
        _lastHandlerResult= HandleObjectMoved(event.GetData().Get_ActiveObjectMoved());
        break;
        
      default:
        PRINT_NAMED_ERROR("BehaviorOCD.EventHandler.InvalidTag",
                          "Received unexpected event with tag %hhu.", event.GetData().GetTag());
        _lastHandlerResult = RESULT_FAIL;
        break;
    }
  }
  
#pragma mark -
#pragma mark Inherited Virtual Implementations
  
  bool BehaviorOCD::IsRunnable(float currentTime_sec) const
  {
    // We can only run this behavior when there are at least two "messy" blocks present,
    // or when there is at least one messy block while we've got any neat blocks
    return (_messyObjects.size() >= 2) || (!_neatObjects.empty() && !_messyObjects.empty());
  }
  
  Result BehaviorOCD::Init()
  {
    Result lastResult = RESULT_OK;
    
    _interrupted = false;
    
    if(_robot.IsCarryingObject()) {
      // Make sure whatever the robot is carrying is some kind of block...
      // (note that we have an actively-updated list of blocks, so we can just
      //  check against that)
      ObjectID carriedObjectID = _robot.GetCarryingObject();
      
      const bool carriedObjectIsBlock = _messyObjects.count(carriedObjectID) > 0;
      
      if(carriedObjectIsBlock) {
        // ... if so, start in PlacingBlock mode
        _currentState = State::PlacingBlock;
        lastResult = SelectNextPlacement();
      } else {
        // ... if not, put this thing down and start in PickingUpBlock mode
        _currentState = State::PickingUpBlock;
        
        // TODO: Find a good place to put down this object
        // For now, just put it down right _here_
        PRINT_NAMED_WARNING("BehaviorOCD.Init.PlacingBlockDown", "Make sure this pose is clear!");
        lastResult = _robot.GetActionList().QueueActionNow(IBehavior::sActionSlot, new PlaceObjectOnGroundAction());
        if(lastResult != RESULT_OK) {
          PRINT_NAMED_ERROR("BehaviorOCD.Init.PlacementFailed",
                            "Failed to queue PlaceObjectOnGroundAction.");
          return lastResult;
        }
        
        lastResult = SelectNextObjectToPickUp();
      }
    } else {
      lastResult = SelectNextObjectToPickUp();
      _currentState = State::PickingUpBlock;
    }
    
    lastResult = SelectArrangement();
    
    return lastResult;
  } // Init()

  
  IBehavior::Status BehaviorOCD::Update(float currentTime_sec)
  {
    // Check to see if we had any problems with any handlers
    if(_lastHandlerResult != RESULT_OK) {
      PRINT_NAMED_ERROR("BehaviorOCD.Update.HandlerFailure",
                        "Event handler failed, returning Status::FAILURE.");
      return Status::Failure;
    }
    
    // Completion trigger is when all (?) blocks make it to his "neat" list
    if(_messyObjects.empty()) {
      return Status::Complete;
    }
    
    if(_interrupted) {
      // If we are in the middle of picking up a block, we can immediately interrupt.
      // Otherwise we will wait until placing completes which switches us back to
      // PickingUpBlock, so we can then get here
      return Status::Complete;
    }
    
    switch(_currentState)
    {
      case State::PickingUpBlock:
      case State::PlacingBlock:
      case State::Irritated:
        break;
        
      default:
        PRINT_NAMED_ERROR("OCD_Behavior.Update.UnknownState",
                          "Reached unknown state %d.", _currentState);
        return Status::Failure;
    }
    
    return Status::Running;
  }
  
  Result BehaviorOCD::Interrupt(float currentTime_sec)
  {
    _interrupted = true;
    return RESULT_OK;
  }
  
  bool BehaviorOCD::GetRewardBid(Reward& reward)
  {
    
    // TODO: Fill in reward
    
    return true;
  }
  
#pragma mark -
#pragma mark OCD-Specific Methods
  
  void BehaviorOCD::UpdateName()
  {
    _stateName = "";
    switch(_currentArrangement)
    {
      case Arrangement::Line:
        _stateName += "LINE";
        break;
        
      case Arrangement::StacksOfTwo:
        _stateName += "STACKS_OF_TWO";
        break;
        
      default:
        PRINT_NAMED_ERROR("BehaviorOCD.UpdateName.InvalidArrangment", "");
    }
    
    switch(_currentState)
    {
      case State::PickingUpBlock:
        _stateName += "-PICKING";
        break;
        
      case State::PlacingBlock:
        _stateName += "-PLACING";
        break;
        
      case State::Irritated:
        _stateName += "-Irritated";
        break;
        
      default:
        PRINT_NAMED_ERROR("BehaviorOCD.UpdateName.InvalidState", "");
    }
  }
  
  Result BehaviorOCD::SelectArrangement()
  {
    Arrangement previousArrangement = _currentArrangement;
    
    // TODO: Make this based on something "smarter"
    /*
    // For now, just rotate between available arrangments
    s32 iArrangement = static_cast<s32>(_currentArrangement);
    ++iArrangement;
    if(iArrangement == static_cast<s32>(Arrangement::NumArrangements)) {
      iArrangement = 0;
    }
    _currentArrangement = static_cast<Arrangement>(iArrangement);
    */
    _currentArrangement = Arrangement::StacksOfTwo;
    
    BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.SelectArrangement", "Selected arrangment %d", _currentArrangement);
    
    if(_currentArrangement != previousArrangement) {
      _lastObjectPlacedOnGround.UnSet();
      UpdateName();
    }
    
    return RESULT_OK;
  }
  
  Result BehaviorOCD::SelectNextObjectToPickUp()
  {
    BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.SelectNextObjectToPickUp", "");
    
    if(_messyObjects.empty()) {
      PRINT_NAMED_ERROR("BehaviorOCD.SelectNextObjectToPickUp.NoMessyObjects", "");
      return RESULT_FAIL;
    }
    
    BlockWorld& blockWorld = _robot.GetBlockWorld();
    
    std::vector<std::pair<Quad2f, ObjectID> > obstacles;
    blockWorld.GetObstacles(obstacles);
    
    // Find the closest object with available pre-action poses
    f32 closestDistSq = std::numeric_limits<f32>::max();
    ObservableObject* object = nullptr;
    for(auto & objectID : _messyObjects)
    {
      object = blockWorld.GetObjectByID(objectID);
      if(object == nullptr) {
        PRINT_NAMED_ERROR("BehaviorOCD.SelectNextObjectToPickUp.InvalidObject",
                          "Could not get object %d from robot %d's world.",
                          objectID.GetValue(), _robot.GetID());
        return RESULT_FAIL;
      }
      
      ActionableObject* actionObject = dynamic_cast<ActionableObject*>(object);
      
      if(actionObject == nullptr) {
        PRINT_NAMED_ERROR("BehaviorOCD.SelectNextObjectToPickUp.NonActionObject",
                          "Could not cast %s object %d from robot %d's world as Actionable.",
                          ObjectTypeToString(object->GetType()),
                          object->GetID().GetValue(),
                          _robot.GetID());
        return RESULT_FAIL;
      }
      
      std::vector<PreActionPose> preActionPoses;
      actionObject->GetCurrentPreActionPoses(preActionPoses, {PreActionPose::DOCKING},
                                             std::set<Vision::Marker::Code>(),
                                             obstacles);
      
      if(!preActionPoses.empty()) {
        for(auto & preActionPose : preActionPoses)
        {
          Pose3d poseWrtRobot;
          if(false == preActionPose.GetPose().GetWithRespectTo(_robot.GetPose(), poseWrtRobot)) {
            PRINT_NAMED_ERROR("BehaviorOCD.SelectNextObjectToPickUp.PoseFailure",
                              "Could not get pre-action pose of %s object %d w.r.t. robot %d.",
                              ObjectTypeToString(object->GetType()),
                              object->GetID().GetValue(),
                              _robot.GetID());
            return RESULT_FAIL;
          }
          
          const f32 currentDistSq = poseWrtRobot.GetTranslation().LengthSq();
          if(currentDistSq < closestDistSq) {
            closestDistSq = currentDistSq;
            _objectToPickUp = objectID;
          }
        } // for each preAction pose
      } // if(!preActionPoses.empty())
      
    } // for each messy object ID
    
    if(_anchorObject.IsUnknown()) {
      _anchorObject = _objectToPickUp;
    }
    
    IActionRunner* pickupAction = new DriveToPickAndPlaceObjectAction(_objectToPickUp);
    _lastActionTag = pickupAction->GetTag();
    _robot.GetActionList().QueueActionNow(IBehavior::sActionSlot, pickupAction);

    _currentState = State::PickingUpBlock;
    
    if (nullptr == object) {
      PRINT_NAMED_WARNING("BehaviorOCD.SelectNextObjectToPickUp", "Invalid object selected!");
    }
    else {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.SelectNextObjectToPickUp", "Selected %s %d to pick up.",
                             ObjectTypeToString(object->GetType()),
                             _objectToPickUp.GetValue());
    }
    
    return RESULT_OK;
  } // SelectNextObjectToPickUp()
  
  
  void BehaviorOCD::ComputeAlignedPoses(const Pose3d& basePose, f32 distance, std::vector<Pose3d> &alignedPoses)
  {
    alignedPoses.clear();
    Pose3d alignedPose(basePose);
   
    // Test spot along +x axis
    Vec3f T(distance, 0, alignedPose.GetTranslation().z());
    Pose3d poseNextTo(0, Z_AXIS_3D(), T, basePose.GetParent());
    alignedPose.PreComposeWith(poseNextTo);
    alignedPoses.push_back(alignedPose);
    
    // Test spot along -x axis
    T.x() *= -1.f;
    poseNextTo.SetTranslation(T);
    alignedPose = basePose;
    alignedPose.PreComposeWith(poseNextTo);
    alignedPoses.push_back(alignedPose);
    
    // Test spot along +y axis
    T.x() = 0.f;
    T.y() = distance;
    poseNextTo.SetTranslation(T);
    alignedPose = basePose;
    alignedPose.PreComposeWith(poseNextTo);
    alignedPoses.push_back(alignedPose);
    
    // Test spot along -y axis
    T.y() *= -1.f;
    poseNextTo.SetTranslation(T);
    alignedPose = basePose;
    alignedPose.PreComposeWith(poseNextTo);
    alignedPoses.push_back(alignedPose);
  }
  
  
  // Return a pose "next to" the given object, along the x or y axes.
  // Simply fails if all of the 4 check locations (+/-x, +/-y) have objects in them.
  Result BehaviorOCD::FindEmptyPlacementPose(const ObjectID& nearObjectID, const f32 offset_mm, Pose3d& pose)
  {
    ObservableObject* nearObject = _robot.GetBlockWorld().GetObjectByID(nearObjectID);
    if(nearObject == nullptr) {
      PRINT_NAMED_ERROR("BehaviorOCD.FindEmptyPlacementPose.InvalidNearObjectID", "");
      return RESULT_FAIL;
    }
    
    BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "FindEmptyPlacementPose","Block pose: (%f, %f, %f)",
                           nearObject->GetPose().GetTranslation().x(),
                           nearObject->GetPose().GetTranslation().y(),
                           RAD_TO_DEG_F32(nearObject->GetPose().GetRotationAngle<'Z'>().ToFloat()));
    
    // Get all aligned poses
    std::vector<Pose3d> alignedPoses;
    ComputeAlignedPoses(nearObject->GetPose(), offset_mm, alignedPoses);
    
    // Find closest empty pose
    std::set<ObjectID> ignoreIDs;
    ignoreIDs.insert(_robot.GetCarryingObject());
    f32 closestDistToRobot = std::numeric_limits<f32>::max();
   
    for (auto& testPose : alignedPoses) {
      if(nullptr == _robot.GetBlockWorld().FindObjectClosestTo(testPose, nearObject->GetSize(), ignoreIDs)) {
        f32 distToRobot = ComputeDistanceBetween(_robot.GetPose(), testPose);
        if ( distToRobot < closestDistToRobot) {
          closestDistToRobot = distToRobot;
          pose = testPose;
        }
      }
      BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.FindEmptyPlacementPose.TestPose", "(%f %f %f) - %f",
                             testPose.GetTranslation().x(), testPose.GetTranslation().y(), RAD_TO_DEG_F32(testPose.GetRotationAngle<'Z'>().ToFloat()), closestDistToRobot);
    }
    
    // If no empty pose found, return fail
    if (closestDistToRobot == std::numeric_limits<f32>::max()) {
      return RESULT_FAIL;
    }

    BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "FindEmptyPlacementPose","Placement pose: (%f, %f, %f), distFromRobot %f",
                           pose.GetTranslation().x(), pose.GetTranslation().y(), RAD_TO_DEG_F32(pose.GetRotationAngle<'Z'>().ToFloat()), closestDistToRobot);
    
    return RESULT_OK;
  } // FindEmptyPlacementPose()
  
  
  Result BehaviorOCD::SelectNextPlacement()
  {
    BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.SelectNextPlacement", "");
    
    IActionRunner* placementAction = nullptr;
    _objectToPlaceOn.UnSet();
    
    ObjectID carriedObjectID;
    carriedObjectID = _robot.GetCarryingObject();
    
    if(carriedObjectID != _objectToPickUp) {
      PRINT_NAMED_WARNING("BehaviorOCD.SelectNextPlacement.UnexpectedID",
                          "Expecting object being carried to match object set to be picked up (carried %d, objectToPickup %d).",
                          carriedObjectID.GetValue(), _objectToPlaceOn.GetValue());
    }
    
    ObservableObject* carriedObject = _robot.GetBlockWorld().GetObjectByID(carriedObjectID);
    if(carriedObject == nullptr) {
      PRINT_NAMED_ERROR("BehaviorOCD.SelectNextPlacement.InvalidCarriedObject", "ID %d", carriedObjectID.GetValue());
      return RESULT_FAIL;
    }
    
    switch(_currentArrangement)
    {
      case Arrangement::StacksOfTwo:
      {
        // Find closest neat block that doesn't have a block on top
        if (GetClosestObjectInSet(_neatObjects, 0, ObjectOnTopStatus::NoObjectOnTop, _objectToPlaceOn) != RESULT_OK) {
          return RESULT_FAIL;
        }
        
        if(_objectToPlaceOn.IsSet()) {
          // Found a neat object with nothing on top. Stack on top of it:
          placementAction = new DriveToPickAndPlaceObjectAction(_objectToPlaceOn);
          BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.SelectNextPlacement.STACKS_OF_TWO", "Chose to place on top of object %d.",
                           _objectToPlaceOn.GetValue());
        } else {
          // ... if there isn't one, then place this block on the ground
          // at the same orientation as the last block placed.
          // If lastObjectPlacedOnGround is not set, it may be because this
          // is the first block to be placed on ground, or the last one was moved.
          // See if there exists a complete neat stack and set relative to that.
          
          // Find closest neat block that does have a block on top
          if (GetClosestObjectInSet(_neatObjects, 0, ObjectOnTopStatus::ObjectOnTop, _lastObjectPlacedOnGround) != RESULT_OK) {
            return RESULT_FAIL;
          }
          
          if(_lastObjectPlacedOnGround.IsSet()) {
            Pose3d pose;
            Result result = FindEmptyPlacementPose(_lastObjectPlacedOnGround, PLACEMENT_OFFSET_MM, pose);
            if(RESULT_OK != result) {
              PRINT_NAMED_ERROR("BehaviorOCD.SelectNextPlacement.NoEmptyPosesFound", "");
              return result;
            }

            // Place block at specified offset from lastObjectPlacedOnGround.
            placementAction = new DriveToPickAndPlaceObjectAction(_lastObjectPlacedOnGround,
                                                                  false,
                                                                  PLACEMENT_OFFSET_MM,
                                                                  0,
                                                                  0,
                                                                  true);

            BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.SelectNextPlacement.STACKS_OF_TWO",
                             "Placing object on ground at (%.1f,%.1f,%.1f) @ %.1fdeg (near object %d)",
                             pose.GetTranslation().x(), pose.GetTranslation().y(), pose.GetTranslation().z(),
                             pose.GetRotationAngle<'Z'>().getDegrees(),
                             _lastObjectPlacedOnGround.GetValue());
          } else {
            // This is the first object placed, just re-orient it to leave it in
            // the same position but aligned to the coordinate system (i.e. no rotation)
            Pose3d pose = carriedObject->GetPose().GetWithRespectToOrigin();
            pose.SetRotation(0, Z_AXIS_3D());
            
            placementAction = new PlaceObjectOnGroundAtPoseAction(_robot, pose);
            BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.SelectNextPlacement.STACKS_OF_TWO",
                             "Placing first object on ground at (%.1f,%.1f,%.1f) @ %.1fdeg",
                             pose.GetTranslation().x(),
                             pose.GetTranslation().y(),
                             pose.GetTranslation().z(),
                             pose.GetRotationAngle<'Z'>().getDegrees());
          }
          
          _lastObjectPlacedOnGround = carriedObjectID;
        } // if/else bottom block set
        
        break;
      } // case STACKS_OF_TWO
        
        
      case Arrangement::Line:
      {
        if(_lastObjectPlacedOnGround.IsSet()) {
          
          // TODO: Find closest available free space near the last object we placed on the ground

          Pose3d pose;
          Result result = FindEmptyPlacementPose(_lastObjectPlacedOnGround, PLACEMENT_OFFSET_MM, pose);
          if(RESULT_OK != result) {
            PRINT_NAMED_ERROR("BehaviorOCD.SelectNextPlacement.NoEmptyPosesFound", "");
            return result;
          }
          
          placementAction = new PlaceObjectOnGroundAtPoseAction(_robot, pose);
          BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.SelectNextPlacement.LINE",
                           "Placing object on ground at (%.1f,%.1f,%.1f) @ %.1fdeg (near object %d)",
                           pose.GetTranslation().x(), pose.GetTranslation().y(), pose.GetTranslation().z(),
                           pose.GetRotationAngle<'Z'>().getDegrees(),
                           _lastObjectPlacedOnGround.GetValue());
        } else {
          // This is the first object placed, just re-orient it to leave it in
          // the same position but aligned to the coordinate system (i.e. no rotation)
          Pose3d pose = carriedObject->GetPose().GetWithRespectToOrigin();
          pose.RotateBy(pose.GetRotation().GetInverse());
          
          placementAction = new PlaceObjectOnGroundAtPoseAction(_robot, pose);
          BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.SelectNextPlacement.LINE",
                           "Placing first object on ground at (%.1f,%.1f,%.1f) @ %.1fdeg",
                           pose.GetTranslation().x(),
                           pose.GetTranslation().y(),
                           pose.GetTranslation().z(),
                           pose.GetRotationAngle<'Z'>().getDegrees());
        }
        
        _lastObjectPlacedOnGround = carriedObjectID;
        break;
      } // case LINE
        
        
      default:
        PRINT_NAMED_ERROR("BehaviorOCD.SelectNextPlacement.UnknownArrangment", "");
        return RESULT_FAIL;
        
    } // switch(_currentArrangement)
    
    if(placementAction == nullptr) {
      // TODO: Error message (necessary? Queueing command below will also catch it...)
      return RESULT_FAIL;
    }
    
    _currentState = State::PlacingBlock;
    
    _lastActionTag = placementAction->GetTag();
    Result queueResult = _robot.GetActionList().QueueActionNow(IBehavior::sActionSlot, placementAction);
    
    return queueResult;
  } // SelectNextPlacement()
  

  // TODO: Move this to blockworld?
  // Returns RESULT_FAIL if there are non-existant objects in the set.
  // objectID is unset if none of the object in set met criteria.
  Result BehaviorOCD::GetClosestObjectInSet(const std::set<ObjectID>& objectSet, u8 heightLevel, ObjectOnTopStatus ootStatus, ObjectID& objectID)
  {
    BlockWorld& blockWorld = _robot.GetBlockWorld();
    objectID.UnSet();
    
    // Pick the closest neatened object that's on the ground without anything on top yet...
    f32 closestDistSq = std::numeric_limits<f32>::max();
    for(auto & objID : objectSet) {
      ObservableObject* oObject = blockWorld.GetObjectByID(objID);
      if(oObject == nullptr) {
        PRINT_NAMED_ERROR("BehaviorOCD.GetClosestObject.InvalidObjectID",
                          "No object ID = %d", objID.GetValue());
        return RESULT_FAIL;
      }
      
      // Get pose of this neat object w.r.t. robot pose
      Pose3d poseWrtRobot;
      if(false == oObject->GetPose().GetWithRespectTo(_robot.GetPose(), poseWrtRobot)) {
        PRINT_NAMED_ERROR("BehaviorOCD.GetClosestObject.PoseFail",
                          "Could not get object %d's pose w.r.t. robot.",
                          oObject->GetID().GetValue());
        return RESULT_FAIL;
      }
      
      // Make sure it's approximately at the same height as the robot
      if(NEAR(poseWrtRobot.GetTranslation().z(), oObject->GetSize().z()*(heightLevel + 0.5f), 10.f)) {
        // See if there's anything on top of it
        ObservableObject* onTop = blockWorld.FindObjectOnTopOf(*oObject, 15.f);
        if((ootStatus == ObjectOnTopStatus::DontCareIfObjectOnTop) || (ootStatus == ObjectOnTopStatus::ObjectOnTop && onTop != nullptr) || (ootStatus == ObjectOnTopStatus::NoObjectOnTop && onTop == nullptr)) {
          const f32 currentDistSq = poseWrtRobot.GetTranslation().LengthSq();
          if(currentDistSq < closestDistSq) {
            closestDistSq = currentDistSq;
            objectID = objID;
          }
        }
      }
    } // for each neat object
    
    return RESULT_OK;
  }
  

  
  void BehaviorOCD::SetBlockLightState(const std::set<ObjectID>& objectSet,  BlockLightState state)
  {
    for (auto& objID : objectSet)
    {
      SetBlockLightState(objID, state);
    }
  }
  
  void BehaviorOCD::SetBlockLightState(const ObjectID& objID, BlockLightState state)
  {
    switch(state) {
      case BlockLightState::Messy:
        // TODO: Get "messy" color and on/off settings from config
        _robot.SetObjectLights(objID, WhichBlockLEDs::ALL, NamedColors::RED, NamedColors::BLACK, 200, 200, 50, 50, false, MakeRelativeMode::RELATIVE_LED_MODE_OFF, {});
        break;
      case BlockLightState::Neat:
        // TODO: Get "neat" color and on/off settings from config
        _robot.SetObjectLights(objID, WhichBlockLEDs::ALL, NamedColors::CYAN, NamedColors::BLACK, 10, 10, 2000, 2000, false, MakeRelativeMode::RELATIVE_LED_MODE_OFF, {});
        break;
      case BlockLightState::Complete:
        _robot.SetObjectLights(objID, WhichBlockLEDs::ALL, NamedColors::GREEN, NamedColors::GREEN, 200, 200, 50, 50, false, MakeRelativeMode::RELATIVE_LED_MODE_OFF, {});
        break;
      default:
        break;
    }
  }
  
  void BehaviorOCD::UpdateBlockLights()
  {
    if (_messyObjects.empty()) {
      for (auto& obj : _neatObjects) {
        SetBlockLightState(obj, BlockLightState::Complete);
      }
    } else {
      for (auto& obj : _neatObjects) {
        SetBlockLightState(obj, BlockLightState::Neat);
      }
      for (auto& obj : _messyObjects) {
        SetBlockLightState(obj, BlockLightState::Messy);
      }
    }
  }
  
  
  // If they have roughly the same neatness score and their poses are roughly axis aligned at
  // distance of approximately two block widths
  bool BehaviorOCD::AreAligned(const ObjectID& o1, const ObjectID& o2)
  {
    f32 scoreDiff = fabs(GetNeatnessScore(o1) - GetNeatnessScore(o2));
    if (scoreDiff > 0.1) {
      return false;
    }
    
    BlockWorld& blockWorld = _robot.GetBlockWorld();
    ObservableObject* obj1 = blockWorld.GetObjectByID(o1);
    ObservableObject* obj2 = blockWorld.GetObjectByID(o2);
    
    // Find aligned poses
    std::vector<Pose3d> alignedPoses;
    ComputeAlignedPoses(obj1->GetPose(), 2*obj1->GetSize().x(), alignedPoses);
    
    // Check if object 2 is at any of them
    for (auto& testPose : alignedPoses) {
      if(obj2 == blockWorld.FindObjectClosestTo(testPose, obj1->GetSize() * 0.5f)) {
        return true;
      }
      //BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.AreAligned.TestPose", "(%f %f %f)",
      //                       testPose.GetTranslation().x(), testPose.GetTranslation().y(), RAD_TO_DEG_F32(testPose.GetRotationAngle<'Z'>().ToFloat()));
    }

    return false;
    
  }
  
  
  // Verify that objects observed in the last second are in the correct state (i.e. neat or messy)
  void BehaviorOCD::VerifyNeatness()
  {
    switch(_currentArrangement)
    {
      case Arrangement::StacksOfTwo:
      {
        const f32 MIN_NEATNESS_SCORE = 0.7f;
        const f32 RECENTLY_OBSERVED_TIME_THRESH_MS = 1000.f;
        
        TimeStamp_t lastMsgRecvdTime = _robot.GetLastMsgTimestamp();
        const BlockWorld& blockWorld = _robot.GetBlockWorld();

        // A messy block can become a neat block if it has a high neatness score and it's
        // 1) on top of a neat block, or
        // 2) under a neat block, or
        // 2) aligned with a neat block from another stack
        bool foundMessyToNeat = false;
        for (auto objID = _messyObjects.begin(); objID != _messyObjects.end(); ) {
          ObservableObject* oObject = blockWorld.GetObjectByID(*objID);
          if (oObject == nullptr) {
            PRINT_NAMED_ERROR("BehaviorOCD.VerifyNeatness.InvalidObject","Object %d", objID->GetValue());
          }
          else if (lastMsgRecvdTime - oObject->GetLastObservedTime() < RECENTLY_OBSERVED_TIME_THRESH_MS) {
          
            f32 score = GetNeatnessScore(*objID);
            if (score > MIN_NEATNESS_SCORE) {
              // 1) Check if on top of a neat block
              
              // Get pose of this neat object w.r.t. robot pose
              Pose3d poseWrtRobot;
              if(false == oObject->GetPose().GetWithRespectTo(_robot.GetPose(), poseWrtRobot)) {
                PRINT_NAMED_ERROR("BehaviorOCD.VerifyNeatness.PoseFail",
                                  "Could not get object %d's pose w.r.t. robot.",
                                  oObject->GetID().GetValue());
                ++objID;
                continue;
              }
              
              // Make sure it's approximately at high dock height
              if(NEAR(poseWrtRobot.GetTranslation().z(), oObject->GetSize().z()*(1.5f), 10.f)) {
                
                // If so look for a nearby neat block below it.
                Vec3f distThresh(0.5f * oObject->GetSize().x(),
                                 0.5f * oObject->GetSize().y(),
                                 1.5f * oObject->GetSize().z());
                std::set<ObjectID> ignoreIDs;
                ignoreIDs.insert(*objID);
                ObservableObject* objBelow = blockWorld.FindObjectClosestTo(oObject->GetPose(), distThresh, ignoreIDs);
                if (objBelow == nullptr) {
                  ++objID;
                  continue;
                }
                
                Vec3f vecBetween = ComputeVectorBetween( objBelow->GetPose(), oObject->GetPose());
                vecBetween.Abs();
                if (vecBetween.x() < 0.5f*(oObject->GetSize().x()) &&
                    vecBetween.y() < 0.5f*(oObject->GetSize().y()) &&
                    vecBetween.z() > 0.5f*(oObject->GetSize().z()) ) {
                  PRINT_NAMED_INFO("BehaviorOCD.VerifyNeatness.MessyToNeat(1)", "Object %d", objID->GetValue());
                  foundMessyToNeat = true;
                }
              }

              
              // 2) Check if block on top is neat
              // 3) Check if aligned with a neat block from another stack
              if (!foundMessyToNeat) {
                for (auto& neatBlockID : _neatObjects) {
                  ObservableObject* objectOnTop = blockWorld.FindObjectOnTopOf(*oObject, 15.f);
                  if ((objectOnTop && (objectOnTop->GetID() == neatBlockID)) ||
                      AreAligned(neatBlockID, *objID)) {
                    PRINT_NAMED_INFO("BehaviorOCD.VerifyNeatness.MessyToNeat(2)", "Object %d", objID->GetValue());
                    foundMessyToNeat = true;
                    break;
                  }
                }
              }
              
              if (foundMessyToNeat) {
                if ((_currentState == State::PickingUpBlock) && (*objID == _objectToPickUp)) {
                  // Robot is trying to pickup the messy block that we're about to make clean,
                  // so cancel it.
                  _robot.GetActionList().Cancel(_lastActionTag);
                }
                _neatObjects.insert(*objID);
                objID = _messyObjects.erase(objID);
                UpdateBlockLights();
                continue;
              }
            }
          }
          ++objID;
        }
        
        // A neat block can become messy if it has a low neatness score
        for (auto objID = _neatObjects.begin(); objID != _neatObjects.end(); ) {
          ObservableObject* oObject = blockWorld.GetObjectByID(*objID);
          if (oObject == nullptr) {
            PRINT_NAMED_ERROR("BehaviorOCD.VerifyNeatness.InvalidObject","Object %d", objID->GetValue());
            continue;
          }
          else if (lastMsgRecvdTime - oObject->GetLastObservedTime() < RECENTLY_OBSERVED_TIME_THRESH_MS) {
            
            f32 score = GetNeatnessScore(*objID);
            if (score < MIN_NEATNESS_SCORE) {
              PRINT_NAMED_INFO("BehaviorOCD.VerifyNeatness.NeatToMessy", "Object %d, score %f", objID->GetValue(), score);
              if ((_currentState == State::PlacingBlock) && (*objID == _objectToPlaceOn)) {
                // Robot is trying to place a block on neat block that we're about to make messy,
                // so cancel it.
                _robot.GetActionList().Cancel(_lastActionTag);
              }
              _messyObjects.insert(*objID);
              objID = _neatObjects.erase(objID);
              UpdateBlockLights();
              continue;
            }
          }
          ++objID;
        }
        
        break;
      } // end case Arrangement::StacksOfTwo
        
      case Arrangement::Line:
      {
        // TODO...
        PRINT_NAMED_ERROR("BehaviorOCD.VerifyNeatness.Line", "TO BE IMPLEMENTED");
        break;
      }
      default:
        PRINT_NAMED_ERROR("BehaviorOCD.VerifyNeatness.InvalidState", "");
        break;
 
    }  // end switch
    
  }
  
  void BehaviorOCD::UnsetLastPickOrPlaceFailure()
  {
    _lastObjectFailedToPickOrPlace.UnSet();
  }
  
  void BehaviorOCD::SetLastPickOrPlaceFailure(const ObjectID& objectID, const Pose3d& pose)
  {
    _lastObjectFailedToPickOrPlace = objectID;
    _lastPoseWhereFailedToPickOrPlace = pose;
  }
  
  void BehaviorOCD::DeleteObjectIfFailedToPickOrPlaceAgain(const ObjectID& objectID)
  {
    const f32 SAME_POSE_DIST_THRESH_MM = 5;
    if (objectID.IsSet() &&
        (_lastObjectFailedToPickOrPlace == objectID) &&
        ComputeDistanceBetween(_robot.GetPose(), _lastPoseWhereFailedToPickOrPlace) < SAME_POSE_DIST_THRESH_MM) {
      PRINT_NAMED_INFO("BehaviorOCD::DeleteObjectIfFailedToPickOrPlaceAgain", "Deleting object %d", objectID.GetValue());
      _robot.GetBlockWorld().ClearObject(objectID);
    }
  }
  
  
  Result BehaviorOCD::HandleActionCompleted(const ExternalInterface::RobotCompletedAction &msg)
  {
    Result lastResult = RESULT_OK;
    
    if(!IsRunning()) {
      // Ignore action completion messages while not runningd
      return lastResult;
    }
    
    switch(_currentState)
    {
      case State::PickingUpBlock:
      {
        if(msg.result == ActionResult::SUCCESS) {
          switch(msg.actionType) {
              
            case RobotActionType::PICKUP_OBJECT_HIGH:
            case RobotActionType::PICKUP_OBJECT_LOW:
              BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.HandleActionCompleted.PickupSuccessful", "");
              // We're done picking up the block, figure out where to put it
              UnsetLastPickOrPlaceFailure();
              SelectNextPlacement();
              _currentState = State::PlacingBlock;
              break;
              
            case RobotActionType::PICK_AND_PLACE_INCOMPLETE:
              // We failed to pick up or place the last block, try again?
              DeleteObjectIfFailedToPickOrPlaceAgain(_objectToPickUp);
              SetLastPickOrPlaceFailure(_objectToPickUp, _robot.GetPose());
              SelectNextObjectToPickUp();
              break;
              
            default:
              // Simply ignore other action completions?
              break;
              
          } // switch(actionType)
        } else {
          BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.HandleActionCompleted.PickupFailure", "Trying again");
          // We failed to pick up or place the last block, try again?
          DeleteObjectIfFailedToPickOrPlaceAgain(_objectToPickUp);
          SetLastPickOrPlaceFailure(_objectToPickUp, _robot.GetPose());
          lastResult = SelectNextObjectToPickUp();
        }
        break;
      } // case PickingUpBlock
        
      case State::PlacingBlock:
      {
        if(msg.result == ActionResult::SUCCESS) {
          switch(msg.actionType) {
            case RobotActionType::PLACE_OBJECT_LOW:
              _lastObjectPlacedOnGround = _objectToPickUp;
              // NOTE: deliberate fallthrough to next case!
            case RobotActionType::PLACE_OBJECT_HIGH:
              BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.HandleActionCompleted.PlacementSuccessful",
                               "Placed object %d, moving from messy to neat.",
                               _objectToPickUp.GetValue());
              
              // We're done placing the block, mark it as neat and move to next one
              _messyObjects.erase(_objectToPickUp);
              _neatObjects.insert(_objectToPickUp);
              
              UpdateBlockLights();
              
              lastResult = SelectNextObjectToPickUp();
              UnsetLastPickOrPlaceFailure();
              _currentState = State::PickingUpBlock;
              break;
              
            case RobotActionType::PICK_AND_PLACE_INCOMPLETE:
              // We failed to place the last block, try again?
              DeleteObjectIfFailedToPickOrPlaceAgain(_objectToPlaceOn);
              SetLastPickOrPlaceFailure(_objectToPlaceOn, _robot.GetPose());
              lastResult = SelectNextPlacement();
              break;
              
            default:
              // Simply ignore other action completions?
              break;
          }
        } else {
          BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.HandleActionCompleted.PlacementFailure", "Trying again");
          // We failed to place the last block, try again?
          DeleteObjectIfFailedToPickOrPlaceAgain(_objectToPlaceOn);
          SetLastPickOrPlaceFailure(_objectToPlaceOn, _robot.GetPose());
          lastResult = SelectNextPlacement();
        }
        break;
      } // case PlacingBlock
      case State::Irritated:
      {
          switch(msg.actionType) {
            case RobotActionType::PLAY_ANIMATION:
              if (msg.idTag == _lastActionTag) {
                BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.HandleActionCompleted.IrritatedAnimComplete", "");
              
                  if (_irritationLevel > 1) {
                    PlayAnimation("VeryIrritated");
                  } else {
                  _robot.IsCarryingObject() ? SelectNextPlacement() : SelectNextObjectToPickUp();
                  }
                  _irritationLevel = 0;
              
              } else {
                BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.HandleActionCompleted.UnknownAnimCompleted", "");
              }
              break;
            default:
              BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.HandleActionCompleted.UnexpectedActionCompleteDuringIrritated", "action %d", msg.actionType);
              break;
          }

      }
    } // switch(_currentState)
    
    UpdateName();
    
    return lastResult;
  } // HandleActionCompleted()
  
  
  Result BehaviorOCD::HandleObjectMoved(const ExternalInterface::ActiveObjectMoved &msg)
  {
    Result lastResult = RESULT_OK;
    
    // If a previously-neat object is moved (and it's not the one we are stacking we are
    // stacking _on_, since we might be the one to move it), move it to the messy list
    // and play some kind of irritated animation
    ObjectID objectID;
    objectID = msg.objectID;
    
    if ((_neatObjects.count(objectID)>0) && (objectID == _objectToPlaceOn) && !_robot.IsPickingOrPlacing()) {
      PRINT_NAMED_INFO("HandleObjectMoved.ResetObjectToPlaceOn", "id %d", _objectToPlaceOn.GetValue());
      _objectToPlaceOn.UnSet();
    }
    
    if(_neatObjects.count(objectID)>0 && objectID != _objectToPlaceOn)
    {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.HandleObjectMoved", "Neat object %d moved, making messy.", msg.objectID);
      
      if(_lastObjectPlacedOnGround == objectID) {
        BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.HandleObjectMoved", "About to make last object on ground messy, clearing.");
        _lastObjectPlacedOnGround.UnSet();
      }
      
      _neatObjects.erase(objectID);
      _messyObjects.insert(objectID);

      UpdateBlockLights();
      
      // Increase irritation level
      ++_irritationLevel;
      
      if (_currentState != State::Irritated) {
        // Queue irritated animation
        PlayAnimation("MinorIrritation");
        
        _currentState = State::Irritated;
        UpdateName();
      }

    }
    
    return lastResult;
  }
  
  
  Result BehaviorOCD::HandleObservedObject(const ExternalInterface::RobotObservedObject &msg)
  {
    // if the object is a BLOCK or ACTIVE_BLOCK, add its ID to the list we care about
    // iff we haven't already seen and neatened it (if it's in our neat list,
    // we won't take it out unless we detect that it was moved)
    ObjectID objectID;
    objectID = msg.objectID;

    if(_neatObjects.count(objectID) == 0) {
      std::pair<std::set<ObjectID>::iterator,bool> insertResult = _messyObjects.insert(objectID);
      
      if(insertResult.second) {
        BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.HandleObservedObject",
                         "Adding observed object %d to messy list.", msg.objectID);
      }
      
    }
    
    
    VerifyNeatness();
    
    return RESULT_OK;
  }
  
  Result BehaviorOCD::HandleDeletedObject(const ExternalInterface::RobotDeletedObject &msg)
  {
    // remove the object if we knew about it
    ObjectID objectID;
    objectID = msg.objectID;

    BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.HandleDeletedObject", "Deleting object %d from messy & neat lists.", msg.objectID);
    
    if(_lastObjectPlacedOnGround == objectID) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.HandleDeletedObject", "About to delete last object on ground, clearing.");
      _lastObjectPlacedOnGround.UnSet();
    }
    
    if(_objectToPickUp == objectID) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.HandleDeletedObject", "About to delete object to pickup, clearing.");
      _objectToPickUp.UnSet();
    }

    if(_objectToPlaceOn == objectID) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.HandleDeletedObject", "About to delete object to place on, clearing.");
      _objectToPlaceOn.UnSet();
    }
    
    
    _messyObjects.erase(objectID);
    _neatObjects.erase(objectID);
    
    return RESULT_OK;
  }
  
  f32 BehaviorOCD::GetNeatnessScore(const ObjectID& whichObject)
  {
    ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(whichObject);
    if(object == nullptr) {
      PRINT_NAMED_ERROR("BehaviorOCD.GetNeatnessScore.InvalidObject",
                        "Could not get object %d from robot %d's world.",
                        whichObject.GetValue(), _robot.GetID());
      return 0.f;
    }
    
    // See how far away the object is from being aligned with the coordinate axes
    f32 angle = std::fabs(object->GetPose().GetRotationAngle<'Z'>().getDegrees());
    
    f32 diffFrom90deg = std::fmod(angle, 90.f);
    diffFrom90deg = MIN(diffFrom90deg, 90 - diffFrom90deg);
    
    const f32 score = 1.f - diffFrom90deg/45.f;
    return score;
  }
  
  
  void BehaviorOCD::PlayAnimation(const std::string& animName)
  {
    BEHAVIOR_VERBOSE_PRINT(DEBUG_OCD_BEHAVIOR, "BehaviorOCD.PlayAnimation", "%s", animName.c_str());
    PlayAnimationAction* animAction = new PlayAnimationAction(animName.c_str());
    _lastActionTag = animAction->GetTag();
    _robot.GetActionList().QueueActionNow(IBehavior::sActionSlot, animAction);
  }
  
} // namespace Cozmo
} // namespace Anki