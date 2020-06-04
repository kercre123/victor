/**
 * File: BehaviorCoordinateWhileHeldInPalm.cpp
 *
 * Author: Guillermo Bautista
 * Created: 2019-01-14
 *
 * Description: Behavior responsible for managing the robot's behaviors when held in a user's palm/hand
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/coordinators/behaviorCoordinateWhileHeldInPalm.h"

#include "clad/types/animationTrigger.h"

#include "engine/aiComponent/behaviorComponent/activeBehaviorIterator.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/heldInPalmTracker.h"
#include "util/console/consoleInterface.h"

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {

CONSOLE_VAR_RANGED(u32, kMaxTimeForInitialHeldInPalmReaction_ms, "HeldInPalm.Coordinator", 1000, 0, 5000);

namespace{
  const BehaviorID kHeldInPalmDispatcher          = BEHAVIOR_ID(HeldInPalmDispatcher);
  const BehaviorID kInitialHeldInPalmReaction     = BEHAVIOR_ID(InitialHeldInPalmReaction);
  const char* const kBehaviorStatesToSuppressHeldInPalmReactionsKey = "suppressingBehaviors";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateWhileHeldInPalm::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
{
  std::vector<std::string> tmpNames;
  ANKI_VERIFY(JsonTools::GetVectorOptional(config, kBehaviorStatesToSuppressHeldInPalmReactionsKey, tmpNames),
              (debugName + ".MissingVectorOfSuppressingBehaviorNames").c_str(),"");
  for(const auto& name : tmpNames) {
    behaviorStatesToSuppressHeldInPalmReactions.insert( BehaviorTypesWrapper::BehaviorIDFromString(name) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateWhileHeldInPalm::BehaviorCoordinateWhileHeldInPalm(const Json::Value& config)
: BehaviorDispatcherPassThrough(config),
  _iConfig(config, GetDebugLabel() + ".Ctor")
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateWhileHeldInPalm::~BehaviorCoordinateWhileHeldInPalm()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileHeldInPalm::GetPassThroughJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kBehaviorStatesToSuppressHeldInPalmReactionsKey );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileHeldInPalm::InitPassThrough()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.heldInPalmDispatcher = BC.FindBehaviorByID(kHeldInPalmDispatcher);
  _iConfig.initialHeldInPalmReaction = BC.FindBehaviorByID(kInitialHeldInPalmReaction);
  {
    _iConfig.suppressHeldInPalmBehaviorSet =
        std::make_unique<AreBehaviorsActivatedHelper>(BC, _iConfig.behaviorStatesToSuppressHeldInPalmReactions);
  }
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileHeldInPalm::OnPassThroughActivated()
{
  const auto persistent = _dVars.persistent;
  _dVars = DynamicVariables();
  _dVars.persistent = persistent;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileHeldInPalm::PassThroughUpdate()
{
  if( !IsActivated() ) {
    return;
  }

  const bool isHeldInPalmReactionPlaying = _iConfig.heldInPalmDispatcher->IsActivated();
  _dVars.persistent.hasInitialHIPReactionPlayed |= _iConfig.initialHeldInPalmReaction->IsActivated();

  // Block the activation of all "held-in-palm" behaviors if a supressing behavior is already activated,
  // or block only the initial get-in animation reaction if the robot never left the palm of the user's
  // hand while it was being supressed.
  if(!isHeldInPalmReactionPlaying){
    if (GetBEI().GetHeldInPalmTracker().IsHeldInPalm()) {
      SuppressHeldInPalmReactionsIfAppropriate();
      SuppressInitialHeldInPalmReactionIfAppropriate();
    } else {
      _dVars.persistent.hasInitialHIPReactionPlayed = false;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileHeldInPalm::OnBehaviorLeftActivatableScope()
{
  _dVars = DynamicVariables();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileHeldInPalm::SuppressHeldInPalmReactionsIfAppropriate()
{
  const bool shouldSuppress = _iConfig.suppressHeldInPalmBehaviorSet->AreBehaviorsActivated();

  if(shouldSuppress){
    _iConfig.heldInPalmDispatcher->SetDontActivateThisTick(GetDebugLabel());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileHeldInPalm::SuppressInitialHeldInPalmReactionIfAppropriate()
{
  const u32 currHeldInPalmDuration_ms = GetBEI().GetHeldInPalmTracker().GetHeldInPalmDuration_ms();
  const bool shouldSuppressInitialReaction = currHeldInPalmDuration_ms > kMaxTimeForInitialHeldInPalmReaction_ms;

  // Only play the initial "get-in palm" animation if the robot has recently been put into a user's palm
  if( shouldSuppressInitialReaction || _dVars.persistent.hasInitialHIPReactionPlayed ){
    _iConfig.initialHeldInPalmReaction->SetDontActivateThisTick(GetDebugLabel());
    _dVars.persistent.hasInitialHIPReactionPlayed = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWhileHeldInPalm::OnFirstPassThroughUpdate()
{
  // Safety check - behaviors that are  in the list need to internally list that they can be activated while in the air
  // or response will not work as expected.
  const auto& BC = GetBEI().GetBehaviorContainer();
  for(const auto& id : _iConfig.behaviorStatesToSuppressHeldInPalmReactions){
    const auto behaviorPtr = BC.FindBehaviorByID(id);
    BehaviorOperationModifiers modifiers;
    behaviorPtr->GetBehaviorOperationModifiers(modifiers);
    ANKI_VERIFY(modifiers.wantsToBeActivatedWhenOffTreads,
                "BehaviorCoordinateWhileHeldInPalm.OnFirstUpdate.BehaviorCantRunInAir",
                "Behavior %s is listed as a behavior that suppresses the held-in-palm reactions, \
                but modifier says it can't run while in the air", BehaviorTypesWrapper::BehaviorIDToString(id));

  }
}

}
}
