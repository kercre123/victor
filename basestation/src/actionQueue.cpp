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


#include "pathPlanner.h"


namespace Anki {
  
  namespace Cozmo {
    
    IActionRunner::IActionRunner()
    : _numRetriesRemaining(0)
    {
      
    }
    
    bool IActionRunner::RetriesRemain()
    {
      if(_numRetriesRemaining > 0) {
        --_numRetriesRemaining;
        return true;
      } else {
        return false;
      }
    }
    
    const std::string& IActionRunner::GetName() const {
      static const std::string name("UnnamedAction");
      return name;
    }
    
#   if USE_ACTION_CALLBACKS
    void IActionRunner::AddCompletionCallback(ActionCompletionCallback callback)
    {
      _completionCallbacks.emplace_back(callback);
    }
    
    void IActionRunner::RunCallbacks(ActionResult result) const
    {
      for(auto callback : _completionCallbacks) {
        callback(result);
      }
    }
#   endif // USE_ACTION_CALLBACKS
    
    
#pragma mark ---- IAction ----
    
    IAction::IAction()
    {
      Reset();
    }
    
    void IAction::Reset()
    {
      _preconditionsMet = false;
      _waitUntilTime = -1.f;
      _timeoutTime = -1.f;
    }
    
    IAction::ActionResult IAction::Update(Robot& robot)
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
          result = Init(robot);
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
          result = CheckIfDone(robot);
        }
      } // if(currentTimeInSeconds > _waitUntilTime)
      
      if(result == FAILURE_RETRY && RetriesRemain()) {
        PRINT_NAMED_INFO("IAction.Update.CurrentActionFailedRetrying",
                         "Robot %d failed running action %s. Retrying.\n",
                         robot.GetID(), GetName().c_str());
        
        Reset();
        result = RUNNING;
      }
      
#     if USE_ACTION_CALLBACKS
      if(result != RUNNING) {
        RunCallbacks(result);
      }
#     endif
      
      return result;
    } // Update()
    
    /*
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
    */
#pragma mark ---- ActionList ----
    
    ActionList::ActionList()
    : _slotCounter(0)
    {
      
    }
    
    ActionList::~ActionList()
    {
      Clear();
    }
    
    void ActionList::Clear()
    {
      _queues.clear();
    }
    
    bool ActionList::IsEmpty() const
    {
      return _queues.empty();
    }
    
    void ActionList::Print() const
    {
      if(IsEmpty()) {
        PRINT_INFO("ActionList is empty.\n");
      } else {
        PRINT_INFO("ActionList contains %d queues:\n", _queues.size());
        for(auto const& queuePair : _queues) {
          PRINT_INFO("---");
          queuePair.second.Print();
        }
      }

    } // Print()
    
    Result ActionList::Update(Robot& robot)
    {
      Result lastResult = RESULT_OK;
      
      for(auto queueIter = _queues.begin(); queueIter != _queues.end(); )
      {
        lastResult = queueIter->second.Update(robot);

        // If the queue is complete, remove it
        if(queueIter->second.IsEmpty()) {
          queueIter = _queues.erase(queueIter);
        } else {
          ++queueIter;
        }
      } // for each actionMemberPair
      
      return lastResult;
    } // Update()
    
    
    ActionList::SlotHandle ActionList::AddAction(IActionRunner* action, u8 numRetries)
    {
      if(action == nullptr) {
        PRINT_NAMED_WARNING("ActionList.AddAction.NullActionPointer", "Refusing to add null action.\n");
        return _slotCounter;
      }
      
      SlotHandle currentSlot = _slotCounter;
      
      if(_queues[currentSlot].QueueAtEnd(action, numRetries) == RESULT_OK) {
        _slotCounter++;
      } else {
        PRINT_NAMED_ERROR("ActionList.AddAction.FailedToAdd", "Failed to add action to new queue.\n");
      }
      
      return currentSlot;
    }
    
    
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
        IActionRunner* action = _queue.front();
        CORETECH_ASSERT(action != nullptr);
        delete action;
        _queue.pop_front();
      }
    }
    
    Result ActionQueue::QueueAtEnd(IActionRunner *action, u8 numRetries)
    {
      if(action == nullptr) {
        PRINT_NAMED_ERROR("ActionQueue.QueueAtEnd.NullActionPointer",
                          "Refusing to queue a null action pointer.\n");
        return RESULT_FAIL;
      }

      action->SetNumRetries(numRetries);
      _queue.push_back(action);
      return RESULT_OK;
    }
    
    Result ActionQueue::QueueNext(IActionRunner *action, u8 numRetries)
    {
      if(action == nullptr) {
        PRINT_NAMED_ERROR("ActionQueue.QueueNext.NullActionPointer",
                          "Refusing to queue a null action pointer.\n");
        return RESULT_FAIL;
      }
      
      action->SetNumRetries(numRetries);
      
      if(_queue.empty()) {
        return QueueAtEnd(action);
      }
      
      std::list<IActionRunner*>::iterator queueIter = _queue.begin();
      ++queueIter;
      _queue.insert(queueIter, action);
      
      return RESULT_OK;
    }
    
    Result ActionQueue::Update(Robot& robot)
    {
      Result lastResult = RESULT_OK;
      
      if(!_queue.empty())
      {
        IActionRunner* currentAction = GetCurrentAction();
        assert(currentAction != nullptr);
        
        const IAction::ActionResult actionResult = currentAction->Update(robot);
        
        if(actionResult != IActionRunner::RUNNING) {
          // Current action just finished, pop it
          PopCurrentAction();
        }
      } // if queue not empty
      
      return lastResult;
    }
    
    IActionRunner* ActionQueue::GetCurrentAction()
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
        PRINT_INFO("ActionQueue with %d actions: ", _queue.size());
        for(auto action : _queue) {
          PRINT_INFO("%s, ", action->GetName().c_str());
        }
        PRINT_INFO("\b\b\n");
      }
      
    } // Print()
    
    
#pragma mark ---- ICompoundAction ----
    
    ICompoundAction::ICompoundAction(std::initializer_list<IActionRunner*> actions)
    : _name("[")
    {
      for(IActionRunner* action : actions) {
        if(action == nullptr) {
          PRINT_NAMED_WARNING("ICompoundAction.NullActionPointer",
                              "Refusing to add a null action pointer to group.\n");
        } else {
          _actions.emplace_back(false, action);
          _name += action->GetName();
          _name += "+";
        }
      }
      _name += "]";
    }
    
    ICompoundAction::~ICompoundAction()
    {
      for(auto & actionPair : _actions) {
        assert(actionPair.second != nullptr);
        // TODO: issue a warning when a group is deleted without all its actions completed?
        delete actionPair.second;
      }
    }
    
    void ICompoundAction::Reset()
    {
      for(auto & actionPair : _actions) {
        actionPair.first = false;
        actionPair.second->Reset();
      }
    }
    
#pragma mark ---- CompoundActionSequential ----
    
    CompoundActionSequential::CompoundActionSequential(std::initializer_list<IActionRunner*> actions)
    : ICompoundAction(actions)
    , _delayBetweenActionsInSeconds(0)
    , _waitUntilTime(-1.f)
    , _currentActionPair(_actions.begin())
    {
      
    }
    
    void CompoundActionSequential::Reset()
    {
      ICompoundAction::Reset();
      _waitUntilTime = -1.f;
      _currentActionPair = _actions.begin();
    }
    
    IAction::ActionResult CompoundActionSequential::Update(Robot& robot)
    {
      SetStatus(GetName());
      
      if(_currentActionPair != _actions.end())
      {
        bool& isDone = _currentActionPair->first;
        assert(isDone == false);
        
        IActionRunner* currentAction = _currentActionPair->second;
        assert(currentAction != nullptr); // should not have been allowed in by constructor
        
        if(_waitUntilTime < 0.f ||
           BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _waitUntilTime)
        {
          const ActionResult subResult = currentAction->Update(robot);
          SetStatus(currentAction->GetStatus());
          switch(subResult)
          {
            case SUCCESS:
            {
              // Finished the current action, move ahead to the next
              isDone = true; // mark as done (not strictly necessary)
              
              if(_delayBetweenActionsInSeconds > 0.f) {
                // If there's a delay specified, figure out how long we need to
                // wait from now to start next action
                _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _delayBetweenActionsInSeconds;
              }
              
              ++_currentActionPair;
              
              // if that was the last action, we're done
              if(_currentActionPair == _actions.end()) {
#             if USE_ACTION_CALLBACKS
                RunCallbacks(SUCCESS);
#             endif
                return SUCCESS;
              }
              
              // Otherwise, we are still running
              return RUNNING;
            }
              
            case FAILURE_RETRY:
              // A constituent action failed . Reset all the constituent actions
              // and try again as long as there are retries remaining
              if(RetriesRemain()) {
                PRINT_NAMED_INFO("CompoundActionSequential.Update.Retrying",
                                 "%s triggered retry.\n", currentAction->GetName().c_str());
                Reset();
                return RUNNING;
              }
              // No retries remaining. Fall through:
              
            case RUNNING:
            case FAILURE_ABORT:
            case FAILURE_TIMEOUT:
            case FAILURE_PROCEED:
#           if USE_ACTION_CALLBACKS
              RunCallbacks(subResult);
#           endif
              return subResult;
              
          } // switch(result)
        } else {
          return RUNNING;
        } // if/else waitUntilTime
      } // if currentAction != actions.end()
      
      // Shouldn't normally get here, but this means we've completed everything
      // and are done
      return SUCCESS;
      
    } // CompoundActionSequential::Update()
    
    
#pragma mark ---- CompoundActionParallel ----
    
    CompoundActionParallel::CompoundActionParallel(std::initializer_list<IActionRunner*> actions)
    : ICompoundAction(actions)
    {
      
    }
    
    IAction::ActionResult CompoundActionParallel::Update(Robot& robot)
    {
      // Return success unless we encounter anything still running or failed in loop below.
      // Note that we will return SUCCESS on the call following the one where the
      // last action actually finishes.
      ActionResult result = SUCCESS;
      
      SetStatus(GetName());
      
      for(auto & currentActionPair : _actions)
      {
        bool& isDone = currentActionPair.first;
        if(!isDone) {
          IActionRunner* currentAction = currentActionPair.second;
          assert(currentAction != nullptr); // should not have been allowed in by constructor
          
          const ActionResult subResult = currentAction->Update(robot);
          SetStatus(currentAction->GetStatus());
          switch(subResult)
          {
            case SUCCESS:
              // Just finished this action, mark it as done
              isDone = true;
              break;
              
            case RUNNING:
              // If any action is still running the group is still running
              result = RUNNING;
              break;
              
            case FAILURE_RETRY:
              // If any retries are left, reset the group and try again.
              if(RetriesRemain()) {
                PRINT_NAMED_INFO("CompoundActionParallel.Update.Retrying",
                                 "%s triggered retry.\n", currentAction->GetName().c_str());
                Reset();
                return RUNNING;
              }
              
              // If not, just fall through to other failure handlers:
            case FAILURE_ABORT:
            case FAILURE_PROCEED:
            case FAILURE_TIMEOUT:
              // Return failure, aborting updating remaining actions the group
#             if USE_ACTION_CALLBACKS
              RunCallbacks(subResult);
#             endif
              return subResult;
              
            default:
              PRINT_NAMED_ERROR("CompoundActionParallel.Update.UnknownResultCase", "\n");
              assert(false);
              return FAILURE_ABORT;
              
          } // switch(subResult)
          
        } // if(!isDone)
      } // for each action in the group
      
#     if USE_ACTION_CALLBACKS
      if(result != RUNNING) {
        RunCallbacks(result);
      }
#     endif
      
      return result;
    } // CompoundActionParallel::Update()
    

#pragma mark ---- DriveToPoseAction ----
    
    DriveToPoseAction::DriveToPoseAction() //, const Pose3d& pose)
    : _isGoalSet(false)
    , _goalDistanceThreshold(DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM)
    , _goalAngleThreshold(DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD)
    {
      
    }
    
    DriveToPoseAction::DriveToPoseAction(const Pose3d& pose)
    {
      SetGoal(pose);
    }
    
    Result DriveToPoseAction::SetGoal(const Anki::Pose3d& pose)
    {
      _goalPose = pose;
      
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
    
    IAction::ActionResult DriveToPoseAction::Init(Robot& robot)
    {
      // Wait for robot to be idle before trying to drive to a new pose, to
      // allow previously-added docking/moving actions to finish
      if(robot.IsMoving()) {
        return RUNNING;
      }
      
      ActionResult result = SUCCESS;
      
      if(!_isGoalSet) {
        PRINT_NAMED_ERROR("DriveToPoseAction.CheckPreconditions.NoGoalSet",
                          "Goal must be set before running this action.\n");
        result = FAILURE_ABORT;
      }
      else {
        Planning::Path p;
        
        // TODO: Make it possible to set the speed/accel somewhere?
        if(robot.MoveHeadToAngle(HEAD_ANGLE_WHILE_FOLLOWING_PATH, 1.f, 3.f) != RESULT_OK) {
          result = FAILURE_ABORT;
        }
        else if(robot.GetPathToPose(_goalPose, p) != RESULT_OK) {
          result = FAILURE_ABORT;
        }
        else if(robot.ExecutePath(p) != RESULT_OK) {
          result = FAILURE_ABORT;
        }
      }
      
      return result;
    }
    
    IAction::ActionResult DriveToPoseAction::CheckIfDone(Robot& robot)
    {
      IAction::ActionResult result = IAction::RUNNING;
      
      // Wait until robot reports it is no longer traversing a path
      if(robot.IsTraversingPath())
      {
        // If the robot is traversing a path, consider replanning it
        if(robot.GetBlockWorld().DidBlocksChange())
        {
          Planning::Path newPath;
          switch(robot.GetPathPlanner()->GetPlan(newPath, robot.GetPose(), _forceReplanOnNextWorldChange))
          {
            case IPathPlanner::DID_PLAN:
            {
              // clear path, but flag that we are replanning
              robot.ClearPath();
              _forceReplanOnNextWorldChange = false;
              
              PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.UpdatePath", "sending new path to robot\n");
              robot.ExecutePath(newPath);
              break;
            } // case DID_PLAN:
              
            case IPathPlanner::PLAN_NEEDED_BUT_GOAL_FAILURE:
            {
              PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.NewGoalForReplanNeeded",
                               "Replan failed due to bad goal. Aborting path.\n");
              
              robot.ClearPath();
              break;
            } // PLAN_NEEDED_BUT_GOAL_FAILURE:
              
            case IPathPlanner::PLAN_NEEDED_BUT_START_FAILURE:
            {
              PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.NewStartForReplanNeeded",
                               "Replan failed during docking due to bad start. Will try again, and hope robot moves.\n");
              break;
            }
              
            case IPathPlanner::PLAN_NEEDED_BUT_PLAN_FAILURE:
            {
              PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.NewEnvironmentForReplanNeeded",
                               "Replan failed during docking due to a planner failure. Will try again, and hope environment changes.\n");
              // clear the path, but don't change the state
              robot.ClearPath();
              _forceReplanOnNextWorldChange = true;
              break;
            }
              
            default:
            {
              // Don't do anything just proceed with the current plan...
              break;
            }
              
          } // switch(GetPlan())
        } // if blocks changed
        
      } else {
        Vec3f Tdiff;
        if(robot.GetPose().IsSameAs(_goalPose, _goalDistanceThreshold, _goalAngleThreshold, Tdiff))
        {
          PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.Success",
                           "Robot %d successfully finished following path (Tdiff=%.1fmm).\n",
                           robot.GetID(), Tdiff.Length());
          
          result = IAction::SUCCESS;
        }
        // The last path sent was definitely received by the robot
        // and it is no longer executing it, but we appear to not be in position
        else if (robot.GetLastSentPathID() == robot.GetLastRecvdPathID()) {
          PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.DoneNotInPlace",
                           "Robot is done traversing path, but is not in position (dist=%.1fmm). lastPathID=%d\n",
                           Tdiff.Length(), robot.GetLastRecvdPathID());
          result = IAction::FAILURE_RETRY;
        }
        else {
          // Something went wrong: not in place and robot apparently hasn't
          // received all that it should have
          PRINT_NAMED_INFO("DriveToPoseAction.CheckIfDone.Failure",
                           "Robot's state is FOLLOWING_PATH, but IsTraversingPath() returned false.\n");
          result = IAction::FAILURE_ABORT;
        }
      }
      
      // If we are not running anymore, for any reason, clear the path and its
      // visualization
      if(result != IAction::RUNNING) {
        robot.ClearPath(); // clear path and indicate that we are not replanning
        VizManager::getInstance()->ErasePath(robot.GetID());
        VizManager::getInstance()->EraseAllPlannerObstacles(true);
        VizManager::getInstance()->EraseAllPlannerObstacles(false);
      }
      
      return result;
    } // CheckIfDone()
    
#pragma mark ---- DriveToObjectAction ----
    
    DriveToObjectAction::DriveToObjectAction(const ObjectID& objectID, const PreActionPose::ActionType& actionType)
    : _objectID(objectID)
    , _actionType(actionType)
    {
      // NOTE: _goalPose will be set later, when we check preconditions
    }
    
    
    const std::string& DriveToObjectAction::GetName() const
    {
      static const std::string name("DriveToObjectAction");
      return name;
    }
    
    
    IAction::ActionResult DriveToObjectAction::InitHelper(Robot& robot, ActionableObject* object)
    {
      ActionResult result = RUNNING;
      
      std::vector<PreActionPose> possiblePreActionPoses;
      object->GetCurrentPreActionPoses(possiblePreActionPoses, {_actionType}, std::set<Vision::Marker::Code>(), &robot.GetPose());
      
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
          if(preActionPose.GetPose().GetWithRespectTo(*robot.GetWorldOrigin(), possiblePose) == false) {
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
        else {
          Planning::Path p;
          if(robot.GetPathToPose(possiblePoses, selectedIndex, p) != RESULT_OK) {
            result = FAILURE_ABORT;
          }
          else if(robot.ExecutePath(p) != RESULT_OK) {
            result = FAILURE_ABORT;
          }
          else if(robot.MoveHeadToAngle(HEAD_ANGLE_WHILE_FOLLOWING_PATH, 1.f, 3.f) != RESULT_OK) {
            result = FAILURE_ABORT;
          } else {
            SetGoal(possiblePoses[selectedIndex]);
            
            // Record where we want the head to end up, but don't actually move it
            // there yet. We'll let the path follower use whatever head angle it
            // wants to that's good for driving and then make use of this head
            // angle at the end, once the path is complete.
            _finalHeadAngle = possiblePreActionPoses[selectedIndex].GetHeadAngle();
            
            result = SUCCESS;
          }
        }
        
      } // if/else possiblePreActionPoses.empty()
      
      return result;
      
    } // InitHelper()
    
    
    IAction::ActionResult DriveToObjectAction::Init(Robot& robot)
    {
      ActionResult result = SUCCESS;
      
      ActionableObject* object = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetObjectByID(_objectID));
      if(object == nullptr) {
        PRINT_NAMED_ERROR("DriveToObjectAction.CheckPreconditions.NoObjectWithID",
                          "Robot %d's block world does not have an ActionableObject with ID=%d.\n",
                          robot.GetID(), _objectID.GetValue());
        
        result = FAILURE_ABORT;
      } else {
      
        result = InitHelper(robot, object);
        
      } // if/else object==nullptr
      
      return result;
    }
    
    IAction::ActionResult DriveToObjectAction::CheckIfDone(Robot& robot)
    {
      ActionResult result = DriveToPoseAction::CheckIfDone(robot);
      
      if(result == SUCCESS) {
        // Just before returning success when normal DriveToPose action finishes,
        // Set the head angle to the one selected by the pre-action pose.
        // (We don't do this earlier because it may not be a good head angle
        // for path following -- e.g., for the prox sensors to be usable.)
        if(robot.MoveHeadToAngle(_finalHeadAngle.ToFloat(), 1.f, 3.f) != RESULT_OK) {
          result = FAILURE_ABORT;
        }
      }
      
      return result;
    }
    
            
#pragma mark ---- DriveToPlaceCarriedObjectAction ----
    
    DriveToPlaceCarriedObjectAction::DriveToPlaceCarriedObjectAction(const Robot& robot, const Pose3d& placementPose)
    : DriveToObjectAction(robot.GetCarryingObject(), PreActionPose::PLACEMENT)
    , _placementPose(placementPose)
    {

    }
    
    const std::string& DriveToPlaceCarriedObjectAction::GetName() const
    {
      static const std::string name("DriveToPlaceCarriedObjectAction");
      return name;
    }
    
    IAction::ActionResult DriveToPlaceCarriedObjectAction::Init(Robot& robot)
    {
      ActionResult result = SUCCESS;
      
      if(robot.IsCarryingObject() == false) {
        PRINT_NAMED_ERROR("DriveToPlaceCarriedObjectAction.CheckPreconditions.NotCarryingObject",
                          "Robot %d cannot place an object because it is not carrying anything.\n",
                          robot.GetID());
        result = FAILURE_ABORT;
      } else {
        
        _objectID = robot.GetCarryingObject();
        
        ActionableObject* object = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetObjectByID(_objectID));
        if(object == nullptr) {
          PRINT_NAMED_ERROR("DriveToPlaceCarriedObjectAction.CheckPreconditions.NoObjectWithID",
                            "Robot %d's block world does not have an ActionableObject with ID=%d.\n",
                            robot.GetID(), _objectID.GetValue());
          
          result = FAILURE_ABORT;
        } else {
          
          // Temporarily move object to desired pose so we can get placement poses
          // at that position
          const Pose3d origObjectPose(object->GetPose());
          object->SetPose(_placementPose);
          
          // Call parent class's init helper
          result = InitHelper(robot, object);
          
          // Move the object back to where it was (being carried)
          object->SetPose(origObjectPose);
          
        } // if/else object==nullptr
      } // if/else robot is carrying object
      
      return result;
      
    } // CheckPreconditions()
    

#pragma mark ---- TurnInPlaceAction ----
    
    TurnInPlaceAction::TurnInPlaceAction(const Radians& angle)
    : _turnAngle(angle)
    {
      
    }
    
    const std::string& TurnInPlaceAction::GetName() const
    {
      static const std::string name("TurnInPlaceAction");
      return name;
    }
    
    IAction::ActionResult TurnInPlaceAction::Init(Robot &robot)
    {
      ActionResult result = SUCCESS;

      // Compute a goal pose rotated by specified angle around robot's
      // _current_ pose
      const Radians heading = robot.GetPose().GetRotationAngle<'Z'>();
      
      Pose3d rotatedPose(heading + _turnAngle, Z_AXIS_3D,
                         robot.GetPose().GetTranslation());
      
      SetGoal(rotatedPose);
      
      return result;
    }
    
    
    
#pragma mark ---- MoveHeadToAngleAction ----
    
    MoveHeadToAngleAction::MoveHeadToAngleAction(const Radians& headAngle, const f32 tolerance)
    : _headAngle(headAngle)
    , _angleTolerance(tolerance)
    , _name("MoveHeadTo" + std::to_string(std::round(RAD_TO_DEG(_headAngle.ToFloat()))) + "DegAction")
    {

    }
    
    IActionRunner::ActionResult MoveHeadToAngleAction::Init(Robot &robot)
    {
      // TODO: Add ability to specify speed/accel
      if(robot.MoveHeadToAngle(_headAngle.ToFloat(), 5, 10) != RESULT_OK) {
        return FAILURE_ABORT;
      } else {
        return SUCCESS;
      }
    }
    
    IActionRunner::ActionResult MoveHeadToAngleAction::CheckIfDone(Robot &robot)
    {
      ActionResult result = RUNNING;
      
      // Wait to get a state message back from the physical robot saying its head
      // is in the commanded position
      // TODO: Is this really necessary in practice?
      if(NEAR(robot.GetHeadAngle(), _headAngle.ToFloat(), _angleTolerance.ToFloat())) {
        result = SUCCESS;
      }
      
      return result;
    }
         
    
#pragma mark ---- IDockAction ----
    
    // TODO: Define this as a constant parameter elsewhere
    #define MAX_DISTANCE_TO_PREDOCK_POSE 20.0f
    
    IDockAction::IDockAction(ObjectID objectID)
    : _dockObjectID(objectID)
    , _dockMarker(nullptr)
    {
      
    }
    
    IAction::ActionResult IDockAction::Init(Robot& robot)
    {
      // Wait for robot to be done moving before trying to dock with a new object, to
      // allow previously-added docking/moving actions to finish
      if(robot.IsMoving()) {
        return RUNNING;
      }
      
      // Make sure the object we were docking with still exists in the world
      ActionableObject* dockObject = dynamic_cast<ActionableObject*>(robot.GetBlockWorld().GetObjectByID(_dockObjectID));
      if(dockObject == nullptr) {
        PRINT_NAMED_ERROR("IDockAction.Init.ActionObjectNotFound",
                          "Action object with ID=%d no longer exists in the world.\n",
                          _dockObjectID.GetValue());
        
        return FAILURE_ABORT;
      }
      
      // Verify that we ended up near enough a PreActionPose of the right type
      std::vector<PreActionPose> preActionPoses;
      dockObject->GetCurrentPreActionPoses(preActionPoses, {GetPreActionType()});
      
      if(preActionPoses.empty()) {
        PRINT_NAMED_ERROR("IDockAction.Init.NoPreActionPoses",
                          "Action object with ID=%d returned no pre-action poses of the given type.\n",
                          _dockObjectID.GetValue());
        
        return FAILURE_ABORT;
      }

      const Point2f currentXY(robot.GetPose().GetTranslation().x(),
                              robot.GetPose().GetTranslation().y());
      
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
        PRINT_NAMED_INFO("IDockAction.Init.TooFarFromGoal",
                         "Robot is too far from pre-action pose (%.1fmm).", closestDist);
        return FAILURE_RETRY;
      }
      else {
        if(SelectDockAction(robot, dockObject) != RESULT_OK) {
          PRINT_NAMED_ERROR("IDockAction.CheckPreconditions.DockActionSelectionFailure",
                            "");
          return FAILURE_ABORT;
        }
        
        PRINT_NAMED_INFO("IDockAction.Init.BeginDocking",
                         "Robot is within %.1fmm of the nearest pre-action pose, "
                         "proceeding with docking.\n", closestDist);
      
        if(DockWithObjectHelper(robot, preActionPoses, closestIndex) == RESULT_OK) {
          return SUCCESS;
        } else {
          return FAILURE_ABORT;
        }
      }
      
    } // CheckPreconditions()
    
    
    IAction::ActionResult IDockAction::CheckIfDone(Robot& robot)
    {
      ActionResult actionResult = RUNNING;
      
      if (!robot.IsPickingOrPlacing() && !robot.IsMoving())
      {
        // Stopped executing docking path, and should have backed out by now,
        // and have head pointed at an angle to see where we just placed or
        // picked up from. So we will check if we see a block with the same
        // ID/Type as the one we were supposed to be picking or placing, in the
        // right position.
        
        actionResult = Verify(robot);
      }
      
      return actionResult;
    } // CheckIfDone()
    
    
    Result IDockAction::DockWithObjectHelper(Robot& robot,
                                             const std::vector<PreActionPose>& preActionPoses,
                                             const size_t closestIndex)
    {
      _dockMarker  = preActionPoses[closestIndex].GetMarker();
      
      PRINT_NAMED_INFO("IDockAction.DockWithObjectHelper.BeginDocking",
                       "Docking with marker %d (%s) using action %d.\n",
                       _dockMarker->GetCode(),
                       Vision::MarkerTypeStrings[_dockMarker->GetCode()], _dockAction);

      
      return robot.DockWithObject(_dockObjectID, _dockMarker, nullptr, _dockAction);
    }
    
    
#pragma mark ---- PickUpObjectAction ----
    
    PickUpObjectAction::PickUpObjectAction(ObjectID objectID)
    : IDockAction(objectID)
    {
      
    }
    
    const std::string& PickUpObjectAction::GetName() const
    {
      static const std::string name("PickUpObjectAction");
      return name;
    }
    
    Result PickUpObjectAction::SelectDockAction(Robot& robot, ActionableObject* object)
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
        if(robot.IsCarryingObject()) {
          PRINT_INFO("Already carrying object. Can't dock to high object. Aborting.\n");
          return RESULT_FAIL;
          
        } else {
          _dockAction = DA_PICKUP_HIGH;
        }
      } else if (robot.IsCarryingObject()) {
        _dockAction = DA_PLACE_HIGH;
      }
      
      return RESULT_OK;
    } // SelectDockAction()
    
    IAction::ActionResult PickUpObjectAction::Verify(Robot& robot) const
    {
      if(robot.IsCarryingObject() == false) {
        PRINT_NAMED_ERROR("PickUpObjectAction.Verify.RobotNotCarryignObject",
                          "Expecting robot to think its carrying an object at this point.\n");
        return FAILURE_ABORT;
      }
      
      BlockWorld& blockWorld = robot.GetBlockWorld();
      
      // We should _not_ still see a object with the
      // same type as the one we were supposed to pick up in that
      // block's original position because we should now be carrying it.
      Vision::ObservableObject* carryObject = blockWorld.GetObjectByID(robot.GetCarryingObject());
      if(carryObject == nullptr) {
        PRINT_NAMED_ERROR("PickUpObjectAction.Verify.CarryObjectNoLongerExists",
                          "Object %d we were carrying no longer exists in the world.\n",
                          robot.GetCarryingObject().GetValue());
        return FAILURE_ABORT;
      }
      
      const BlockWorld::ObjectsMapByID_t& objectsWithType = blockWorld.GetExistingObjectsByType(carryObject->GetType());
      
      bool objectInOriginalPoseFound = false;
      for(auto object : objectsWithType) {
        // TODO: is it safe to always have useAbsRotation=true here?
        if(object.second->GetPose().IsSameAs_WithAmbiguity(_dockObjectOrigPose, carryObject->
                                                           GetRotationAmbiguities(),
                                                           carryObject->GetSameDistanceTolerance(),
                                                           carryObject->GetSameAngleTolerance(), true))
        {
          objectInOriginalPoseFound = true;
          break;
        }
      }
      
      if(objectInOriginalPoseFound)
      {
        // Must not actually be carrying the object I thought I was!
        blockWorld.ClearObject(robot.GetCarryingObject());
        robot.UnSetCarryingObject();
        
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
    
    PutDownObjectAction::PutDownObjectAction()
    {
      
    }
    
    const std::string& PutDownObjectAction::GetName() const
    {
      static const std::string name("PutDownObjectAction");
      return name;
    }
   
    IAction::ActionResult PutDownObjectAction::Init(Robot& robot)
    {
      ActionResult result = RUNNING;
      
      // Wait for robot to stop moving before proceeding with pre-conditions
      if (robot.IsMoving() == false)
      {
        // Robot must be carrying something to put something down!
        if(robot.IsCarryingObject() == false) {
          PRINT_NAMED_ERROR("PutDownObjectAction.CheckPreconditions.NotCarryingObject",
                            "Robot %d executing PutDownObjectAction but not carrying object.\n", robot.GetID());
          result = FAILURE_ABORT;
        } else {
          
          _carryingObjectID  = robot.GetCarryingObject();
          _carryObjectMarker = robot.GetCarryingMarker();
        
          if(robot.PlaceObjectOnGround() == RESULT_OK)
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
    
    
    IAction::ActionResult PutDownObjectAction::CheckIfDone(Robot& robot)
    {
      ActionResult actionResult = RUNNING;
      
      // Wait for robot to report it is done picking/placing and that it's not
      // moving
      if (!robot.IsPickingOrPlacing() && !robot.IsMoving())
      {
        // Stopped executing docking path, and should have placed carried block
        // and backed out by now, and have head pointed at an angle to see
        // where we just placed or picked up from.
        // So we will check if we see a block with the same
        // ID/Type as the one we were supposed to be picking or placing, in the
        // right position.

        // TODO: check to see it ended up in the right place?
        Vision::ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_carryingObjectID);
        if(object == nullptr) {
          PRINT_NAMED_ERROR("PutDownObjectAction.CheckIfDone.CarryObjectNoLongerExists",
                            "Object %d we were carrying no longer exists in the world.\n",
                            robot.GetCarryingObject().GetValue());
          return FAILURE_ABORT;
        }
        else if(object->GetLastObservedTime() > (robot.GetLastMsgTimestamp()-500))
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
          robot.SetObjectAsAttachedToLift(_carryingObjectID, _carryObjectMarker); // re-pickup object to attach it to the lift again
          return FAILURE_RETRY;
        }
        
      } // if robot is not picking/placing or moving
      
      return actionResult;
      
    } // CheckIfDone()
    

    
#pragma mark ---- CrossBridgeAction ----
    
    CrossBridgeAction::CrossBridgeAction(ObjectID bridgeID)
    : IDockAction(bridgeID)
    , _dockMarker2(nullptr)
    {
      
    }
    
    const std::string& CrossBridgeAction::GetName() const
    {
      static const std::string name("CrossBridgeAction");
      return name;
    }
    
    Result CrossBridgeAction::DockWithObjectHelper(Robot & robot,
                                                   const std::vector<PreActionPose>& preActionPoses,
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
      
      return robot.DockWithObject(_dockObjectID, _dockMarker, _dockMarker2, _dockAction);
    }
    
    Result CrossBridgeAction::SelectDockAction(Robot& robot, ActionableObject* object)
    {
      _dockAction = DA_CROSS_BRIDGE;
      return RESULT_OK;
    } // SelectDockAction()
    
    IAction::ActionResult CrossBridgeAction::Verify(Robot& robot) const
    {
      // TODO: Need some kind of verificaiton here?
      PRINT_NAMED_INFO("CrossBridgeAction.Verify.BridgeCrossingComplete",
                       "Robot has completed crossing a bridge.\n");
      return SUCCESS;
    } // Verify()
    
    
#pragma mark ---- AscendOrDescendRampAction ----
    
    AscendOrDescendRampAction::AscendOrDescendRampAction(ObjectID rampID)
    : IDockAction(rampID)
    {

    }
    
    const std::string& AscendOrDescendRampAction::GetName() const
    {
      static const std::string name("AscendOrDescendRampAction");
      return name;
    }
    
    Result AscendOrDescendRampAction::SelectDockAction(Robot& robot, ActionableObject* object)
    {
      Ramp* ramp = dynamic_cast<Ramp*>(object);
      if(ramp == nullptr) {
        PRINT_NAMED_ERROR("AscendOrDescendRampAction.SelectDockAction.NotRampObject",
                          "Could not cast generic ActionableObject into Ramp object.\n");
        return RESULT_FAIL;
      }
      
      Result result = RESULT_OK;
      
      // Choose ascent or descent
      const Ramp::TraversalDirection direction = ramp->WillAscendOrDescend(robot.GetPose());
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
    
      // Tell robot which ramp it will be using, and in which direction
      robot.SetRamp(_dockObjectID, direction);
            
      return result;
      
    } // SelectDockAction()
    
    
    IAction::ActionResult AscendOrDescendRampAction::Verify(Robot& robot) const
    {
      // TODO: Need to do some kind of verification here?
      PRINT_NAMED_INFO("AscendOrDescendRampAction.Verify.RampAscentOrDescentComplete",
                       "Robot has completed going up/down ramp.\n");
      
      return SUCCESS;
    } // Verify()
    
    
#pragma mark ---- PlayAnimationAction ----
    
    PlayAnimationAction::PlayAnimationAction(AnimationID_t animID)
    : _animID(animID)
    , _name("PlayAnimation" + std::to_string(_animID) + "Action")
    {
      
    }
    
    IAction::ActionResult PlayAnimationAction::CheckIfDone(Robot& robot)
    {
      if(robot.PlayAnimation(_animID) == RESULT_OK) {
        return SUCCESS;
      } else {
        return FAILURE_ABORT;
      }
    }
    
#pragma mark ---- PlaySoundAction ----
    
    PlaySoundAction::PlaySoundAction(SoundID_t soundID)
    : _soundID(soundID)
    , _name("PlaySound" + std::to_string(_soundID) + "Action")
    {
      
    }

    IAction::ActionResult PlaySoundAction::CheckIfDone(Robot& robot)
    {
      // TODO: Implement!
      return FAILURE_ABORT;
    }
    
#pragma mark ---- WaitAction ----
    
    WaitAction::WaitAction(f32 waitTimeInSeconds)
    : _waitTimeInSeconds(waitTimeInSeconds)
    , _doneTimeInSeconds(-1.f)
    {
      // Put the wait time with two decimals of precision in the action's name
      char tempBuffer[32];
      snprintf(tempBuffer, 32, "Wait%.2fSecondsAction", _waitTimeInSeconds);
      _name = tempBuffer;
    }
    
    IActionRunner::ActionResult WaitAction::Init(Robot& robot)
    {
      _doneTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _waitTimeInSeconds;
      return SUCCESS;
    }
    
    IActionRunner::ActionResult WaitAction::CheckIfDone(Robot& robot)
    {
      assert(_doneTimeInSeconds > 0.f);
      if(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _doneTimeInSeconds) {
        return SUCCESS;
      } else {
        return RUNNING;
      }
    }
    
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
