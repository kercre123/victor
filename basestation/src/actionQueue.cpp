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
#include "ramp.h"

#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/basestation/robot.h"

#include "anki/cozmo/robot/cozmoConfig.h"

namespace Anki {
  
  namespace Cozmo {
    
    IAction::IAction(Robot& robot)
    : _robot(robot)
    , _retryFcn(nullptr)
    {
      Reset();
    }
    
    void IAction::Reset()
    {
      _preconditionsMet = false;
      _waitUntilTime = -1.f;
      _timeoutTime = -1.f;
    }
    
    IAction::ActionResult  IAction::Update()
    {
      ActionResult result = RUNNING;
      SetStatus(GetName());
      
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
          //PRINT_NAMED_INFO("IAction.Update", "Updating %s: checking preconditions.\n", GetName().c_str());
          SetStatus(GetName() + ": check preconditions");
          
          // Note that derived classes will define what to do when pre-conditions
          // are not met: if they return RUNNING, then the action will effectively
          // just wait for the preconditions to be met. Otherwise, a failure
          // will get propagated out as the return value of the Update method.
          result = CheckPreconditions();
          if(result == SUCCESS) {
            PRINT_NAMED_INFO("IAction.Update.PrecondtionsMet",
                             "Preconditions for %s successfully met.\n", GetName().c_str());
            
            // If preconditions were successfully met, switch result to RUNNING
            // so that we don't think the entire action is completed. (We still
            // need to do CheckIfDone() calls!)
            // TODO: there's probably a tidier way to do this.
            _preconditionsMet = true;
            result = RUNNING;
            
            // Don't check if done until a sufficient amount of time has passed
            // after preconditions are met
            _waitUntilTime = currentTimeInSeconds + GetCheckIfDoneDelayInSeconds();
          }
          
        } else {
          //PRINT_NAMED_INFO("IAction.Update", "Updating %s: checking if done.\n", GetName().c_str());
          SetStatus(GetName() + ": check if done");
          
          // Pre-conditions already met, just run until done
          result = CheckIfDone();
        }
      } // if(currentTimeInSeconds > _waitUntilTime)
      
      return result;
    } // Update()
    
    const std::string& IAction::GetName() const {
      static const std::string name("UnnamedAction");
      return name;
    }
    
    void IAction::SetStatus(const std::string& msg)
    {
      _statusMsg = msg;
    }
    
    void IAction::SetRetryFunction(std::function<Result(Robot&)> retryFcn)
    {
      _retryFcn = retryFcn;
    }
  
    Result IAction::Retry()
    {
      if(_retryFcn != nullptr) {
        return _retryFcn(_robot);
      } else {
        // No retry function provided, just reset this action.
        Reset();
        return RESULT_OK;
      }
      
    } // Retry()
    
#pragma mark ---- ActionQueue ----
    
    ActionQueue::ActionQueue()
    {
      
    }
    
    ActionQueue::~ActionQueue()
    {
      Clear();
    }
    
    void ActionQueue::Clear()
    {
      while(!_queue.empty()) {
        IAction* action = _queue.front();
        CORETECH_ASSERT(action != nullptr);
        delete action;
        _queue.pop_front();
      }
    }
    
    Result ActionQueue::QueueAtEnd(IAction *action)
    {
      _queue.push_back(action);
      return RESULT_OK;
    }
    
    Result ActionQueue::QueueNext(Anki::Cozmo::IAction *action)
    {
      if(_queue.empty()) {
        return QueueAtEnd(action);
      }
      
      std::list<IAction*>::iterator queueIter = _queue.begin();
      ++queueIter;
      _queue.insert(queueIter, action);
      
      return RESULT_OK;
    }
    
    IAction* ActionQueue::GetCurrentAction()
    {
      if(_queue.empty()) {
        return nullptr;
      }
      
      return _queue.front();
    }
    
    void ActionQueue::PopCurrentAction()
    {
      if(!IsEmpty()) {
        if(_queue.front() == nullptr) {
          PRINT_NAMED_ERROR("ActionQueue.PopCurrentAction.NullActionPointer",
                            "About to delete and pop action pointer from queue, found it to be nullptr!\n");
        } else {
          delete _queue.front();
        }
        _queue.pop_front();
      }
    }
    
    void ActionQueue::Print() const
    {
      
      if(IsEmpty()) {
        PRINT_INFO("ActionQueue is empty.\n");
      } else {
        for(auto action : _queue) {
          PRINT_INFO("%s, ", action->GetName().c_str());
        }
        PRINT_INFO("\n");
      }
      
    } // Print()
    
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
        Vec3f Tdiff;
        if(robot.GetPose().IsSameAs(goalPose, distanceThreshold, angleThreshold, Tdiff))
        {
          PRINT_NAMED_INFO("Action.CheckForPathDoneHelper.Success",
                           "Robot %d successfully finished following path (Tdiff=%.1fmm).\n",
                           robot.GetID(), Tdiff.Length());
          robot.ClearPath(); // clear path and indicate that we are not replanning
          result = IAction::SUCCESS;
        }
        // The last path sent was definitely received by the robot
        // and it is no longer executing it, but we appear to not be in position
        else if (robot.GetLastSentPathID() == robot.GetLastRecvdPathID()) {
          PRINT_NAMED_INFO("Action.CheckForPathDoneHelper.DoneNotInPlace",
                           "Robot is done traversing path, but is not in position (dist=%.1fmm). lastPathID %d\n",
                           robot.GetLastRecvdPathID(), Tdiff.Length());
          result = IAction::FAILURE_RETRY;
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
    
    DriveToPoseAction::DriveToPoseAction(Robot& robot) //, const Pose3d& pose)
    : IAction(robot)
    , _isGoalSet(false)
    , _goalDistanceThreshold(DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM)
    , _goalAngleThreshold(DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD)
    {
      
    }
    
    DriveToPoseAction::DriveToPoseAction(Robot& robot, const Pose3d& pose)
    : DriveToPoseAction(robot)
    {
      SetGoal(pose);
    }
    
    Result DriveToPoseAction::SetGoal(const Anki::Pose3d& pose)
    {
      if(pose.GetWithRespectTo(*_robot.GetWorldOrigin(), _goalPose) == false) {
        PRINT_NAMED_ERROR("DriveToPoseAction.SetGoal.OriginMisMatch",
                          "Could not get specified pose w.r.t. robot %d's origin.\n", _robot.GetID());
        return RESULT_FAIL;
      }
      
      PRINT_NAMED_INFO("DriveToPoseAction.SetGoal",
                       "Setting pose goal to (%.1f,%.1f,%.1f) @ %.1fdeg\n",
                       _goalPose.GetTranslation().x(),
                       _goalPose.GetTranslation().y(),
                       _goalPose.GetTranslation().z(),
                       RAD_TO_DEG(_goalPose.GetRotationAngle<'Z'>().ToFloat()));
      
      _isGoalSet = true;
      
      return RESULT_OK;
    }
    
    const std::string& DriveToPoseAction::GetName() const
    {
      static const std::string name("DriveToPoseAction");
      return name;
    }
    
    IAction::ActionResult DriveToPoseAction::CheckPreconditions()
    {
      ActionResult result = SUCCESS;
      
      if(!_isGoalSet) {
        PRINT_NAMED_ERROR("DriveToPoseAction.CheckPreconditions.NoGoalSet",
                          "Goal must be set before running this action.\n");
        result = FAILURE_ABORT;
      }
      else if(_robot.ExecutePathToPose(_goalPose) != RESULT_OK) {
        result = FAILURE_ABORT;
      }
      
      return result;
    }
    
    IAction::ActionResult DriveToPoseAction::CheckIfDone()
    {
      return CheckForPathDoneHelper(_robot, _goalPose, _goalDistanceThreshold, _goalAngleThreshold);
    } // CheckIfDone()
    
#pragma mark ---- DriveToObjectAction ----
    
    DriveToObjectAction::DriveToObjectAction(Robot& robot, const ObjectID& objectID, const PreActionPose::ActionType& actionType)
    : DriveToPoseAction(robot)
    , _objectID(objectID)
    , _actionType(actionType)
    {
      // NOTE: _goalPose will be set later, when we check preconditions
    }
    

    
    const std::string& DriveToObjectAction::GetName() const
    {
      static const std::string name("DriveToObjectAction");
      return name;
    }
    
    
    IAction::ActionResult DriveToObjectAction::CheckPreconditionsHelper(ActionableObject* object)
    {
      ActionResult result = RUNNING;
      
      std::vector<PreActionPose> possiblePreActionPoses;
      object->GetCurrentPreActionPoses(possiblePreActionPoses, {_actionType}, std::set<Vision::Marker::Code>(), &_robot.GetPose());
      
      if(possiblePreActionPoses.empty()) {
        PRINT_NAMED_ERROR("DriveToObjectAction.CheckPreconditions.NoPreActionPoses",
                          "ActionableObject %d did not return any pre-action poses with action type %d.\n",
                          _objectID.GetValue(), _actionType);
        
        result = FAILURE_ABORT;
        
      } else {
        size_t selectedIndex = 0;
        
        // Make a vector of just poses (not preaction poses) for call to
        // Robot::ExecutePathToPose() below
        // TODO: Prettier way to handle this?
        std::vector<Pose3d> possiblePoses;
        for(auto & preActionPose : possiblePreActionPoses)
        {
          Pose3d possiblePose;
          if(preActionPose.GetPose().GetWithRespectTo(*_robot.GetWorldOrigin(), possiblePose) == false) {
            PRINT_NAMED_WARNING("DriveToObjectAction.CheckPreconditions.PreActionPoseOriginProblem",
                                "Could not get pre-action pose w.r.t. robot origin.\n");
            
          } else {

            possiblePoses.emplace_back(possiblePose);
            /*
            // If this pose is at a dockable height relative to the robot, queue it
            // as possible pose for the planner to consider. Just drop it to the
            // z=0 height and keep only the heading angle (rotaiton around Z)
            if(NEAR(possiblePose.GetTranslation().z(), _robot.GetPose().GetTranslation().z() + ROBOT_BOUNDING_Z*.5f, 25.f)) {
              possiblePose.SetRotation(possiblePose.GetRotationAngle<'Z'>(), Z_AXIS_3D);
              possiblePose.SetTranslation({{possiblePose.GetTranslation().x(), possiblePose.GetTranslation().y(), 0.f}});

            }
             */
          }
        }
        
        if(possiblePoses.empty()) {
          PRINT_NAMED_ERROR("DriveToObjectAction.CheckPreconditions.NoPossiblePoses",
                            "No pre-action poses survived as possible docking poses.\n");
          result = FAILURE_ABORT;
        }
        else if(_robot.ExecutePathToPose(possiblePoses, selectedIndex) != RESULT_OK) {
          result = FAILURE_ABORT;
        }
        else {
          _robot.MoveHeadToAngle(possiblePreActionPoses[selectedIndex].GetHeadAngle().ToFloat(), 5, 10);
          SetGoal(possiblePreActionPoses[selectedIndex].GetPose());
          result = SUCCESS;
        }
        
      } // if/else possiblePreActionPoses.empty()
      
      return result;
      
    } // CheckPreconditionsHelper()
    
    
    IAction::ActionResult DriveToObjectAction::CheckPreconditions()
    {
      ActionResult result = SUCCESS;
      
      ActionableObject* object = dynamic_cast<ActionableObject*>(_robot.GetBlockWorld().GetObjectByID(_objectID));
      if(object == nullptr) {
        PRINT_NAMED_ERROR("DriveToObjectAction.CheckPreconditions.NoObjectWithID",
                          "Robot %d's block world does not have an ActionableObject with ID=%d.\n",
                          _robot.GetID(), _objectID.GetValue());
        
        result = FAILURE_ABORT;
      } else {
      
        result = CheckPreconditionsHelper(object);
        
      } // if/else object==nullptr
      
      return result;
    }
    
    
#pragma mark ---- DriveToPlaceCarriedObjectAction ----
    
    DriveToPlaceCarriedObjectAction::DriveToPlaceCarriedObjectAction(Robot& robot, const Pose3d& placementPose)
    : DriveToObjectAction(robot, robot.GetCarryingObject(), PreActionPose::PLACEMENT)
    , _placementPose(placementPose)
    {

    }
    
    IAction::ActionResult DriveToPlaceCarriedObjectAction::CheckPreconditions()
    {
      ActionResult result = SUCCESS;
      
      if(_robot.IsCarryingObject() == false) {
        PRINT_NAMED_ERROR("DriveToPlaceCarriedObjectAction.CheckPreconditions.NotCarryingObject",
                          "Robot %d cannot place an object because it is not carrying anything.\n",
                          _robot.GetID());
        result = FAILURE_ABORT;
      } else {
        
        _objectID = _robot.GetCarryingObject();
        
        ActionableObject* object = dynamic_cast<ActionableObject*>(_robot.GetBlockWorld().GetObjectByID(_objectID));
        if(object == nullptr) {
          PRINT_NAMED_ERROR("DriveToPlaceCarriedObjectAction.CheckPreconditions.NoObjectWithID",
                            "Robot %d's block world does not have an ActionableObject with ID=%d.\n",
                            _robot.GetID(), _objectID.GetValue());
          
          result = FAILURE_ABORT;
        } else {
          
          // Temporarily move object to desired pose so we can get placement poses
          // at that position
          const Pose3d origObjectPose(object->GetPose());
          object->SetPose(_placementPose);
          
          result = CheckPreconditionsHelper(object);
          
          // Move the object back to where it was (being carried)
          object->SetPose(origObjectPose);
          
        } // if/else object==nullptr
      } // if/else robot is carrying object
      
      return result;
      
    } // CheckPreconditions()
    

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
    , _dockMarker(nullptr)
    , _dockMarker2(nullptr)
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
      std::vector<PreActionPose> preActionPoses;
      dockObject->GetCurrentPreActionPoses(preActionPoses, {GetPreActionType()});
      
      if(preActionPoses.empty()) {
        PRINT_NAMED_ERROR("IDockAction.CheckPreconditions.NoPreActionPoses",
                          "Action object with ID=%d returned no pre-action poses of the given type.\n",
                          _dockObjectID.GetValue());
        
        return FAILURE_ABORT;
      }

      const Point2f currentXY(_robot.GetPose().GetTranslation().x(),
                              _robot.GetPose().GetTranslation().y());
      
      float closestDistSq = std::numeric_limits<float>::max();
      size_t closestIndex = preActionPoses.size();
      
      //for(auto const& preActionPair : preActionPoseMarkerPairs) {
      for(size_t index=0; index < preActionPoses.size(); ++index) {
        const PreActionPose& preActionPose = preActionPoses[index];
        
        const Point2f preActionXY(preActionPose.GetPose().GetTranslation().x(),
                                  preActionPose.GetPose().GetTranslation().y());
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
                         "proceeding with docking.\n", closestDist);
      
        if(DockWithObjectHelper(preActionPoses, closestIndex) == RESULT_OK) {
          _robot.SetDockObject(_dockObjectID);
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
        
        actionResult = Verify();
      }
      
      return actionResult;
    } // CheckIfDone()
    
    
    Result IDockAction::DockWithObjectHelper(const std::vector<PreActionPose>& preActionPoses,
                                             const size_t closestIndex)
    {
      _dockMarker  = preActionPoses[closestIndex].GetMarker();
      _dockMarker2 = nullptr;
      
      PRINT_NAMED_INFO("IDockAction.DockWithObjectHelper.BeginDocking",
                       "Docking with marker %d (%s) using action %d.\n",
                       _dockMarker->GetCode(),
                       Vision::MarkerTypeStrings[_dockMarker->GetCode()], _dockAction);

      
      return _robot.DockWithObject(_dockObjectID, _dockMarker, _dockMarker2, _dockAction);
    }
    
    
#pragma mark ---- PickUpObjectAction ----
    
    PickUpObjectAction::PickUpObjectAction(Robot& robot, ObjectID objectID)
    : IDockAction(robot, objectID)
    {
      
    }
    
    const std::string& PickUpObjectAction::GetName() const
    {
      static const std::string name("PickUpObjectAction");
      return name;
    }
    
    Result PickUpObjectAction::SelectDockAction(ActionableObject* object)
    {
      // Record the object's original pose (before picking it up) so we can
      // verify later whether we succeeded.
      _dockObjectOrigPose = object->GetPose();
      
      // Choose docking action based on block's position and whether we are
      // carrying a block
      const f32 dockObjectHeight = _dockObjectOrigPose.GetTranslation().z();
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
    
    IAction::ActionResult PickUpObjectAction::Verify() const
    {
      if(_robot.IsCarryingObject() == false) {
        PRINT_NAMED_ERROR("PickUpObjectAction.Verify.RobotNotCarryignObject",
                          "Expecting robot to think its carrying an object at this point.\n");
        return FAILURE_ABORT;
      }
      
      BlockWorld& blockWorld = _robot.GetBlockWorld();
      
      // We should _not_ still see a object with the
      // same type as the one we were supposed to pick up in that
      // block's original position because we should now be carrying it.
      Vision::ObservableObject* carryObject = blockWorld.GetObjectByID(_robot.GetCarryingObject());
      if(carryObject == nullptr) {
        PRINT_NAMED_ERROR("PickUpObjectAction.Verify.CarryObjectNoLongerExists",
                          "Object %d we were carrying no longer exists in the world.\n",
                          _robot.GetCarryingObject().GetValue());
        return FAILURE_ABORT;
      }
      
      const BlockWorld::ObjectsMapByID_t& objectsWithType = blockWorld.GetExistingObjectsByType(carryObject->GetType());
      
      bool objectInOriginalPoseFound = false;
      for(auto object : objectsWithType) {
        // TODO: Make thresholds parameters
        // TODO: is it safe to always have useAbsRotation=true here?
        if(object.second->GetPose().IsSameAs_WithAmbiguity(_dockObjectOrigPose, carryObject->
                                                           GetRotationAmbiguities(),
                                                           15.f, DEG_TO_RAD(25), true))
        {
          objectInOriginalPoseFound = true;
          break;
        }
      }
      
      if(objectInOriginalPoseFound)
      {
        // Must not actually be carrying the object I thought I was!
        blockWorld.ClearObject(_robot.GetCarryingObject());
        _robot.UnSetCarryingObject();
        PRINT_INFO("Object pick-up FAILED! (Still seeing object in same place.)\n");
        return FAILURE_RETRY;
      } else {
        //_carryingObjectID = _dockObjectID;  // Already set?
        //_carryingMarker   = _dockMarker;   //   "
        PRINT_INFO("Object pick-up SUCCEEDED!\n");
        return SUCCESS;
      }
      
    } // Verify()
       
    
#pragma mark ---- PutDownObjectAction ----
    
    PutDownObjectAction::PutDownObjectAction(Robot & robot)
    : IAction(robot)
    {
      
    }
    
    const std::string& PutDownObjectAction::GetName() const
    {
      static const std::string name("PutDownObjectAction");
      return name;
    }
   
    IAction::ActionResult PutDownObjectAction::CheckPreconditions()
    {
      ActionResult result = RUNNING;
      
      // Wait for robot to stop moving before proceeding with pre-conditions
      if (_robot.IsMoving() == false)
      {
        // Robot must be carrying something to put something down!
        if(_robot.IsCarryingObject() == false) {
          PRINT_NAMED_ERROR("PutDownObjectAction.CheckPreconditions.NotCarryingObject",
                            "Robot %d executing PutDownObjectAction but not carrying object.\n", _robot.GetID());
          result = FAILURE_ABORT;
        } else {
          
          _carryingObjectID  = _robot.GetCarryingObject();
          _carryObjectMarker = _robot.GetCarryingMarker();
        
          if(_robot.SendPlaceObjectOnGround(0, 0, 0) == RESULT_OK)
          {
            result = SUCCESS;
          } else {
            PRINT_NAMED_ERROR("PutDownObjectAction.CheckPreconditions.SendPlaceObjectOnGroundFailed",
                              "Robot's SendPlaceObjectOnGround method reported failure.\n");
            result = FAILURE_ABORT;
          }
          
        } // if/else IsCarryingObject()
      } // if robot IsMoving()
      
      return result;
      
    } // CheckPreconditions()
    
    
    IAction::ActionResult PutDownObjectAction::CheckIfDone()
    {
      ActionResult actionResult = RUNNING;
      
      // Wait for robot to report it is done picking/placing and that it's not
      // moving
      if (!_robot.IsPickingOrPlacing() && !_robot.IsMoving())
      {
        // Stopped executing docking path, and should have placed carried block
        // and backed out by now, and have head pointed at an angle to see
        // where we just placed or picked up from.
        // So we will check if we see a block with the same
        // ID/Type as the one we were supposed to be picking or placing, in the
        // right position.

        // TODO: check to see it ended up in the right place?
        Vision::ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(_carryingObjectID);
        if(object == nullptr) {
          PRINT_NAMED_ERROR("PutDownObjectAction.CheckIfDone.CarryObjectNoLongerExists",
                            "Object %d we were carrying no longer exists in the world.\n",
                            _robot.GetCarryingObject().GetValue());
          return FAILURE_ABORT;
        }
        else if(object->GetLastObservedTime() > (_robot.GetLastMsgTimestamp()-500))
        {
          // We've seen the object in the last half second (which could
          // not be true if we were still carrying it)
          PRINT_NAMED_INFO("PutDownObjectAction.CheckIfDone.ObjectPlacementSuccess",
                           "Verification of object placement SUCCEEDED!\n");
          return SUCCESS;
        } else {
          PRINT_NAMED_INFO("PutDownObjectAction.CheckIfDone.ObjectPlacementSuccess",
                           "Verification of object placement FAILED!\n");
          // TODO: correct to assume we are still carrying the object?
          _robot.PickUpObject(_carryingObjectID, _carryObjectMarker); // re-pickup object to attach it to the lift again
          return FAILURE_RETRY;
        }
        
      } // if robot is not picking/placing or moving
      
      return actionResult;
      
    } // CheckIfDone()
    

    
#pragma mark ---- CrossBridgeAction ----
    
    CrossBridgeAction::CrossBridgeAction(Robot& robot, ObjectID bridgeID)
    : IDockAction(robot, bridgeID)
    {
      
    }
    
    const std::string& CrossBridgeAction::GetName() const
    {
      static const std::string name("CrossBridgeAction");
      return name;
    }
    
    Result CrossBridgeAction::DockWithObjectHelper(const std::vector<PreActionPose>& preActionPoses,
                                                   const size_t closestIndex)
    {
      _dockMarker = preActionPoses[closestIndex].GetMarker();
      
      // Use the unchosen pre-crossing pose marker (the one at the other end of
      // the bridge) as dockMarker2
      assert(preActionPoses.size() == 2);
      size_t indexForOtherEnd = 1 - closestIndex;
      assert(indexForOtherEnd == 0 || indexForOtherEnd == 1);
      _dockMarker2 = preActionPoses[indexForOtherEnd].GetMarker();
 
      assert(_dockMarker != nullptr);
      assert(_dockMarker2 != nullptr);
      
      PRINT_NAMED_INFO("CrossBridgeAction.DockWithObjectHelper.BeginDocking",
                       "Crossing bridge using markers %d (%s) and %d (%s) using action %d.\n",
                       _dockMarker->GetCode(),
                       Vision::MarkerTypeStrings[_dockMarker->GetCode()],
                       _dockMarker2->GetCode(),
                       Vision::MarkerTypeStrings[_dockMarker2->GetCode()],
                       _dockAction);
      
      return _robot.DockWithObject(_dockObjectID, _dockMarker, _dockMarker2, _dockAction);
    }
    
    Result CrossBridgeAction::SelectDockAction(ActionableObject* object)
    {
      _dockAction = DA_CROSS_BRIDGE;
      return RESULT_OK;
    } // SelectDockAction()
    
    IAction::ActionResult CrossBridgeAction::Verify() const
    {
      // TODO: Need some kind of verificaiton here?
      PRINT_NAMED_INFO("CrossBridgeAction.Verify.BridgeCrossingComplete",
                       "Robot has completed crossing a bridge.\n");
      return SUCCESS;
    } // Verify()
    
    
#pragma mark ---- AscendOrDescendRampAction ----
    
    AscendOrDescendRampAction::AscendOrDescendRampAction(Robot& robot, ObjectID rampID)
    : IDockAction(robot, rampID)
    {

    }
    
    const std::string& AscendOrDescendRampAction::GetName() const
    {
      static const std::string name("AscendOrDescendRampAction");
      return name;
    }
    
    Result AscendOrDescendRampAction::SelectDockAction(ActionableObject* object)
    {
      Ramp* ramp = dynamic_cast<Ramp*>(object);
      if(ramp == nullptr) {
        PRINT_NAMED_ERROR("AscendOrDescendRampAction.SelectDockAction.NotRampObject",
                          "Could not cast generic ActionableObject into Ramp object.\n");
        return RESULT_FAIL;
      }
      
      Result result = RESULT_OK;
      
      // Choose ascent or descent
      const Ramp::TraversalDirection direction = ramp->IsAscendingOrDescending(_robot.GetPose());
      switch(direction)
      {
        case Ramp::ASCENDING:
          _dockAction = DA_RAMP_ASCEND;
          break;
          
        case Ramp::DESCENDING:
          _dockAction = DA_RAMP_DESCEND;
          break;
          
        case Ramp::UNKNOWN:
        default:
          result = RESULT_FAIL;
      }
    
      _robot.SetRamp(GetDockObjectID(), direction);
      
      return result;
      
    } // SelectDockAction()
    
    IAction::ActionResult AscendOrDescendRampAction::Verify() const
    {
      // TODO: Need to do some kind of verification here?
      PRINT_NAMED_INFO("AscendOrDescendRampAction.Verify.RampAscentOrDescentComplete",
                       "Robot has completed going up/down ramp.\n");
      return SUCCESS;
    } // Verify()
    
    /*
    static void TestInstantiation(void)
    {
      Robot robot(7, nullptr);
      ObjectID id;
      id.Set();
      
      PickUpObjectAction action(robot, id);
      AscendOrDescendRampAction action2(robot, id);
      CrossBridgeAction action3(robot, id);
    }
    */
  } // namespace Cozmo
} // namespace Anki
