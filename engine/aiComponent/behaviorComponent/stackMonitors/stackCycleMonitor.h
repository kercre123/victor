/**
 * File: stackCycleMonitor.h
 *
 * Author: ross
 * Created: 2018-06 26
 *
 * Description: Monitors the stack for cycles, which usually indicate that the robot is looking stupid.
 *              For now, this is only looking for _rapid_ cycles, which usually indicates a bug, but eventually
 *              we may consider longer loops.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_StackMonitors_StackCycleMonitor_H__
#define __Engine_AiComponent_BehaviorComponent_StackMonitors_StackCycleMonitor_H__

#include "engine/aiComponent/behaviorComponent/stackMonitors/iStackMonitor.h"
#include "util/container/circularBuffer.h"
#include <vector>

namespace Anki {

  
namespace Vector {

class BehaviorExternalInterface;
class BehaviorStack;
class IBehavior;
  
template <typename T>
class CircularBuffer;
  
class StackCycleMonitor : public IStackMonitor
{
public:
  StackCycleMonitor();
  virtual ~StackCycleMonitor() = default;
  
  virtual void NotifyOfChange( BehaviorExternalInterface& bei,
                               const std::vector<IBehavior*>& stack,
                               const BehaviorStack* stackComponent ) override;
private:
  
  void SwitchToSafeStack( BehaviorExternalInterface& bei, IBehavior* oldBaseOfStack ) const;
  bool CheckForCycle() const;
  
  size_t _lastTick;
  Util::CircularBuffer<const IBehavior*> _recentBehaviors;
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_StackMonitors_StackCycleMonitor_H__
