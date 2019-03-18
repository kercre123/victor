/**
* File: behaviorDispatcherPassThrough.h
*
* Author: Kevin M. Karol
* Created: 2/26/18
*
* Description: Defines the base class for a "Pass Through" behavior
*  This style of behavior has one and only one delegate which follow the lifecycle
*  of the pass through exactly. The pass through implementation can then perform
*  other updates (setting lights/coordinating between behaviors) but may not 
*  delegate to an alternative behavior or action
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_Behaviors_PassThroughBehavior_H__
#define __Engine_Behaviors_PassThroughBehavior_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

// forward declarations
class BehaviorProceduralClock;
class BehaviorAnimGetInLoop;
class TimerUtility;
// Specified in .cpp
class AnticTracker;

class BehaviorDispatcherPassThrough : public ICozmoBehavior
{
public:
  virtual ~BehaviorDispatcherPassThrough();

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorDispatcherPassThrough(const Json::Value& config);

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override final;

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override final;
  virtual void GetLinkedActivatableScopeBehaviors(std::set<IBehavior*>& delegates) const override {
    delegates.insert(_iConfig.delegate.get());
  };
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override final;
  virtual bool WantsToBeActivatedBehavior() const override final;
  virtual void OnBehaviorActivated() override final;
  virtual void BehaviorUpdate() override final;
  virtual void OnBehaviorDeactivated() override final;

  virtual void GetPassThroughJsonKeys(std::set<const char*>& expectedKeys) const {};
  virtual void InitPassThrough() {};
  virtual void OnPassThroughActivated() {};
  virtual void OnFirstPassThroughUpdate() {};
  virtual void PassThroughUpdate() {};
  virtual void OnPassThroughDeactivated() {};

private:
  struct InstanceConfig{
    std::string       delegateID;
    ICozmoBehaviorPtr delegate;
  };

  struct DynamicVariables{
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  // NOTE(GB): This is a hack for pass-through dispatchers to safely check modifiers of external
  // behaviors that they are dependent on for some reason. Doing this in the InitPassThrough()
  // function is not robust due to the fragileness of init order dependencies causing some behaviors
  // to not have their operation modifiers set up yet when the dispatcher is initialized.
  bool _hasUpdatedOnce = false;
  void OnFirstUpdate();
};

} // namespace Vector
} // namespace Anki


#endif // __Engine_Behaviors_PassThroughBehavior_H__
