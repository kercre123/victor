/**
* File: iBehaviorRunner.h
*
* Author: Kevin Karol
* Date:   8/17/2017
*
* Description: Coretech interface for robot component that runs behaviors
*
* Copyright: Anki, Inc. 2017
**/

#ifndef CORETECH_IBEHAVIOR_RUNNER
#define CORETECH_IBEHAVIOR_RUNNER


namespace Anki {
namespace Cozmo {

// Forward declarations
class BehaviorExternalInterface;
class IBehavior;

struct BehaviorRunningInfo;

class IBehaviorRunner
{
public:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/Destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  IBehaviorRunner(){};
  virtual ~IBehaviorRunner() {};
  
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  virtual void Update(BehaviorExternalInterface& behaviorExternalInterface) {};
  
  virtual bool IsControlDelegated(const IBehavior* delegator) = 0;
  virtual bool CanDelegate(IBehavior* delegator) = 0;
  virtual bool Delegate(IBehavior* delegator, IBehavior* delegated) = 0;
  virtual void CancelDelegates(IBehavior* delegator) = 0;
  virtual void CancelSelf(IBehavior* delegator) = 0;
  

}; // class IBehaviorRunner

} // namespace Cozmo
} // namespace Anki


#endif // CORETECH_IBEHAVIOR_RUNNER
