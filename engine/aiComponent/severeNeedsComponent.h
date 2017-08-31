/**
* File: severeNeedsComponent.h
*
* Author: Kevin M. Karol
* Created: 08/23/17
*
* Description: Component which manages Cozmo's severe needs state
* and its associated idles/activation/etc.
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_SevereNeedsComponent_H__
#define __Cozmo_Basestation_BehaviorSystem_SevereNeedsComponent_H__

#include "clad/types/needsSystemTypes.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {

class Robot;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// SevereNeedsComponent
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class SevereNeedsComponent
{
public:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor
  SevereNeedsComponent(Robot& robot);

  virtual ~SevereNeedsComponent();
  
  // initializes the whiteboard, registers to events
  void Init();

  // Tick the whiteboard
  void Update();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Needs states
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Indicates which severe needs state is currently being _expressed_ in freeplay, or to Count if no severe
  // states are being expressed. Note that this is different from the current values of the needs from the
  // needs manager because the activity system usually takes some time to transition into / out of a given
  // activity, so even though the current value may be critical for a need, the behavior system may still be
  // expressing the previous state. This value will automatically be cleared to Count if the needs system says
  // the given need is no longer critically low
  NeedId GetSevereNeedExpression() const { return _severeNeedExpression; }
  bool HasSevereNeedExpression() const { return _severeNeedExpression != NeedId::Count; }

  void SetSevereNeedExpression(NeedId need);
  void ClearSevereNeedExpression();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Events
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // template for all events we subscribe to
  template<typename T>
  void HandleMessage(const T& msg);

private:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // the robot this whiteboard belongs to
  Robot& _robot;
  
  // signal handles for events we register to. These are currently unsubscribed when destroyed
  std::vector<Signal::SmartHandle> _signalHandles;
  // current severe need based on activity expression (may differ from needs manager state)
  NeedId _severeNeedExpression;
};

} // namespace Cozmo
} // namespace Anki

#endif //
