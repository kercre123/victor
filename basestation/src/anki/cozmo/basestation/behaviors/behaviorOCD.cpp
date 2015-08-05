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

//TODO: Remove once Lee's Events are in
#include "tempSignals.h"

#define DEBUG_OCD_BEHAVIOR 1

namespace Anki {
namespace Cozmo {
  
  BehaviorOCD::BehaviorOCD(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _currentArrangement(Arrangement::STACKS_OF_TWO)
  {
    
    //TODO: Remove once Lee's Events are in
    auto cbActionCompleted = [this](ExternalInterface::MessageEngineToGame msg) {
      this->HandleActionCompleted(msg.Get_RobotCompletedAction());
    };
    _signalHandles.emplace_back(CozmoEngineSignals::RobotCompletedActionSignal().ScopedSubscribe(cbActionCompleted));
    
    auto cbObservedObject = [this](ExternalInterface::MessageEngineToGame msg) {
      this->HandleObservedObject(msg.Get_RobotObservedObject());
    };
    _signalHandles.emplace_back(CozmoEngineSignals::RobotObservedObjectSignal().ScopedSubscribe(cbObservedObject));
    
    auto cbObjectDeleted = [this](ExternalInterface::MessageEngineToGame msg) {
      this->HandleDeletedObject(msg.Get_RobotDeletedObject());
    };
    _signalHandles.emplace_back(CozmoEngineSignals::RobotDeletedObjectSignal().ScopedSubscribe(cbObjectDeleted));
    
    auto cbObjectMoved = [this](ExternalInterface::MessageEngineToGame msg) {
      this->HandleObjectMoved(msg.Get_ActiveObjectMoved());
    };
    _signalHandles.emplace_back(CozmoEngineSignals::ObjectMovedSignal().ScopedSubscribe(cbObjectMoved));
    
  }
  
  
#pragma mark -
#pragma mark Inherited Virtual Implementations
  
  bool BehaviorOCD::IsRunnable() const
  {
    // We can only run this behavior when there are "messy" blocks present
    return !_messyObjects.empty();
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
        // ... if so, start in PLACING_BLOCK mode
        _currentState = State::PLACING_BLOCK;
        lastResult = SelectNextPlacement();
      } else {
        // ... if not, put this thing down and start in PICKING_UP_BLOCK mode
        _currentState = State::PICKING_UP_BLOCK;
        
        // TODO: Find a good place to put down this object
        // For now, just put it down right _here_
        lastResult = _robot.GetActionList().QueueActionNow(IBehavior::sActionSlot, new PlaceObjectOnGroundAction());
        if(lastResult != RESULT_OK) {
          PRINT_NAMED_ERROR("BehaviorOCD.Init.PlacementFailed",
                            "Failed to queue PlaceObjectOnGroundAction.\n");
          return lastResult;
        }
        
        lastResult = SelectNextObjectToPickUp();
      }
    } else {
      lastResult = SelectNextObjectToPickUp();
    }
    
    lastResult = SelectArrangement();
    
    return lastResult;
  } // Init()

  
  IBehavior::Status BehaviorOCD::Update(float currentTime_sec)
  {
    // Completion trigger is when all (?) blocks make it to his "neat" list
    if(_messyObjects.empty()) {
      return Status::COMPLETE;
    }
    
    switch(_currentState)
    {
      case State::PICKING_UP_BLOCK:
        if(_interrupted) {
          // If we are in the middle of picking up a block, we can immediately interrupt.
          // Otherwise we will wait until placing completes which switches us back to
          // PICKING_UP_BLOCK, so we can then get here
          return Status::COMPLETE;
        }
        break;
        
      case State::PLACING_BLOCK:
        
        break;
        
      default:
        PRINT_NAMED_ERROR("OCD_Behavior.Update.UnknownState",
                          "Reached unknown state %d.\n", _currentState);
        return Status::FAILURE;
    }
    
    return Status::RUNNING;
  }
  
  Result BehaviorOCD::Interrupt()
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
  
  Result BehaviorOCD::SelectArrangement()
  {
    // TODO: Make this based on something "smarter"
    // For now, just rotate between available arrangments
    s32 iArrangement = static_cast<s32>(_currentArrangement);
    ++iArrangement;
    if(iArrangement == static_cast<s32>(Arrangement::NUM_ARRANGEMENTS)) {
      iArrangement = 0;
    }
    _currentArrangement = static_cast<Arrangement>(iArrangement);
    
#   if DEBUG_OCD_BEHAVIOR
    PRINT_NAMED_INFO("BehaviorOCD.SelectArrangement", "Selected arrangment %d\n", iArrangement);
#   endif
    
    // we'll select the arrangement's anchor once we place our first block
    _anchorObject.UnSet();
    
    _lastObjectPlacedOnGround.UnSet();
    
    return RESULT_OK;
  }
  
  Result BehaviorOCD::SelectNextObjectToPickUp()
  {
    if(_messyObjects.empty()) {
      PRINT_NAMED_ERROR("BehaviorOCD.SelectNextObjectToPickUp.NoMessyObjects", "\n");
      return RESULT_FAIL;
    }
    
    BlockWorld& blockWorld = _robot.GetBlockWorld();
    
    std::vector<std::pair<Quad2f, ObjectID> > obstacles;
    blockWorld.GetObstacles(obstacles);
    
    // Find the closest object with available pre-action poses
    f32 closestDistSq = std::numeric_limits<f32>::max();
    Vision::ObservableObject* object = nullptr;
    for(auto & objectID : _messyObjects)
    {
      object = blockWorld.GetObjectByID(objectID);
      if(object == nullptr) {
        PRINT_NAMED_ERROR("BehaviorOCD.SelectNextObjectToPickUp.InvalidObject",
                          "Could not get %s object %d from robot %d's world.\n",
                          object->GetType().GetName().c_str(),
                          object->GetID().GetValue(),
                          _robot.GetID());
        return RESULT_FAIL;
      }
      
      ActionableObject* actionObject = dynamic_cast<ActionableObject*>(object);
      
      if(actionObject == nullptr) {
        PRINT_NAMED_ERROR("BehaviorOCD.SelectNextObjectToPickUp.NonActionObject",
                          "Could not cast %s object %d from robot %d's world as Actionable.\n",
                          object->GetType().GetName().c_str(),
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
                              "Could not get pre-action pose of %s object %d w.r.t. robot %d.\n",
                              object->GetType().GetName().c_str(),
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
    
    _robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot,
                                            new DriveToPickAndPlaceObjectAction(_objectToPickUp));
    
    _currentState = State::PICKING_UP_BLOCK;
    
#   if DEBUG_OCD_BEHAVIOR
    assert(object != nullptr);
    PRINT_NAMED_INFO("BehaviorOCD.SelectNextObjectToPickUp", "Selected %s %d to pick up.\n",
                     object->GetType().GetName().c_str(),
                     _objectToPickUp.GetValue());
#   endif
    
    return RESULT_OK;
  } // SelectNextObjectToPickUp()
  
  Result BehaviorOCD::SelectNextPlacement()
  {
    IActionRunner* placementAction = nullptr;
    
    ObjectID carriedObjectID;
    carriedObjectID = _robot.GetCarryingObject();
    
    if(carriedObjectID != _objectToPickUp) {
      PRINT_NAMED_WARNING("BehaviorOCD.SelectNextPlacement.UnexpectedID",
                          "Expecting object being carried to match object set to be picked up.\n");
    }
    
    Vision::ObservableObject* carriedObject = _robot.GetBlockWorld().GetObjectByID(carriedObjectID);
    if(carriedObject == nullptr) {
      PRINT_NAMED_ERROR("BehaviorOCD.SelectNextPlacement.InvalidCarriedObject", "\n");
      return RESULT_FAIL;
    }
    
    switch(_currentArrangement)
    {
      case Arrangement::STACKS_OF_TWO:
      {
        BlockWorld& blockWorld = _robot.GetBlockWorld();
        
        // Pick the closest neatened block without anything on top yet...
        ObjectID bottomBlockID;
        assert(bottomBlockID.IsUnknown());
        f32 closestDistSq = std::numeric_limits<f32>::max();
        for(auto & neatObjectID : _neatObjects) {
          Vision::ObservableObject* neatObject = blockWorld.GetObjectByID(neatObjectID);
          if(neatObject == nullptr) {
            PRINT_NAMED_ERROR("BehaviorOCD.SelectNextPlacement.InvalidObjectID",
                              "No object ID = %d\n", neatObjectID.GetValue());
            return RESULT_FAIL;
          }
          Vision::ObservableObject* onTop = blockWorld.FindObjectOnTopOf(*neatObject, 15.f);
          if(onTop == nullptr) {
            Pose3d poseWrtRobot;
            if(false == neatObject->GetPose().GetWithRespectTo(_robot.GetPose(), poseWrtRobot)) {
              PRINT_NAMED_ERROR("BehaviorOCD.SelectNextPlacement.PoseFail",
                                "Could not get object %d's pose w.r.t. robot.\n",
                                neatObject->GetID().GetValue());
              return RESULT_FAIL;
            }
            const f32 currentDistSq = poseWrtRobot.GetTranslation().LengthSq();
            if(currentDistSq < closestDistSq) {
              closestDistSq = currentDistSq;
              bottomBlockID = neatObjectID;
            }
          }
        } // for each neat object
        
        if(bottomBlockID.IsSet()) {
          // Found a neat object with nothing on top. Stack on top of it:
          placementAction = new PickAndPlaceObjectAction(bottomBlockID);
#         if DEBUG_OCD_BEHAVIOR
          PRINT_NAMED_INFO("BehaviorOCD.SelectNextPlacement.STACKS_OF_TWO", "Chose to place on top of object %d.\n",
                           bottomBlockID.GetValue());
#         endif
        } else {
          // ... if there isn't one, then place this block on the ground
          // at the same orientation as the last block placed
          if(_lastObjectPlacedOnGround.IsSet()) {
            Pose3d pose;
            
            // TODO: Find closest available free space near the last object we placed on the ground
            Vision::ObservableObject* lastObject = blockWorld.GetObjectByID(_lastObjectPlacedOnGround);
            pose = lastObject->GetPose();
            Vec3f T = pose.GetTranslation();
            T.x() += 3*lastObject->GetSameDistanceTolerance().x();
            pose.SetTranslation(T);
            
            placementAction = new PlaceObjectOnGroundAtPoseAction(_robot, pose);
#           if DEBUG_OCD_BEHAVIOR
            PRINT_NAMED_INFO("BehaviorOCD.SelectNextPlacement.STACKS_OF_TWO",
                             "Placing object on ground at (%.1f,%.1f,%.1f) @ %.1fdeg (near object %d)\n",
                             T.x(), T.y(), T.z(), pose.GetRotationAngle<'Z'>().getDegrees(),
                             _lastObjectPlacedOnGround.GetValue());
#           endif
          } else {
            // This is the first object placed, just re-orient it in the same position
            Pose3d pose = carriedObject->GetPose();
            f32 angle_deg = pose.GetRotationAngle<'Z'>().getDegrees();
            if(angle_deg < 45 && angle_deg >= -45) {
              angle_deg = 0;
            } else if(angle_deg < 135 && angle_deg >= 45) {
              angle_deg = 90;
            } else if(angle_deg < -45 && angle_deg >= -135) {
              angle_deg = -90;
            } else {
              angle_deg = 180;
            }
            pose.SetRotation(DEG_TO_RAD(angle_deg), Z_AXIS_3D());
            
            placementAction = new PlaceObjectOnGroundAtPoseAction(_robot, pose);
#           if DEBUG_OCD_BEHAVIOR
            PRINT_NAMED_INFO("BehaviorOCD.SelectNextPlacement.STACKS_OF_TWO",
                             "Placing first object on ground at (%.1f,%.1f,%.1f) @ %.1fdeg\n",
                             pose.GetTranslation().x(),
                             pose.GetTranslation().y(),
                             pose.GetTranslation().z(),
                             pose.GetRotationAngle<'Z'>().getDegrees());
#           endif
          }
          
          _lastObjectPlacedOnGround = carriedObjectID;
        } // if/else bottom block set
        
        break;
      } // case STACKS_OF_TWO
        
        
      case Arrangement::LINE:
      {
        if(_lastObjectPlacedOnGround.IsSet()) {
          Pose3d pose;
          
          // TODO: Find closest available free space near the last object we placed on the ground
          Vision::ObservableObject* lastObject = _robot.GetBlockWorld().GetObjectByID(_lastObjectPlacedOnGround);
          pose = lastObject->GetPose();
          Vec3f T = pose.GetTranslation();
          T.x() += 3*lastObject->GetSameDistanceTolerance().x();
          pose.SetTranslation(T);
          
          placementAction = new PlaceObjectOnGroundAtPoseAction(_robot, pose);
#         if DEBUG_OCD_BEHAVIOR
          PRINT_NAMED_INFO("BehaviorOCD.SelectNextPlacement.LINE",
                           "Placing object on ground at (%.1f,%.1f,%.1f) @ %.1fdeg (near object %d)\n",
                           T.x(), T.y(), T.z(), pose.GetRotationAngle<'Z'>().getDegrees(),
                           _lastObjectPlacedOnGround.GetValue());
#         endif
        } else {
          // This is the first object placed, just re-orient it in the same position
          Pose3d pose = carriedObject->GetPose();
          f32 angle_deg = pose.GetRotationAngle<'Z'>().getDegrees();
          if(angle_deg < 45 && angle_deg >= -45) {
            angle_deg = 0;
          } else if(angle_deg < 135 && angle_deg >= 45) {
            angle_deg = 90;
          } else if(angle_deg < -45 && angle_deg >= -135) {
            angle_deg = -90;
          } else {
            angle_deg = 180;
          }
          pose.SetRotation(DEG_TO_RAD(angle_deg), Z_AXIS_3D());
          
          placementAction = new PlaceObjectOnGroundAtPoseAction(_robot, pose);
#         if DEBUG_OCD_BEHAVIOR
          PRINT_NAMED_INFO("BehaviorOCD.SelectNextPlacement.LINE",
                           "Placing first object on ground at (%.1f,%.1f,%.1f) @ %.1fdeg\n",
                           pose.GetTranslation().x(),
                           pose.GetTranslation().y(),
                           pose.GetTranslation().z(),
                           pose.GetRotationAngle<'Z'>().getDegrees());
#         endif
        }
        
        _lastObjectPlacedOnGround = carriedObjectID;
        break;
      } // case LINE
        
        
      default:
        PRINT_NAMED_ERROR("BehaviorOCD.SelectNextPlacement.UnknownArrangment", "\n");
        return RESULT_FAIL;
        
    } // switch(_currentArrangement)
    
    if(placementAction == nullptr) {
      // TODO: Error message (necessary? Queueing command below will also catch it...)
      return RESULT_FAIL;
    }
    
    _currentState = State::PLACING_BLOCK;
    
    Result queueResult = _robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, placementAction);
    
    return queueResult;
  } // SelectNextPlacement()
  

  void BehaviorOCD::HandleActionCompleted(const ExternalInterface::RobotCompletedAction &msg)
  {
    switch(_currentState)
    {
      case State::PICKING_UP_BLOCK:
      {
        switch(msg.actionType) {
            
          case RobotActionType::PICKUP_OBJECT_HIGH:
          case RobotActionType::PICKUP_OBJECT_LOW:
            // We're done picking up the block, figure out where to put it
            SelectNextPlacement();
            _currentState = State::PLACING_BLOCK;
            break;
            
          case RobotActionType::PICK_AND_PLACE_INCOMPLETE:
            // We failed to pick up or place the last block, try again?
            SelectNextObjectToPickUp();
            break;
            
          default:
            // Simply ignore other action completions?
            break;

        } // switch(actionType)
        break;
      } // case PICKING_UP_BLOCK
        
      case State::PLACING_BLOCK:
      {
        switch(msg.actionType) {
          case RobotActionType::PLACE_OBJECT_LOW:
            _lastObjectPlacedOnGround = _objectToPickUp;
            // NOTE: deliberate fallthrough to next case!
          case RobotActionType::PLACE_OBJECT_HIGH:
            // We're done placing the block, mark it as neat and move to next one
            _messyObjects.erase(_objectToPickUp);
            _neatObjects.insert(_objectToPickUp);
            
            // TODO: Get "neat" color and on/off settings from config
            _robot.SetObjectLights(_objectToPickUp, WhichBlockLEDs::ALL, NamedColors::CYAN, NamedColors::BLACK, 10, 10, 2000, 2000, false, MakeRelativeMode::RELATIVE_LED_MODE_OFF, {});
            
            SelectNextObjectToPickUp();
            _currentState = State::PICKING_UP_BLOCK;
            break;
            
          case RobotActionType::PICK_AND_PLACE_INCOMPLETE:
            // We failed to place the last block, try again?
            SelectNextPlacement();
            break;
            
          default:
            // Simply ignore other action completions?
            break;
        }
        break;
      } // case PLACING_BLOCK
    } // switch(_currentState)
    
  } // HandleActionCompleted()
  
  
  void BehaviorOCD::HandleObjectMoved(const ExternalInterface::ActiveObjectMoved &msg)
  {
    // If a previously-neat object is moved, move it to the messy list
    // and play some kind of irritated animation
    ObjectID objectID;
    objectID = msg.objectID;
    
    if(_neatObjects.count(objectID)>0)
    {
      _neatObjects.erase(objectID);
      _messyObjects.insert(objectID);
      
      // TODO: Get "messy" color and on/off settings from config
      _robot.SetObjectLights(objectID, WhichBlockLEDs::ALL, NamedColors::RED, NamedColors::BLACK, 200, 200, 50, 50, false, MakeRelativeMode::RELATIVE_LED_MODE_OFF, {});
      
      // TODO: Should this be "now" or "next"?
      _robot.GetActionList().QueueActionNow(IBehavior::sActionSlot, new PlayAnimationAction("Irritated"));
    }
  }
  
  
  void BehaviorOCD::HandleObservedObject(const ExternalInterface::RobotObservedObject &msg)
  {
    // if the object is a BLOCK or ACTIVE_BLOCK, add its ID to the list we care about
    // iff we haven't already seen and neatened it (if it's in our neat list,
    // we won't take it out unless we detect that it was moved)
    ObjectID objectID;
    objectID = msg.objectID;

    if(_neatObjects.count(objectID) == 0) {
      _messyObjects.insert(objectID);
    }
  }
  
  void BehaviorOCD::HandleDeletedObject(const ExternalInterface::RobotDeletedObject &msg)
  {
    // remove the object if we knew about it
    ObjectID objectID;
    objectID = msg.objectID;

    _messyObjects.erase(objectID);
    _neatObjects.erase(objectID);
  }
  
  f32 BehaviorOCD::GetNeatnessScore(ObjectID whichObject)
  {
    Vision::ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(whichObject);
    if(object == nullptr) {
      PRINT_NAMED_ERROR("BehaviorOCD.GetNeatnessScore.InvalidObject",
                        "Could not get object %d from robot %d's world.\n",
                        whichObject.GetValue(), _robot.GetID());
      return 0.f;
    }
    
    // See how far away the object is from being aligned with the coordinate axes
    Radians angle = object->GetPose().GetRotationAngle<'Z'>();
    
    f32 diffFrom90deg;
    if(angle < 0) {
      diffFrom90deg = std::fmod(angle.getDegrees(), -90.f);
    } else {
      diffFrom90deg = std::fmod(angle.getDegrees(), 90.f);
    }
    
    const f32 score = 1.f - diffFrom90deg/90.f;
    return score;
  }
  
} // namespace Cozmo
} // namespace Anki