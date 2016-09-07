/**
 * File: compoundActions.h
 *
 * Author: Andrew Stein
 * Date:   7/9/2014
 *
 * Description: Defines compound actions, which are groups of IActions to be
 *              run together in series or in parallel.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_COMPOUND_ACTIONS_H
#define ANKI_COZMO_COMPOUND_ACTIONS_H

#include "anki/cozmo/basestation/actions/actionInterface.h"

namespace Anki {
  namespace Cozmo {
    
    // Interface for compound actions, which are fixed sets of actions to be
    // run together or in order (determined by derived type)
    class ICompoundAction : public IActionRunner
    {
    public:
      ICompoundAction(Robot& robot, std::list<IActionRunner*> actions);
      
      virtual void AddAction(IActionRunner* action, bool ignoreFailure = false, bool emitCompletionSignal = false);
      
      // First calls cleanup on any constituent actions and then removes them
      // from this compound action completely.
      void ClearActions();
      
      const std::list<IActionRunner*>& GetActionList() const { return _actions; }
      
      // Constituent actions will be deleted upon destruction of the group
      virtual ~ICompoundAction();
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;

      // The proxy action, if set, is the one whose type and completion info are used.
      // Specify it by the constituent action's tag.
      void SetProxyTag(u32 tag);
      
      // Sets whether or not to delete actions from the compound action when they complete
      // By default actions will be destroyed on completion
      void SetDeleteActionOnCompletion(bool deleteOnCompletion);

    protected:
      
      // Call the constituent actions' Reset() methods and mark them each not done.
      virtual void Reset(bool shouldUnlockTracks = true) override;
      
      std::list<IActionRunner*> _actions;
      
      bool ShouldIgnoreFailure(IActionRunner* action) const;
      
      // Stack of pairs of actionCompletionUnions and actionTypes of the already completed actions
      struct CompletionData {
        ActionCompletedUnion  completionUnion;
        RobotActionType       type;
      };
      
      // Map of action tag to completion data
      std::map<u32, CompletionData> _completedActionInfoStack;
      
      // NOTE: Moves currentAction iterator to next action after deleting
      void StoreUnionAndDelete(std::list<IActionRunner*>::iterator& currentAction);
      
    private:
      
      // If actions are in this list, we ignore their failures
      std::set<IActionRunner*> _ignoreFailure;
      u32  _proxyTag;
      bool _proxySet = false;
      
      bool _deleteActionOnCompletion = true;
      
      void DeleteActions();
    };
    
    
    // Executes a fixed set of actions sequentially
    class CompoundActionSequential : public ICompoundAction
    {
    public:
      CompoundActionSequential(Robot& robot);
      CompoundActionSequential(Robot& robot, std::list<IActionRunner*> actions);
      
      // Add a delay, in seconds, between running each action in the group.
      // Default is 0 (no delay).
      void SetDelayBetweenActions(f32 seconds);
      
      // Called at the very beginning of UpdateInternal, so derived classes can
      // do additional work. If this does not return RESULT_OK, then UpdateInternal
      // will return ActionResult::FAILURE_ABORT.
      virtual Result UpdateDerived() { return RESULT_OK; }
      
    private:
      virtual void Reset(bool shouldUnlockTracks = true) override final;
      
      virtual ActionResult UpdateInternal() override final;
      
      ActionResult MoveToNextAction(double currentTime);
      
      f32 _delayBetweenActionsInSeconds;
      f32 _waitUntilTime;
      std::list<IActionRunner*>::iterator _currentAction;
      bool _wasJustReset;
      
    }; // class CompoundActionSequential
    
    inline void CompoundActionSequential::SetDelayBetweenActions(f32 seconds) {
      _delayBetweenActionsInSeconds = seconds;
    }
    
    // Executes a fixed set of actions in parallel
    class CompoundActionParallel : public ICompoundAction
    {
    public:
      CompoundActionParallel(Robot& robot);
      CompoundActionParallel(Robot& robot, std::list<IActionRunner*> actions);
      
    protected:
      
      virtual ActionResult UpdateInternal() override final;
      
    }; // class CompoundActionParallel
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_COMPOUND_ACTIONS_H


