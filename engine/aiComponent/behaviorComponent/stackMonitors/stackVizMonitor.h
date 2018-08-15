/**
 * File: stackVizMonitor.h
 *
 * Author: ross
 * Created: 2018-06 26
 *
 * Description: Sends stack info to webviz and webots
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_StackMonitors_StackVizMonitor_H__
#define __Engine_AiComponent_BehaviorComponent_StackMonitors_StackVizMonitor_H__

#include "engine/aiComponent/behaviorComponent/stackMonitors/iStackMonitor.h"
#include <vector>

namespace Anki {
namespace Vector {

class BehaviorExternalInterface;
class BehaviorStack;
class IBehavior;
  
class StackVizMonitor : public IStackMonitor
{
public:
  StackVizMonitor() {}
  virtual ~StackVizMonitor() = default;
  
  virtual void NotifyOfChange( BehaviorExternalInterface& bei,
                               const std::vector<IBehavior*>& stack,
                               const BehaviorStack* stackComponent );
private:
  void SendToWebViz( BehaviorExternalInterface& bei,
                     const std::vector<IBehavior*>& stack,
                     const BehaviorStack* stackComponent ) const;
  
  void SendToWebots( BehaviorExternalInterface& bei,
                     const std::vector<IBehavior*>& stack,
                     const BehaviorStack* stackComponent ) const;
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_StackMonitors_StackVizMonitor_H__
