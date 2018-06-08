/**
* File: behaviorCoordinateGlobalInterrupts.cpp
*
* Author: Kevin M. Karol
* Created: 2/22/18
*
* Description: Behavior responsible for handling special case needs 
* that require coordination across behavior global interrupts
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/behaviorComponent/behaviors/coordinators/behaviorCoordinateGlobalInterrupts.h"

#include "engine/aiComponent/behaviorComponent/activeBehaviorIterator.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorAnimGetInLoop.h"
#include "engine/aiComponent/behaviorComponent/behaviors/behaviorHighLevelAI.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToVoiceCommand.h"
#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorTimerUtilityCoordinator.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/photographyManager.h"

#include "util/helpers/boundedWhile.h"

#include "coretech/common/engine/utils/timer.h"

#include <deque>

namespace Anki {
namespace Cozmo {

namespace{

  // add behavior _classes_ here if we should disable the prox-based "react to sudden obstacle" behavior while
  // _any_ behavior of that class is running below us on the stack
  static const std::set<BehaviorClass> kBehaviorClassesToSuppressProx = {{ BEHAVIOR_CLASS(FistBump),
                                                                           BEHAVIOR_CLASS(Keepaway),
                                                                           BEHAVIOR_CLASS(RollBlock),
                                                                           BEHAVIOR_CLASS(PounceWithProx) }};
  
  static const std::set<BehaviorID> kBehaviorIDsToSuppressWhenSleeping = {{
    BEHAVIOR_ID(ReactToTouchPetting),
    BEHAVIOR_ID(TriggerWordDetected),
  }};
  static const std::set<BehaviorID> kBehaviorIDsThatMeanSleeping = {{
    BEHAVIOR_ID(Sleeping),
    BEHAVIOR_ID(SleepingWakeUp),
  }};
  
  static const std::set<BehaviorID> kBehaviorIDsToSuppressWhenMeetVictor = {{
    BEHAVIOR_ID(ReactToTouchPetting),       // the user will often turn the robot to face them and in the process touch it
    BEHAVIOR_ID(ReactToUnexpectedMovement), // the user will often turn the robot to face them
    BEHAVIOR_ID(ReactToSoundAwake),         // fully concentrate on what's in front
  }};
  static const std::set<BehaviorID> kBehaviorIDsToSuppressWhenDancingToTheBeat = {
    BEHAVIOR_ID(ReactToSoundAwake),
  };
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::DynamicVariables::DynamicVariables()
  : suppressProx(false)
  , isSuppressingStreaming(false)
{
}


///////////
/// BehaviorCoordinateGlobalInterrupts
///////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::BehaviorCoordinateGlobalInterrupts(const Json::Value& config)
: BehaviorDispatcherPassThrough(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::~BehaviorCoordinateGlobalInterrupts()
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::InitPassThrough()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.wakeWordBehavior         = BC.FindBehaviorByID(BEHAVIOR_ID(TriggerWordDetected));
  
  for( const auto& id : kBehaviorIDsToSuppressWhenSleeping ) {
    _iConfig.toSuppressWhenSleeping.push_back( BC.FindBehaviorByID(id) );
  }
  for( const auto& id : kBehaviorIDsToSuppressWhenMeetVictor ) {
    _iConfig.toSuppressWhenMeetVictor.push_back( BC.FindBehaviorByID(id) );
  }
  for( const auto& id : kBehaviorIDsToSuppressWhenDancingToTheBeat ) {
    _iConfig.toSuppressWhenDancingToTheBeat.push_back( BC.FindBehaviorByID(id) );
  }

  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(TimerUtilityCoordinator),
                                 BEHAVIOR_CLASS(TimerUtilityCoordinator),
                                 _iConfig.timerCoordBehavior);
  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(TriggerWordDetected),
                                 BEHAVIOR_CLASS(ReactToVoiceCommand),
                                 _iConfig.reactToVoiceCommandBehavior);
  
  _iConfig.triggerWordPendingCond = BEIConditionFactory::CreateBEICondition(BEIConditionType::TriggerWordPending, GetDebugLabel());
  _iConfig.triggerWordPendingCond->Init(GetBEI());
  
  _iConfig.reactToObstacleBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ReactToObstacle));
  _iConfig.meetVictorBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(MeetVictor));
  _iConfig.danceToTheBeatBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(DanceToTheBeat));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::OnPassThroughActivated() 
{
  _iConfig.triggerWordPendingCond->SetActive(GetBEI(), true);
  
  if( ANKI_DEV_CHEATS ) {
    CreateConsoleVars();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::PassThroughUpdate()
{
  if(!IsActivated()){
    return;
  }
  
  const bool triggerWordPending = _iConfig.triggerWordPendingCond->AreConditionsMet(GetBEI());
  const bool isTimerRinging     = _iConfig.timerCoordBehavior->IsTimerRinging();
  if(triggerWordPending && isTimerRinging){
    // Timer is ringing and will handle the pending trigger word instead of the wake word behavior
    _iConfig.wakeWordBehavior->SetDontActivateThisTick(GetDebugLabel());
  }
  
  bool shouldSuppressStreaming = isTimerRinging;
  
  // suppress certain behaviors during sleeping
  {
    bool highLevelRunning = false;
    auto callback = [this, &highLevelRunning, &shouldSuppressStreaming](const ICozmoBehavior& behavior) {
      if( behavior.GetID() == BEHAVIOR_ID(HighLevelAI) ) {
        highLevelRunning = true;
      }

      if( highLevelRunning
          && (std::find(kBehaviorIDsThatMeanSleeping.begin(), kBehaviorIDsThatMeanSleeping.end(), behavior.GetID())
              != kBehaviorIDsThatMeanSleeping.end()) )
      {
        // High level AI is running the Sleeping behavior (probably through the Napping state).
        // Wake word serves as the wakeup for a napping robot, so disable the wake word behavior and let
        // high level AI resume. It will clear the pending trigger and resume in some other state. (The
        // wake up animation is the getout for napping). Also petting behaviors,
        // since those will cause a graceful getout
        for( const auto& beh : _iConfig.toSuppressWhenSleeping ) {
          beh->SetDontActivateThisTick(GetDebugLabel());
        }
        shouldSuppressStreaming = true;
      }
    };

    const auto& behaviorIterator = GetBehaviorComp<ActiveBehaviorIterator>();
    behaviorIterator.IterateActiveCozmoBehaviorsForward( callback, this );
  }
  
  // suppress during meet victor
  {
    if( _iConfig.meetVictorBehavior->IsActivated() ) {
      for( const auto& beh : _iConfig.toSuppressWhenMeetVictor ) {
        beh->SetDontActivateThisTick(GetDebugLabel());
      }
    }
  }
  
  // Suppress behaviors if dancing to the beat
  if( _iConfig.danceToTheBeatBehavior->IsActivated() ) {
    for( const auto& beh : _iConfig.toSuppressWhenDancingToTheBeat ) {
      beh->SetDontActivateThisTick(GetDebugLabel());
    }
  }
  
  // Suppress ReactToObstacle if needed
  if( ShouldSuppressProxReaction() ) {
    _iConfig.reactToObstacleBehavior->SetDontActivateThisTick(GetDebugLabel());
  }
  
  // Suppress behaviors disabled via console vars
  if( ANKI_DEV_CHEATS ) {
    for( const auto& behPair : _iConfig.devActivatableOverrides ) {
      if( !behPair.second && (behPair.first != nullptr) ) {
        behPair.first->SetDontActivateThisTick( "CV:" + GetDebugLabel() );
      }
    }
  }
  
  if( shouldSuppressStreaming != _dVars.isSuppressingStreaming ) {
    GetBEI().GetMicComponent().SetShouldStreamAfterWakeWord( !shouldSuppressStreaming );
    _dVars.isSuppressingStreaming = shouldSuppressStreaming;
  }

  // If we are responding to "take a photo", and the user is not requesting a selfie
  // Disable the react to voice command turn so that Victor takes the photo in his current direction
  // Exception: If storage is full we want to turn towards the user to let them know
  {
    auto& uic = GetBehaviorComp<UserIntentComponent>();
    UserIntent intent;
    const bool isPhotoPending = uic.IsUserIntentPending(USER_INTENT(take_a_photo), intent);
    if(isPhotoPending){
      const auto& takeAPhoto = intent.Get_take_a_photo();
      const bool isNotASelfie = takeAPhoto.empty_or_selfie.empty();
      const bool isStorageFull = GetBEI().GetPhotographyManager().IsPhotoStorageFull();
      if(isNotASelfie && !isStorageFull){
        const auto ts = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
        _iConfig.reactToVoiceCommandBehavior->DisableTurnForTimestamp(ts);
      }
    }
  }
}


bool BehaviorCoordinateGlobalInterrupts::ShouldSuppressProxReaction()
{
  // scan through the stack below this behavior and return true if any behavior is active which is listed in
  // kBehaviorClassesToSuppressProx
  
  const auto& behaviorIterator = GetBehaviorComp<ActiveBehaviorIterator>();

  // If the behavior stack has changed this tick or last tick, then update, otherwise use the last value
  const size_t currTick = BaseStationTimer::getInstance()->GetTickCount();
  if( behaviorIterator.GetLastTickBehaviorStackChanged() + 1 >= currTick ) {
    _dVars.suppressProx = false;

    auto callback = [this](const ICozmoBehavior& behavior) {
      if( kBehaviorClassesToSuppressProx.find( behavior.GetClass() ) != kBehaviorClassesToSuppressProx.end() ) {
        _dVars.suppressProx = true;
      }
    };
    
    behaviorIterator.IterateActiveCozmoBehaviorsForward( callback, this );
  }

  return _dVars.suppressProx;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::OnPassThroughDeactivated()
{
  _iConfig.triggerWordPendingCond->SetActive(GetBEI(), false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateGlobalInterrupts::CreateConsoleVars()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  std::set<IBehavior*> passThroughList;
  GetLinkedActivatableScopeBehaviors( passThroughList );
  if( !passThroughList.empty() ) {
    std::set<IBehavior*> globalInterruptions;
    (*passThroughList.begin())->GetAllDelegates( globalInterruptions );
    for( const auto* delegate : globalInterruptions ) {
      const auto* cozmoDelegate = dynamic_cast<const ICozmoBehavior*>( delegate );
      if( cozmoDelegate != nullptr ) {
        BehaviorID id = cozmoDelegate->GetID();
        auto pairIt = _iConfig.devActivatableOverrides.emplace( BC.FindBehaviorByID(id), true );
        // deque can contain non-copyable objects. its kept here to keep the header cleaner
        static std::deque<Anki::Util::ConsoleVar<bool>> vars;
        vars.emplace_back( pairIt.first->second,
                           BehaviorTypesWrapper::BehaviorIDToString( id ),
                           "BehaviorCoordinateGlobalInterrupts",
                           true );
      }
    }
  }
}
  


} // namespace Cozmo
} // namespace Anki
