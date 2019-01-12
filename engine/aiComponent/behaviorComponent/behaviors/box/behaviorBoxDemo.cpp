/**
 * File: BehaviorBoxDemo.cpp
 *
 * Author: Brad
 * Created: 2019-01-03
 *
 * Description: Main state machine for the box demo
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/box/behaviorBoxDemo.h"

#include "audioEngine/multiplexer/audioCladMessageHelper.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/beiConditions/conditions/conditionLambda.h"
#include "util/console/consoleInterface.h"
#include "util/helpers/ankiDefines.h"

namespace Anki {
namespace Vector {
  
namespace {
  // set to 0 to test trigger word more than once. 1 = transition after trigger word. 2 = transition immediately (needed for mac)
  CONSOLE_VAR_RANGED(unsigned int, kTransitionAfterTriggerWord, "TheBox.State", 1, 0, 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemo::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemo::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemo::BehaviorBoxDemo(const Json::Value& config)
 : InternalStatesBehavior( config, CreateCustomConditions() )
{
  AddConsoleVarTransitions( "TheBoxMoveToState", "TheBox.State" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemo::~BehaviorBoxDemo()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemo::OnBehaviorActivatedInternal()
{
  SmartDisableKeepFaceAlive();

  // NOTE: earcon is disabled at the anim level, but still required, so just use the normal one (it won't play
  // in the box)
  namespace AECH = AudioEngine::Multiplexer::CladMessageHelper;
  auto postAudioEvent = AECH::CreatePostAudioEvent(
    AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Sfx__Wake_Word_On,
    AudioMetaData::GameObjectType::Behavior, 0 );

  GetBehaviorComp<UserIntentComponent>().PushResponseToTriggerWord( GetDebugLabel(),
                                                                    AnimationTrigger::Count, // no animation
                                                                    postAudioEvent,
                                                                    StreamAndLightEffect::StreamingEnabled );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemo::OnBehaviorDeactivatedInternal()
{
  GetBehaviorComp<UserIntentComponent>().PopResponseToTriggerWord(GetDebugLabel());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CustomBEIConditionHandleList BehaviorBoxDemo::CreateCustomConditions()
{
  CustomBEIConditionHandleList handles;
  
  const std::string emptyOwnerLabel = ""; // when these conditions are claimed from the factory, a label is added

  handles.emplace_back(
    BEIConditionFactory::InjectCustomBEICondition(
      "TriggerPhaseComplete",
      std::make_shared<ConditionLambda>(
        [this](BehaviorExternalInterface& bei) {
          static bool printed __attribute((unused)) = false;
          #if defined(ANKI_PLATFORM_OSX)
          {
            if( !printed ) {
              PRINT_NAMED_WARNING("THEBOX", "Note that the wake word doesnt work in mac, so you'll either need to trigger"
                                  " it artificially, or set TheBox.State > TransitionAfterTriggerWord to 2" );
              printed = true;
            }
          }
          #endif
          const auto& uic = GetBehaviorComp<UserIntentComponent>();
          // this will likely need to be based on pending intents being claimed, but I'll leave that until we know what's
          // in the demo script
          return (uic.IsTriggerWordPending() && kTransitionAfterTriggerWord) || (kTransitionAfterTriggerWord == 2);
        },
        emptyOwnerLabel )));
  
  return handles;
}


}
}
