/**
 * File: devBaseRunnable.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-02
 *
 * Description: An "init"-like base runnable that sits at the bottom of the behavior stack and handles
 *              developer tools, such as messages coming in from webots. It is completely optional, and by
 *              default just delegates to the passed in delegate
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/devBaseRunnable.h"

#include "clad/externalInterface/messageGameToEngine.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DevBaseRunnable::DevBaseRunnable( IBSRunnable* initialDelegate )
  : IBSRunnable("DevBase")
  , _initialDelegate(initialDelegate)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DevBaseRunnable::InitInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  // Because of dev messages, this behavior could delegate to any activity or behavior. Add all behavior IDs
  // as possible delegates
  for( int i=0; i<(int)BehaviorIDNumEntries; ++i ) {
    const BehaviorID bID = static_cast<BehaviorID>(i);
    IBehaviorPtr behavior = behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByID( bID );
    if( behavior != nullptr ) {
      _possibleDelegates.insert(behavior.get());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DevBaseRunnable::GetAllDelegates(std::set<IBSRunnable*>& delegates) const
{
  // add all behaviors (stored during init)
  delegates.insert(_possibleDelegates.begin(), _possibleDelegates.end());

  // also add the initial delegate (if there is one)
  if( _initialDelegate != nullptr ) {
    delegates.insert( _initialDelegate );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DevBaseRunnable::OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  auto robotExternalInterface = behaviorExternalInterface.GetRobotExternalInterface().lock();
  if (robotExternalInterface != nullptr)
  {
    // TODO:(bn) figure out how message handling should work in the new behavior system (it's not this!)

    const auto& behaviorContainer = behaviorExternalInterface.GetBehaviorContainer();

    using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;
    
    auto handleExecuteByID = [this, &behaviorContainer](const GameToEngineEvent& event) {
      const auto& msg = event.GetData().Get_ExecuteBehaviorByID();
      auto behaviorPtr = behaviorContainer.FindBehaviorByID( msg.behaviorID );
        
      if( behaviorPtr ) {

        if( _pendingDelegate != nullptr ) {
          PRINT_NAMED_WARNING("DevBaseRunnable.HandleMessage.ExecuteBehaviorByID.AlreadyPending",
                              "Setting pending behavior to '%s', but '%s' is already pending",
                              behaviorPtr->GetIDStr().c_str(),
                              _pendingDelegate->GetPrintableID().c_str());
        }
    
        _pendingDelegate = behaviorPtr.get();
        _pendingDelegateRepeatCount = msg.numRuns;
        _shouldCancelDelegates = true;
      }
      else {
        PRINT_NAMED_WARNING("DevBaseRunnable.HandleMessage.ExecuteBehaviorByID.NoBehavior",
                            "Couldn't find behavior for id '%s'",
                            BehaviorIDToString(msg.behaviorID));
      }
    };
    
    _eventHandlers.push_back(robotExternalInterface->Subscribe(
                               ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByID,
                               handleExecuteByID));
  }

  if( _initialDelegate != nullptr ) {
    _pendingDelegate = _initialDelegate;
    _pendingDelegateRepeatCount = 1;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DevBaseRunnable::OnDeactivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  _eventHandlers.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DevBaseRunnable::UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface)
{

  auto delegationComponent = behaviorExternalInterface.GetDelegationComponent().lock();
  if( !delegationComponent ) {
    return;
  }
  
  if( _shouldCancelDelegates ) {
    delegationComponent->CancelDelegates(this);
    _shouldCancelDelegates = false;
  }
  
  if( _pendingDelegate != nullptr &&
      !delegationComponent->IsControlDelegated(this) ) {
    // TEMP: // TODO:(bn) kevink: I don't think we should pass in the same interface to WantstoBeActivated. Or
    // rather, maybe we can, but then it modifies it for WantsToBeActivatedInternal? Otherwise
    // WantsToBeActivated could do the same things Update can

    if( _pendingDelegate->WantsToBeActivated( behaviorExternalInterface ) ) {
      const bool ret = DelegateIfInControl(behaviorExternalInterface, _pendingDelegate);
      if( ret ) {
        // we successfully delegated, so decrement repeat count (if it's negative, that means loop forever)
        if( _pendingDelegateRepeatCount > 0 ) {
          _pendingDelegateRepeatCount--;
        }
        if( _pendingDelegateRepeatCount == 0 ){
          _pendingDelegate = nullptr;
        }
      }
    }
  }
}

// TEMP: // TEMP: new bug: when behavior is started by webots command and it's first action completes, it
// doesn't get the callback. Looks like there's an issue with the action completed message handling, so we'll
// need to fix that



}
}
