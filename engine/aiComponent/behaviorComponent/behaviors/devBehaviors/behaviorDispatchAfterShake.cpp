/**
 * File: behaviorDispatchAfterShake.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-01-18
 *
 * Description: Simple behavior to wait for the robot to be shaken and placed down before delegating to
 *              another data-defined behavior
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDispatchAfterShake.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"

#include "util/console/consoleInterface.h"

#include <set>

namespace Anki {
namespace Cozmo {

namespace{
const float kAccelMagnitudeShakingStartedThreshold = 16000.f;

// set to true from console to fake the "shake" (shake is hard to do in webots sim)
CONSOLE_VAR(bool, kDevDispatchAfterShake, "DevBaseBehavior", false);

}

BehaviorDispatchAfterShake::BehaviorDispatchAfterShake(const Json::Value& config)
  : ICozmoBehavior(config)
{
  _delegateID = BehaviorTypesWrapper::BehaviorIDFromString(config["behavior"].asString());
}

void BehaviorDispatchAfterShake::OnBehaviorActivated()
{
  _hasBeenShaken = false;
}

void BehaviorDispatchAfterShake::InitBehavior()
{
  _delegate = GetBEI().GetBehaviorContainer().FindBehaviorByID(_delegateID);

  ANKI_VERIFY( _delegate,
               "BehaviorDispatchAfterShake.Delegate.InvalidBehavior",
               "could not get pointer for behavior '%s'",
               BehaviorTypesWrapper::BehaviorIDToString(_delegateID));
}

void BehaviorDispatchAfterShake::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_delegate.get());
}

void BehaviorDispatchAfterShake::BehaviorUpdate()
{
  if(!IsActivated() || IsControlDelegated()){
    return;
  }

  const auto& robotInfo = GetBEI().GetRobotInfo();
  
  if( !_hasBeenShaken ) {
    if( kDevDispatchAfterShake ) {
      _hasBeenShaken = true;
      kDevDispatchAfterShake = false;
    }
    else {
      const bool isBeingShaken = (robotInfo.GetHeadAccelMagnitudeFiltered() > kAccelMagnitudeShakingStartedThreshold);
      _hasBeenShaken = isBeingShaken;
    }
  }

  if( _hasBeenShaken ) {
    const bool isOnTreads = (robotInfo.GetOffTreadsState() == OffTreadsState::OnTreads);
    if( isOnTreads ) {
      // time to delegate to the data defined delegate
      DEV_ASSERT(_delegate, "BehaviorDispatchAfterShake.Update.NullDelegate");
      
      if( _delegate->WantsToBeActivated() ) {
        DelegateIfInControl( _delegate.get() );

        // clear shaken so you have to shake again to run the behavior again if it completes
        _hasBeenShaken = false;
        return;
      }
    }
  }
}

}
}
