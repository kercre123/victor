/**
 * File: behaviorQuietModeCoordinator.cpp
 *
 * Author: ross
 * Created: 2018-05-14
 *
 * Description: Coordinator for quiet modes. No volume, no stimulation.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/coordinators/behaviorQuietModeCoordinator.h"
#include "engine/actions/dockActions.h"
#include "engine/activeObject.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "clad/types/animationTrigger.h"
#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cubes/cubeLightComponent.h"
#include "engine/moodSystem/moodManager.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

namespace {
  const char* const kActiveTimeKey        = "activeTime_s";
  const char* const kBehaviorsKey         = "behaviors";
  const char* const kBehaviorKey          = "behavior";
  const char* const kAudioAllowedKey      = "audioAllowed";

  const float kAccelMagnitudeShakingStartedThreshold = 16000.f;
}
  
CONSOLE_VAR_EXTERN(float, kTimeMultiplier);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorQuietModeCoordinator::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorQuietModeCoordinator::DynamicVariables::DynamicVariables()
{
  audioActive = true;
  wasFixed = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorQuietModeCoordinator::BehaviorQuietModeCoordinator(const Json::Value& config)
 : ICozmoBehavior(config)
{
  _iConfig.activeTime_s = JsonTools::ParseFloat(config, kActiveTimeKey, GetDebugLabel());
  
  const auto& behaviors = config[kBehaviorsKey];
  if( behaviors.isArray() ) {
    for( const auto& entry : behaviors ) {
      const bool allowsAudio = JsonTools::ParseBool(entry, kAudioAllowedKey, GetDebugLabel());
      const std::string behaviorIDStr = JsonTools::ParseString(entry, kBehaviorKey, GetDebugLabel());
      BehaviorID behaviorID;
      const bool behaviorMatches = BehaviorTypesWrapper::BehaviorIDFromString( behaviorIDStr, behaviorID );
      if( ANKI_VERIFY( behaviorMatches,
                       "BehaviorQuietModeCoordinator.Ctor.InvalidBehavior",
                       "Behavior %s invalid", behaviorIDStr.c_str() ) )
      {
        _iConfig.behaviors.push_back( {behaviorID, nullptr, allowsAudio} );
      }
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorQuietModeCoordinator::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.wakeWordBehavior = BC.FindBehaviorByID( BEHAVIOR_ID(ReactToTriggerDirectionAwake) );
  ANKI_VERIFY( _iConfig.wakeWordBehavior != nullptr,
               "BehaviorQuietModeCoordinator.InitBehavior.InvalidBehavior",
               "Wake word behavior not found" );
  
  for( auto& entry : _iConfig.behaviors ) {
    entry.behavior = BC.FindBehaviorByID( entry.behaviorID );
    ANKI_VERIFY( entry.behavior != nullptr,
                 "BehaviorQuietModeCoordinator.InitBehavior.InvalidBehavior",
                 "Behavior ID %s not found",
                 BehaviorTypesWrapper::BehaviorIDToString(entry.behaviorID) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorQuietModeCoordinator::WantsToBeActivatedBehavior() const
{
  // dont activate if the trigger word behavior is active, so that it can exit before we respond to the user intent
  return !_iConfig.wakeWordBehavior->IsActivated();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorQuietModeCoordinator::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorQuietModeCoordinator::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  for( const auto& entry : _iConfig.behaviors ) {
    delegates.insert( entry.behavior.get() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorQuietModeCoordinator::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kBehaviorsKey,
    kBehaviorKey,
    kAudioAllowedKey,
    kActiveTimeKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorQuietModeCoordinator::OnBehaviorActivated()
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  bool carrying = GetBEI().GetRobotInfo().GetCarryingComponent().IsCarryingObject();
  if( carrying ) {
    auto* action = new PlaceObjectOnGroundAction();
    DelegateIfInControl( action, &BehaviorQuietModeCoordinator::SimmerDownNow );
  } else {
    SimmerDownNow();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorQuietModeCoordinator::OnBehaviorDeactivated()
{
  ResumeNormal();
  
  GetAIComp<AIWhiteboard>().OfferPostBehaviorSuggestion( PostBehaviorSuggestions::Nothing );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorQuietModeCoordinator::BehaviorUpdate()
{
  if( !IsActivated() ) {
    return;
  }
  
  // warn if stimulation is creeping up somehow (shouldn't happen)
  auto& moodManager = GetBEI().GetMoodManager();
  const float stimVal = moodManager.GetEmotionValue(EmotionType::Stimulated);
  if( !Anki::Util::IsNearZero( stimVal ) ) {
    PRINT_NAMED_WARNING("BehaviorQuietModeCoordinator.Update.NonZeroStim",
                        "Stimulation is at %f", stimVal);
  }
  
  // is there a user intent pending other than unmatched? if so quit
  const auto& uic = GetBehaviorComp<UserIntentComponent>();
  if( uic.IsAnyUserIntentPending() && !uic.IsUserIntentPending( USER_INTENT(unmatched_intent) ) ) {
    // note that this means the trigger word was used (to break us out of the quiet behavior)
    // but that the stimulation was still fixed at 0. we might consider issuing a fake "ReactToTriggerWord"
    // emotion event here to compensate
    CancelSelf();
    return;
  }
  
  // exit quiet mode once enough time has elapsed
  const float timeActivated_s = GetActivatedDuration();
  if( timeActivated_s * kTimeMultiplier >= _iConfig.activeTime_s ) {
    CancelSelf();
    return;
  }
  
  // exit quiet mode if the robot has been shaken
  if( GetBEI().GetRobotInfo().GetHeadAccelMagnitudeFiltered() > kAccelMagnitudeShakingStartedThreshold ) {
    CancelSelf();
    return;
  }
  
  // if we're here, quiet mode is stll active. go through the behavior list like a DispatcherStrictPriority would
  for( const auto& entry : _iConfig.behaviors ) {
    if( !entry.behavior->IsActivated() && entry.behavior->WantsToBeActivated() ) {
      SetAudioActive( entry.allowsAudio );
      DelegateNow( entry.behavior.get() );
      break;
    } else if( entry.behavior->IsActivated() ) {
      break;
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorQuietModeCoordinator::SimmerDownNow()
{
  SetAudioActive( false );
  
  GetBEI().GetCubeLightComponent().StopAllAnims();
  BlockWorldFilter filter;
  filter.AddAllowedFamily(ObjectFamily::LightCube);
  std::vector<const ActiveObject*> connectedCubes;
  GetBEI().GetBlockWorld().FindConnectedActiveMatchingObjects(filter, connectedCubes);
  for( const auto* obj : connectedCubes ) {
    GetBEI().GetCubeLightComponent().PlayLightAnim( obj->GetID(), CubeAnimationTrigger::SleepNoFade );
  }
  
  auto& moodManager = GetBEI().GetMoodManager();
  _dVars.wasFixed = moodManager.IsEmotionFixed( EmotionType::Stimulated );
  moodManager.SetEmotionFixed( EmotionType::Stimulated, true );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorQuietModeCoordinator::ResumeNormal()
{
  SetAudioActive( true );
  
  auto& moodManager = GetBEI().GetMoodManager();
  moodManager.SetEmotionFixed( EmotionType::Stimulated, _dVars.wasFixed );
  
  GetBEI().GetCubeLightComponent().StopAllAnims();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorQuietModeCoordinator::SetAudioActive( bool active )
{
  if( _dVars.audioActive != active ) {
    using GE = AudioMetaData::GameEvent::GenericEvent;
    using GO = AudioMetaData::GameObjectType;
    const auto event = active ? GE::Play__Robot_Vic_Scene__Quiet_Off : GE::Play__Robot_Vic_Scene__Quiet_On;
    GetBEI().GetRobotAudioClient().PostEvent( event, GO::Behavior );

    _dVars.audioActive = active;
  }
}

}
}
