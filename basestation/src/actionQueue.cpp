/**
 * File: actionQueue.cpp
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements IAction interface for action states for a robot.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "actionQueue.h"

#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/basestation/robot.h"

#include "anki/cozmo/robot/cozmoConfig.h"

namespace Anki {
  
  namespace Cozmo {
    
    IAction::IAction(Robot& robot)
    : _robot(robot)
    , _preconditionsMet(false)
    , _waitUntilTime(-1.f)
    , _timeoutTime(-1.f)
    , _retryFcn(nullptr)
    {
    
    }
    
    IAction::ActionResult  IAction::Update()
    {
      ActionResult result = RUNNING;
      
      // On first call to Update(), figure out the waitUntilTime
      const f32 currentTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if(_waitUntilTime < 0.f) {
        _waitUntilTime = currentTimeInSeconds + GetStartDelayInSeconds();
      }
      if(_timeoutTime < 0.f) {
        _timeoutTime = currentTimeInSeconds + GetTimeoutInSeconds();
      }

      // Fail if we have exceeded timeout time
      if(currentTimeInSeconds > _timeoutTime) {
        PRINT_NAMED_INFO("IAction.Update.TimedOut",
                         "%s timed out after %.1f seconds.\n",
                         GetName().c_str(), GetStartDelayInSeconds());
        
        result = FAILURE_TIMEOUT;
      }
      // Don't do anything until we have reached the waitUntilTime
      else if(currentTimeInSeconds > _waitUntilTime)
      {
        
        if(!_preconditionsMet) {
          // Note that derived classes will define what to do when pre-conditions
          // are not met: if they return RUNNING, then the action will effectively
          // just wait for the preconditions to be met. Otherwise, a failure
          // will get propagated out as the return value of the Update method.
          result = CheckPreconditions();
          if(result == SUCCESS) {
            PRINT_NAMED_INFO("IAction.Update.PrecondtionsMet",
                             "Preconditions for %s successfully met.\n", GetName().c_str());
            _preconditionsMet = true;
          }
          
        } else {
          // Pre-conditions already met, just run until done
          result = CheckIfDone();
          if(result == FAILURE_RETRY) {
            if(_retryFcn != nullptr) {
              _retryFcn(_robot);
            } else {
              PRINT_NAMED_WARNING("IAction.Update.NoRetryFunction",
                                  "CheckIfDone() for %s returned FAILURE_RETRY, but no retry function is available.\n",
                                  GetName().c_str());
            }
          }
          
        }
      } // if(currentTimeInSeconds > _waitUntilTime)
      
      return result;
    } // Update()
    
    const std::string& IAction::GetName() const {
      static const std::string name("UnnamedAction");
      return name;
    }
  
    
#pragma mark ---- CheckForPathDoneHelper ----
    
    // Helper function used by Actions that utilize ExecutePathToPose internally
    static IAction::ActionResult CheckForPathDoneHelper(Robot& robot, const Pose3d& goalPose,
                                                        const f32 distanceThreshold,
                                                        const Radians& angleThreshold)
    {
      IAction::ActionResult result = IAction::RUNNING;
      
      // Wait until robot reports it is no longer traversing a path
      if(!robot.IsTraversingPath())
      {
        if(robot.GetPose().IsSameAs(goalPose, distanceThreshold, angleThreshold))
        {
          PRINT_NAMED_INFO("Action.CheckForPathDoneHelper.Success", "Robot %d successfully finished following path.\n", robot.GetID());
          robot.ClearPath(); // clear path and indicate that we are not replanning
          result = IAction::SUCCESS;
        }
        // The last path sent was definitely received by the robot
        // and it is no longer executing it, but we appear to not be in position
        else if (robot.GetLastSentPathID() == robot.GetLastRecvdPathID()) {
          PRINT_NAMED_INFO("Action.CheckForPathDoneHelper.DoneNotInPlace",
                           "Robot is done traversing path, but is not in position. lastPathID %d\n", robot.GetLastRecvdPathID());
          result = IAction::FAILURE_PROCEED;
        }
        else {
          // Something went wrong: not in place and robot apparently hasn't
          // received all that it should have
          PRINT_NAMED_INFO("Action.CheckForPathDoneHelper.Failure",
                           "Robot's state is FOLLOWING_PATH, but IsTraversingPath() returned false. currPathSegment = %d\n",
                           robot.GetCurrPathSegment());
          result = IAction::FAILURE_ABORT;
        }
      }
      
      return result;
      
    }
    
#pragma mark ---- DriveToPoseAction ----
    
    DriveToPoseAction::DriveToPoseAction(Robot& robot, const Pose3d& pose, const Radians& headAngle)
    : IAction(robot)
    , _goalPose(pose)
    , _goalHeadAngle(headAngle)
    , _goalDistanceThreshold(DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM)
    , _goalAngleThreshold(DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD)
    {
      
    }
    
    DriveToPoseAction::DriveToPoseAction(Robot& robot, const Pose3d& pose)
    : DriveToPoseAction(robot, pose, robot.GetHeadAngle())
    {
      
    }
    
    DriveToPoseAction::DriveToPoseAction(Robot& robot, const std::vector<Pose3d>& possiblePoses, const Radians& headAngle)
    : IAction(robot)
    , _possibleGoalPoses(possiblePoses)
    , _goalHeadAngle(headAngle)
    , _goalDistanceThreshold(DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM)
    , _goalAngleThreshold(DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD)
    {
      
    }
    
    DriveToPoseAction::DriveToPoseAction(Robot& robot, const std::vector<Pose3d>& possiblePoses)
    : DriveToPoseAction(robot, possiblePoses, robot.GetHeadAngle())
    {
      
    }
    
    const std::string& DriveToPoseAction::GetName() const
    {
      static const std::string name("DriveToPoseAction");
      return name;
    }
    
    IAction::ActionResult DriveToPoseAction::CheckPreconditions()
    {
      ActionResult result = SUCCESS;
      
      if(_possibleGoalPoses.empty()) {
        if(_robot.ExecutePathToPose(_goalPose, _goalHeadAngle) != RESULT_OK) {
          result = FAILURE_ABORT;
        }
      } else {
        size_t selectedIndex = 0;
        if(_robot.ExecutePathToPose(_possibleGoalPoses, _goalHeadAngle, selectedIndex) != RESULT_OK) {
          result = FAILURE_ABORT;
        } else {
          _goalPose = _possibleGoalPoses[selectedIndex];
        }
      }
      
      return result;
    }
    
    IAction::ActionResult DriveToPoseAction::CheckIfDone()
    {
      return CheckForPathDoneHelper(_robot, _goalPose, _goalDistanceThreshold, _goalDistanceThreshold);
    } // CheckIfDone()
    
    
    
#pragma mark ---- TurnInPlaceAction ----
    
    TurnInPlaceAction::TurnInPlaceAction(Robot& robot, const Radians& angle)
    : IAction(robot)
    , _turnAngle(angle)
    {
      
    }
    
    const std::string& TurnInPlaceAction::GetName() const
    {
      static const std::string name("TurnInPlaceAction");
      return name;
    }
    
    IAction::ActionResult TurnInPlaceAction::CheckPreconditions()
    {
      ActionResult result = SUCCESS;

      // Compute a goal pose rotated by specified angle around robot's
      // _current_ pose
      const Radians heading = _robot.GetPose().GetRotationAngle<'Z'>();
      
      _goalPose.SetRotation(heading + _turnAngle, Z_AXIS_3D);
      _goalPose.SetTranslation(_robot.GetPose().GetTranslation());
      
      if(_robot.ExecutePathToPose(_goalPose) != RESULT_OK) {
        result = FAILURE_ABORT;
      }

      return result;
    }
    
    IAction::ActionResult TurnInPlaceAction::CheckIfDone()
    {
      return CheckForPathDoneHelper(_robot, _goalPose,
                                    DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM,
                                    DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD);
    } // CheckIfDone()
    
    
#pragma mark ---- IDockAction ----
    
    // TODO: Define this as a constant parameter elsewhere
    #define MAX_DISTANCE_TO_PREDOCK_POSE 20.0f
    
    IDockAction::IDockAction(Robot& robot, ObjectID objectID)
    : IAction(robot)
    , _dockObjectID(objectID)
    {
      
    }
    
    IAction::ActionResult IDockAction::CheckPreconditions()
    {
      // Make sure the object we were docking with still exists in the world
      ActionableObject* dockObject = dynamic_cast<ActionableObject*>(_robot.GetBlockWorld().GetObjectByID(_dockObjectID));
      if(dockObject == nullptr) {
        PRINT_NAMED_ERROR("IDockAction.CheckPreconditions.ActionObjectNotFound",
                          "Action object with ID=%d no longer exists in the world.\n",
                          _dockObjectID.GetValue());
        
        return FAILURE_ABORT;
      }
      
      // Verify that we ended up near enough a PreActionPose of the right type
      std::vector<ActionableObject::PoseMarkerPair_t> preActionPoseMarkerPairs;
      dockObject->GetCurrentPreActionPoses(preActionPoseMarkerPairs, {GetPreActionType()});
      
      if(preActionPoseMarkerPairs.empty()) {
        PRINT_NAMED_ERROR("IDockAction.CheckPreconditions.NoPreActionPoses",
                          "Action object with ID=%d returned no pre-action poses of the given type.\n",
                          _dockObjectID.GetValue());
        
        return FAILURE_ABORT;
      }

      const Point2f currentXY(_robot.GetPose().GetTranslation().x(),
                              _robot.GetPose().GetTranslation().y());
      
      float closestDistSq = std::numeric_limits<float>::max();
      size_t closestIndex = preActionPoseMarkerPairs.size();
      
      //for(auto const& preActionPair : preActionPoseMarkerPairs) {
      for(size_t index=0; index < preActionPoseMarkerPairs.size(); ++index) {
        const ActionableObject::PoseMarkerPair_t& preActionPair = preActionPoseMarkerPairs[index];
        
        const Point2f preActionXY(preActionPair.first.GetTranslation().x(),
                                  preActionPair.first.GetTranslation().y());
        const float distSq = (currentXY - preActionXY).LengthSq();
        if(distSq < closestDistSq) {
          closestDistSq = distSq;
          closestIndex  = index;
        }
      }
      
      const f32 closestDist = sqrtf(closestDistSq);
      
      if(closestDist > MAX_DISTANCE_TO_PREDOCK_POSE) {
        PRINT_NAMED_INFO("IDockAction.CheckPreconditions.TooFarFromGoal",
                         "Robot is too far from pre-action pose (%.1fmm).", closestDist);
        return FAILURE_RETRY;
      }
      else {
        if(SelectDockAction(dockObject) != RESULT_OK) {
          PRINT_NAMED_ERROR("IDockAction.CheckPreconditions.DockActionSelectionFailure",
                            "");
          return FAILURE_ABORT;
        }
        
        PRINT_NAMED_INFO("IDockAction.CheckPreconditions.BeginDocking",
                         "Robot is within %.1fmm of the nearest pre-action pose, "
                         "docking with marker %d = %s (action = %d).\n",
                         closestDist, _dockMarker->GetCode(),
                         Vision::MarkerTypeStrings[_dockMarker->GetCode()], _dockAction);
      
        if(DockWithObjectHelper(preActionPoseMarkerPairs, closestIndex) == RESULT_OK) {
          return SUCCESS;
        } else {
          return FAILURE_ABORT;
        }
      }
      
    } // CheckPreconditions()
    
    
    IAction::ActionResult IDockAction::CheckIfDone()
    {
      ActionResult actionResult = RUNNING;
      
      if (!_robot.IsPickingOrPlacing() && !_robot.IsMoving())
      {
        // Stopped executing docking path, and should have backed out by now,
        // and have head pointed at an angle to see where we just placed or
        // picked up from. So we will check if we see a block with the same
        // ID/Type as the one we were supposed to be picking or placing, in the
        // right position.
        
        Result lastResult = RESULT_OK;
        
        switch(_dockAction)
        {
          case DA_PICKUP_LOW:
          case DA_PICKUP_HIGH:
          {
            lastResult = _robot.VerifyObjectPickup();
            if(lastResult != RESULT_OK) {
              PRINT_NAMED_ERROR("Robot.Update.VerifyObjectPickupFailed",
                                "VerifyObjectPickup returned error code %x.\n",
                                lastResult);
              actionResult = FAILURE_PROCEED;
            } else {
              actionResult = SUCCESS;
            }
            break;
          } // case PICKUP
            
          case DA_PLACE_LOW:
          case DA_PLACE_HIGH:
          {
            lastResult = _robot.VerifyObjectPlacement();
            if(lastResult != RESULT_OK) {
              PRINT_NAMED_ERROR("Robot.Update.VerifyObjectPlacementFailed",
                                "VerifyObjectPlacement returned error code %x.\n",
                                lastResult);
              actionResult = FAILURE_PROCEED;
            } else {
              actionResult = SUCCESS;
            }
            break;
          } // case PLACE
            
          case DA_RAMP_ASCEND:
          case DA_RAMP_DESCEND:
          {
            // TODO: Need to do some kind of verification here?
            PRINT_NAMED_INFO("Robot.Update.RampAscentOrDescentComplete",
                             "Robot has completed going up/down ramp.\n");
            actionResult = SUCCESS;
            break;
          } // case RAMP
            
          case DA_CROSS_BRIDGE:
          {
            // TODO: Need some kind of verificaiton here?
            PRINT_NAMED_INFO("Robot.Update.BridgeCrossingComplete",
                             "Robot has completed crossing a bridge.\n");
            actionResult = SUCCESS;
            break;
          }
            
          default:
            PRINT_NAMED_ERROR("Robot.Update", "Reached unknown dockAction case.\n");
            assert(false);
            actionResult = FAILURE_ABORT;
            
        } // switch(_dockAction)
      }
      
      return actionResult;
    } // CheckIfDone()
    
    
    Result IDockAction::DockWithObjectHelper(const std::vector<ActionableObject::PoseMarkerPair_t>& preActionPoseMarkerPairs,
                                             const size_t closestIndex)
    {
      _dockMarker  = &preActionPoseMarkerPairs[closestIndex].second;
      _dockMarker2 = nullptr;
      
      return _robot.DockWithObject(_dockObjectID, _dockMarker, _dockMarker2, _dockAction);
    }
    
    
#pragma mark ---- PickUpObjectAction ----
    
    PickUpObjectAction::PickUpObjectAction(Robot& robot, ObjectID objectID)
    : IDockAction(robot, objectID)
    {
      
    }
    
    Result PickUpObjectAction::SelectDockAction(ActionableObject* object)
    {
      // Choose docking action based on block's position and whether we are
      // carrying a block
      const f32 dockObjectHeight = object->GetPose().GetTranslation().z();
      _dockAction = DA_PICKUP_LOW;
      
      // TODO: Stop using constant ROBOT_BOUNDING_Z for this
      if (dockObjectHeight > 0.5f*ROBOT_BOUNDING_Z) { //  dockObject->GetSize().z()) {
        if(_robot.IsCarryingObject()) {
          PRINT_INFO("Already carrying object. Can't dock to high object. Aborting.\n");
          return RESULT_FAIL;
          
        } else {
          _dockAction = DA_PICKUP_HIGH;
        }
      } else if (_robot.IsCarryingObject()) {
        _dockAction = DA_PLACE_HIGH;
      }
      
      return RESULT_OK;
    } // SelectDockAction()
    
    
#pragma mark ---- CrossBridgeAction ----
    
    Result CrossBridgeAction::DockWithObjectHelper(const std::vector<ActionableObject::PoseMarkerPair_t>& preActionPoseMarkerPairs,
                                        const size_t closestIndex)
    {
      _dockMarker = &preActionPoseMarkerPairs[closestIndex].second;
      
      // Use the unchosen pre-crossing pose marker (the one at the other end of
      // the bridge) as dockMarker2
      assert(preActionPoseMarkerPairs.size() == 2);
      size_t indexForOtherEnd = 1 - closestIndex;
      assert(indexForOtherEnd == 0 || indexForOtherEnd == 1);
      _dockMarker2 = &(preActionPoseMarkerPairs[indexForOtherEnd].second);
 
      return _robot.DockWithObject(_dockObjectID, _dockMarker, _dockMarker2, _dockAction);
    }
    
    Result CrossBridgeAction::SelectDockAction(ActionableObject* object)
    {
      _dockAction = DA_CROSS_BRIDGE;
      return RESULT_OK;
    } // SelectDockAction()
    
    void Test(void)
    {
      Robot robot(7, nullptr);
      ObjectID id;
      id.Set();
      
      PickUpObjectAction action(robot, id);
    }
    
  } // namespace Cozmo
} // namespace Anki
