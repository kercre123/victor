/**
 * File: devBaseBehavior.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-02
 *
 * Description: An "init"-like base behavior that sits at the bottom of the behavior stack and handles
 *              developer tools, such as messages coming in from webots. It is completely optional, and by
 *              default just delegates to the passed in delegate
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_DevBaseBehavior_H__
#define __Engine_AiComponent_BehaviorComponent_DevBaseBehavior_H__

#include "engine/aiComponent/behaviorComponent/iBehavior.h"
#include "util/signals/simpleSignal_fwd.h"
#include <vector>
#include <set>

namespace Anki {
namespace Cozmo {

class DevBaseBehavior : public IBehavior
{
public:

  // pass in the delegate that this behavior should default to (or nullptr for it to not delegate at all)
  explicit DevBaseBehavior( IBehavior* initialDelegate );
  virtual ~DevBaseBehavior();

  
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
  
  IBehavior* _initialDelegate = nullptr;

  IBehavior* _pendingDelegate = nullptr;
  int _pendingDelegateRepeatCount = 0;
  bool _shouldCancelDelegates = false;

  
};

}
}

#endif
