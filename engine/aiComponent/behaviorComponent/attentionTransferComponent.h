/**
 * File: attentionTransferComponent.h
 *
 * Author: Brad Neuman
 * Created: 2018-06-15
 *
 * Description: Component to manage state related to attention transfer between the robot and app
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_AttentionTransferComponent_H__
#define __Engine_AiComponent_BehaviorComponent_AttentionTransferComponent_H__

#include "clad/types/behaviorComponent/attentionTransferTypes.h"
#include "coretech/common/engine/utils/recentOccurrenceTracker.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

class IGatewayInterface;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class AttentionTransferComponent : public IDependencyManagedComponent<BCComponentID>
                                 , public Anki::Util::noncopyable
{
public:

  AttentionTransferComponent();

  virtual void GetInitDependencies( BCCompIDSet& dependencies ) const override {
    dependencies.insert(BCComponentID::RobotInfo);
  }
  virtual void InitDependent( Robot* robot, const BCCompMap& dependentComps ) override;

  // Notify this component that a possible reason for attention transfer just happened
  void PossibleAttentionTransferNeeded(AttentionTransferReason reason);

  // Tell the component that an attention transfer happened for the given reason
  void OnAttentionTransferred(AttentionTransferReason reason);

  // Tell the component to reset tracking for the given reason (without trigger the reason)
  void ResetAttentionTransfer(AttentionTransferReason reason);

  // Get a handle that will return that conditions are met when an attention transfer reason has happened
  // numberOfTimes in amountOfSeconds.
  RecentOccurrenceTracker::Handle GetRecentOccurrenceHandle(AttentionTransferReason reason,
                                                            int numberOfTimes,
                                                            float amountOfSeconds);
private:

  RecentOccurrenceTracker& GetTracker(AttentionTransferReason reason);

  void HandleAppRequest(const AppToEngineEvent& event);
  
  // when the tracker has it's conditions met, we should do the attention transfer. Otherwise, we should do
  // the fallback (if defined)
  std::map<AttentionTransferReason, RecentOccurrenceTracker> _transferTrackers;

  AttentionTransferReason _lastAttentionTransferReason = AttentionTransferReason::Invalid;
  float _lastAttentionTransferTime_s = 0.0f;
  
  IGatewayInterface* _gi = nullptr;
  std::vector<Signal::SmartHandle> _eventHandles;
};


}
}

#endif
