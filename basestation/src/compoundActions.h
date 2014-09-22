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

#include "actionInterface.h"

namespace Anki {
  namespace Cozmo {
    
    // Interface for compound actions, which are fixed sets of actions to be
    // run together or in order (determined by derived type)
    class ICompoundAction : public IActionRunner
    {
    public:
      ICompoundAction(std::initializer_list<IActionRunner*> actions);
      
      // Constituent actions will be deleted upon destruction of the group
      virtual ~ICompoundAction();
      
      virtual const std::string& GetName() const override { return _name; }
      
    protected:
      
      // Call the constituent actions' Reset() methods and mark them each not done.
      virtual void Reset() override;
      
      std::list<std::pair<bool, IActionRunner*> > _actions;
      std::string _name;
    };
    
    
    // Executes a fixed set of actions sequentially
    class CompoundActionSequential : public ICompoundAction
    {
    public:
      
      CompoundActionSequential(std::initializer_list<IActionRunner*> actions);
      
      virtual ActionResult Update(Robot& robot) override final;
      
      // Add a delay, in seconds, between running each action in the group.
      // Default is 0 (no delay).
      void SetDelayBetweenActions(f32 seconds);
      
    private:
      virtual void Reset() override;
      
      f32 _delayBetweenActionsInSeconds;
      f32 _waitUntilTime;
      std::list<std::pair<bool,IActionRunner*> >::iterator _currentActionPair;
    }; // class CompoundActionSequential
    
    inline void CompoundActionSequential::SetDelayBetweenActions(f32 seconds) {
      _delayBetweenActionsInSeconds = seconds;
    }
    
    // Executes a fixed set of actions in parallel
    class CompoundActionParallel : public ICompoundAction
    {
    public:
      
      CompoundActionParallel(std::initializer_list<IActionRunner*> actions);
      
      virtual ActionResult Update(Robot& robot) override final;
      
    protected:
      CompoundActionParallel();
      
    }; // class CompoundActionParallel
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_COMPOUND_ACTIONS_H


