/**
 * File: iStackMonitor.h
 *
 * Author: ross
 * Created: 2018-06 26
 *
 * Description: Interface for stack monitors
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_StackMonitors_IStackMonitor_H__
#define __Engine_AiComponent_BehaviorComponent_StackMonitors_IStackMonitor_H__

#include <vector>

namespace Anki {
namespace Vector {

class BehaviorExternalInterface;
class BehaviorStack;
class IBehavior;
  
class IStackMonitor
{
public:
  IStackMonitor(){}
  virtual ~IStackMonitor() = default;
  
  virtual void NotifyOfChange( BehaviorExternalInterface& bei,
                               const std::vector<IBehavior*>& stack,
                               const BehaviorStack* stackComponent ) = 0;
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_StackMonitors_IStackMonitor_H__
