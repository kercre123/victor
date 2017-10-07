/**
 * File: devBaseRunnable.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-02
 *
 * Description: An "init"-like base runnable that sits at the bottom of the behavior stack and handles
 *              developer tools, such as messages coming in from webots. It is completely optional, and by
 *              default just delegates to the passed in delegate
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_DevBaseRunnable_H__
#define __Engine_AiComponent_BehaviorComponent_DevBaseRunnable_H__

#include "engine/aiComponent/behaviorComponent/iBehavior.h"
#include "util/signals/simpleSignal_fwd.h"
#include <vector>
#include <set>

namespace Anki {
namespace Cozmo {

class DevBaseRunnable : public IBehavior
{
public:

  // pass in the delegate that this runnable should default to (or nullptr for it to not delegate at all)
  explicit DevBaseRunnable( IBehavior* initialDelegate );

protected:

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual bool WantsToBeActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) const override {
    return true;
  }

  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnDeactivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) override;

private:

  std::set<IBehavior*> _possibleDelegates;
  
  std::vector<Signal::SmartHandle> _eventHandlers;
  IBehavior* _initialDelegate = nullptr;

  IBehavior* _pendingDelegate = nullptr;
  int _pendingDelegateRepeatCount = 0;
  bool _shouldCancelDelegates = false;

  
};

}
}

#endif
