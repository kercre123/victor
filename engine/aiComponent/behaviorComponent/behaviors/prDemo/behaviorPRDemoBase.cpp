/**
 * File: BehaviorPRDemoBase.cpp
 *
 * Author: Brad
 * Created: 2018-07-06
 *
 * Description: Base behavior for running the PR demo
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/prDemo/behaviorPRDemoBase.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/activeBehaviorIterator.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/prDemo/behaviorPRDemo.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/components/mics/micComponent.h"

namespace Anki {
namespace Cozmo {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPRDemoBase::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPRDemoBase::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPRDemoBase::BehaviorPRDemoBase(const Json::Value& config)
  : ICozmoBehavior(config)
{
  // TODO: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPRDemoBase::~BehaviorPRDemoBase()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPRDemoBase::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig.sleepingBehavior.get() );
  delegates.insert( _iConfig.wakeUpBehavior.get() );
  delegates.insert( _iConfig.mainBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPRDemoBase::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPRDemoBase::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(PRDemoStateMachine),
                                 BEHAVIOR_CLASS(PRDemo),
                                 _iConfig.demoBehavior);

  _iConfig.sleepingBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(PRDemoSleeping));
  ANKI_VERIFY(_iConfig.sleepingBehavior != nullptr, "BehaviorPRDemoBase.CouldntGetSleepingBehavior", "");
  
  _iConfig.wakeUpBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(SleepingWakeUp));
  ANKI_VERIFY(_iConfig.wakeUpBehavior != nullptr, "BehaviorPRDemoBase.CouldntGetWakeupBehavior", "");

  // TODO:(bn) at least read this last one from config?
  _iConfig.mainBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ModeSelector));
  ANKI_VERIFY(_iConfig.mainBehavior != nullptr, "BehaviorPRDemoBase.CouldntGetMainBehavior", "");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPRDemoBase::OnBehaviorActivated()
{

  // for now we want to disable streaming until we're awake
  GetBEI().GetMicComponent().SetShouldStreamAfterWakeWord( false );
  
  if( _iConfig.sleepingBehavior->WantsToBeActivated() ) {
    DelegateIfInControl(_iConfig.sleepingBehavior.get(), [this]() {
        GetBEI().GetMicComponent().SetShouldStreamAfterWakeWord( true );
        if( _iConfig.wakeUpBehavior->WantsToBeActivated() ) {
          DelegateIfInControl(_iConfig.wakeUpBehavior.get());
        }
      });
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPRDemoBase::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  auto& uic = GetBehaviorComp<UserIntentComponent>();
  if( uic.IsAnyUserIntentPending() ) {
    const auto* userIntentDataPtr = uic.GetPendingUserIntent();
    if( userIntentDataPtr != nullptr ) {
      _iConfig.demoBehavior->InformOfVoiceIntent(userIntentDataPtr->intent.GetTag());
    }
  }

  const auto& behaviorIterator = GetBehaviorComp<ActiveBehaviorIterator>();

  const size_t currTick = BaseStationTimer::getInstance()->GetTickCount();
  if( behaviorIterator.GetLastTickBehaviorStackChanged() + 1 >= currTick ) {
    // check if timer ringing behavior is active
    bool active = false;
    auto callback = [&active](const ICozmoBehavior& behavior) {
      ActiveFeature feature = ActiveFeature::NoFeature;
      if( behavior.GetAssociatedActiveFeature(feature) &&
          feature == ActiveFeature::TimerRinging ) {
        active = true;
      }
      return true; // Iterate the whole stack
    };

    behaviorIterator.IterateActiveCozmoBehaviorsForward(callback);

    if( active ) {
      // TODO:(bn) maybe don't call this every tick?
      _iConfig.demoBehavior->InformTimerIsRinging();
    }
  }

  if( !IsControlDelegated() ) {
    if( _iConfig.mainBehavior->WantsToBeActivated() ) {
      DelegateIfInControl(_iConfig.mainBehavior.get());
    }
  }
  
}


}
}
