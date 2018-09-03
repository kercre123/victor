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
#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorDriveToFace.h"
#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorTimerUtilityCoordinator.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/photographyManager.h"

#include "util/helpers/boundedWhile.h"

#include "coretech/common/engine/utils/timer.h"

#include "clad/types/behaviorComponent/streamAndLightEffect.h"

#include <deque>

namespace Anki {
namespace Vector {

namespace{

  // add behavior _classes_ here if we should disable the prox-based "react to sudden obstacle" behavior while
  // _any_ behavior of that class is running below us on the stack
  static const std::set<BehaviorClass> kBehaviorClassesToSuppressProx = {{ BEHAVIOR_CLASS(BlackJack),
                                                                           BEHAVIOR_CLASS(FetchCube),
                                                                           BEHAVIOR_CLASS(FistBump),
                                                                           BEHAVIOR_CLASS(FindCubeAndThen),
                                                                           BEHAVIOR_CLASS(Keepaway),
                                                                           BEHAVIOR_CLASS(InspectCube),
                                                                           BEHAVIOR_CLASS(PickUpCube),
                                                                           BEHAVIOR_CLASS(PopAWheelie),
                                                                           BEHAVIOR_CLASS(PounceWithProx),
                                                                           BEHAVIOR_CLASS(RollBlock) }};
  
  static const std::set<BehaviorClass> kBehaviorClassesToSuppressReactToSound = {{ BEHAVIOR_CLASS(BlackJack),
                                                                                   BEHAVIOR_CLASS(FetchCube),
                                                                                   BEHAVIOR_CLASS(FistBump),
                                                                                   BEHAVIOR_CLASS(Keepaway),
                                                                                   BEHAVIOR_CLASS(InspectCube),
                                                                                   BEHAVIOR_CLASS(PickUpCube),
                                                                                   BEHAVIOR_CLASS(PopAWheelie),
                                                                                   BEHAVIOR_CLASS(PounceWithProx),
                                                                                   BEHAVIOR_CLASS(RollBlock) }};

  static const std::set<BehaviorClass> kBehaviorClassesToSuppressTouch = { BEHAVIOR_CLASS(BlackJack) };

  static const std::set<BehaviorClass> kBehaviorClassesToSuppressCliff = { BEHAVIOR_CLASS(BlackJack) };

  static const std::set<BehaviorClass> kBeahviorClassesToSuppressTimerAntics = {{ BEHAVIOR_CLASS(BlackJack),
                                                                                  BEHAVIOR_CLASS(CoordinateWeather) }};
  
  static const std::set<BehaviorID> kBehaviorIDsToSuppressWhenMeetVictor = {{
    BEHAVIOR_ID(ReactToTouchPetting),       // the user will often turn the robot to face them and in the process touch it
    BEHAVIOR_ID(ReactToUnexpectedMovement), // the user will often turn the robot to face them
    BEHAVIOR_ID(ReactToSoundAwake),         // fully concentrate on what's in front
    BEHAVIOR_ID(ReactToDarkness)            // user hand near camera may trigger darkened condition
  }};
  static const std::set<BehaviorID> kBehaviorIDsToSuppressWhenDancingToTheBeat = {
    BEHAVIOR_ID(ReactToSoundAwake),
  };
  static const std::set<BehaviorID> kBehaviorIDsToSuppressWhenGoingHome = {
    BEHAVIOR_ID(DanceToTheBeatCoordinator),
    BEHAVIOR_ID(ListenForBeats),
    BEHAVIOR_ID(DanceToTheBeat),
  };
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateGlobalInterrupts::DynamicVariables::DynamicVariables()
  : suppressProx(false)
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
  
  for( const auto& id : kBehaviorIDsToSuppressWhenMeetVictor ) {
    _iConfig.toSuppressWhenMeetVictor.push_back( BC.FindBehaviorByID(id) );
  }
  for( const auto& id : kBehaviorIDsToSuppressWhenDancingToTheBeat ) {
    _iConfig.toSuppressWhenDancingToTheBeat.push_back( BC.FindBehaviorByID(id) );
  }
  for( const auto& id : kBehaviorIDsToSuppressWhenGoingHome ) {
    _iConfig.toSuppressWhenGoingHome.push_back( BC.FindBehaviorByID(id) );
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
  
  _iConfig.behaviorsThatShouldntReactToUnexpectedMovement.AddBehavior(BC, BEHAVIOR_CLASS(BumpObject));
  _iConfig.reactToUnexpectedMovementBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ReactToUnexpectedMovement));

  _iConfig.reactToSoundAwakeBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ReactToSoundAwake));
  for(const auto& behaviorClass : kBehaviorClassesToSuppressReactToSound){
    _iConfig.behaviorsThatShouldntReactToSoundAwake.AddBehavior(BC, behaviorClass);
  }

  _iConfig.reactToTouchPettingBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ReactToTouchPetting));
  for(const auto& behaviorClass : kBehaviorClassesToSuppressTouch){
    _iConfig.behaviorsThatShouldntReactToTouch.AddBehavior(BC, behaviorClass);
  }

  for(const auto& behaviorClass : kBeahviorClassesToSuppressTimerAntics){
    _iConfig.behaviorsThatShouldSuppressTimerAntics.AddBehavior(BC, behaviorClass);
  }

  _iConfig.reactToCliffBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ReactToCliff));
  for(const auto& behaviorClass : kBehaviorClassesToSuppressCliff){
    _iConfig.behaviorsThatShouldntReactToCliff.AddBehavior(BC, behaviorClass);
  }

  std::set<ICozmoBehaviorPtr> driveToFaceBehaviors = BC.FindBehaviorsByClass(BEHAVIOR_CLASS(DriveToFace));
  _iConfig.driveToFaceBehaviors.reserve( driveToFaceBehaviors.size() );
  for( const auto& ptr : driveToFaceBehaviors ) {
    auto beh = std::dynamic_pointer_cast<BehaviorDriveToFace>(ptr);
    if( beh != nullptr ) {
      _iConfig.driveToFaceBehaviors.push_back( beh );
    }
  }
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

  bool shouldSuppressTriggerWordBehavior = false;

  if ( shouldSuppressTriggerWordBehavior )
  {
    _iConfig.wakeWordBehavior->SetDontActivateThisTick(GetDebugLabel());
  }

  // todo: generalize "if X is running then suppress Y"
  
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

  // Suppress timer antics if necessary
  if(_iConfig.behaviorsThatShouldSuppressTimerAntics.AreBehaviorsActivated() ) {
    const auto tickCount = BaseStationTimer::getInstance()->GetTickCount();
    _iConfig.timerCoordBehavior->SuppressAnticThisTick(tickCount);
  }

  // this will suppress the streaming POST-wakeword pending
  // the "do a fist bump" part of "hey victor"
  const bool shouldSuppressStreaming = shouldSuppressTriggerWordBehavior;
  if(shouldSuppressStreaming){
    SmartAlterStreamStateForCurrentResponse(StreamAndLightEffect::StreamingDisabledButWithLight);
  }else{
    SmartPopResponseToTriggerWord();
  }

  {
    auto& uic = GetBehaviorComp<UserIntentComponent>();
    
    // If we are responding to "take a photo", and the user is not requesting a selfie
    // Disable the react to voice command turn so that Victor takes the photo in his current direction
    // Exception: If storage is full we want to turn towards the user to let them know
    UserIntent photoIntent;
    const bool isPhotoPending = uic.IsUserIntentPending(USER_INTENT(take_a_photo), photoIntent);
    if(isPhotoPending){
      const auto& takeAPhoto = photoIntent.Get_take_a_photo();
      const bool isNotASelfie = takeAPhoto.empty_or_selfie.empty();
      const bool isStorageFull = GetBEI().GetPhotographyManager().IsPhotoStorageFull();
      if(isNotASelfie && !isStorageFull){
        const EngineTimeStamp_t ts = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
        _iConfig.reactToVoiceCommandBehavior->DisableTurnForTimestamp(ts);
      }
    }
    
    // If we are responding to "go home", disable the voice command turn since we want him to just go directly
    // into looking for the charger / going to the charger. Also disable dancing to the beat, need to get home
    // first.  // TODO:(bn) disable others too?
    const bool isGoHomePending = uic.IsUserIntentPending(USER_INTENT(system_charger));
    if (isGoHomePending) {
      const EngineTimeStamp_t ts = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      _iConfig.reactToVoiceCommandBehavior->DisableTurnForTimestamp(ts);
    }

    const bool isGoHomeActive = uic.IsUserIntentActive(USER_INTENT(system_charger));
    if( isGoHomeActive ) {
      for( const auto& beh : _iConfig.toSuppressWhenGoingHome ) {
        beh->SetDontActivateThisTick(GetDebugLabel() + ": going home");
      }
    }
  }
  
  // disable ReactToUnexpectedMovement when intentionally bumping things
  {
    if( _iConfig.behaviorsThatShouldntReactToUnexpectedMovement.AreBehaviorsActivated() ) {
      _iConfig.reactToUnexpectedMovementBehavior->SetDontActivateThisTick(GetDebugLabel());
    }
  }
  
  // Suppress ReactToSoundAwake if needed
  {
    if( _iConfig.behaviorsThatShouldntReactToSoundAwake.AreBehaviorsActivated() ) {
      _iConfig.reactToSoundAwakeBehavior->SetDontActivateThisTick(GetDebugLabel());
    }
  }

  // Suppress ReactToTouchPetting if needed
  {
    if( _iConfig.behaviorsThatShouldntReactToTouch.AreBehaviorsActivated() ) {
      _iConfig.reactToTouchPettingBehavior->SetDontActivateThisTick(GetDebugLabel());
    }
  }

  // Suppress ReactToCliff if needed
  {
    if( _iConfig.behaviorsThatShouldntReactToCliff.AreBehaviorsActivated() ) {
      _iConfig.reactToCliffBehavior->SetDontActivateThisTick(GetDebugLabel());
    }
  }
  
  // tell BehaviorDriveToFace whenever a cliff interruption behavior is active, so that it knows when
  // it is reasonable to resume-i-mean-wants-to-be-activated-sorry-kevin
  {
    if( _iConfig.reactToCliffBehavior->IsActivated() ) {
      for( const auto& driveToFaceBehavior : _iConfig.driveToFaceBehaviors ) {
        driveToFaceBehavior->SetInterruptionEndTick( BaseStationTimer::getInstance()->GetTickCount() );
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
        return false; // A behavior satisfied the condition, stop iterating
      }
      return true; // Haven't satisfied the condition yet, keep iterating
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
  


} // namespace Vector
} // namespace Anki
