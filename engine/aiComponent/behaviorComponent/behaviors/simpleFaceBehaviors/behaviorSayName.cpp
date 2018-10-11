/**
 * File: behaviorSayName.cpp
 *
 * Author: ross
 * Created: 2018-06-22
 *
 * Description: says the user's name
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorSayName.h"
#include "coretech/vision/engine/trackedFace.h"
#include "engine/actions/animActions.h"
#include "engine/actions/sayTextAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"
#include "engine/faceWorld.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"

namespace Anki {
namespace Vector {
  
namespace JsonKeys {
  
static const char * const kDontKnowNameAnimation = "dontKnowNameAnimation";
static const char * const kKnowNameAnimation = "knowNameAnimation";
static const char * const kDontKnowText = "dontKnowText";
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSayName::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  // default animations
  knowNameAnimation = AnimationTrigger::MeetVictorSayNameAgain;
  dontKnowNameAnimation = AnimationTrigger::MeetVictorSawWrongFace;
  
  JsonTools::GetCladEnumFromJSON(config, JsonKeys::kDontKnowNameAnimation, dontKnowNameAnimation,
                                 "BehaviorSayName.InstanceConfig");
  
  JsonTools::GetCladEnumFromJSON(config, JsonKeys::kKnowNameAnimation, knowNameAnimation,
                                 "BehaviorSayName.InstanceConfig");
  
  JsonTools::GetValueOptional(config, JsonKeys::kDontKnowText, dontKnowText);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSayName::DynamicVariables::DynamicVariables()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSayName::BehaviorSayName(const Json::Value& config)
  : ISimpleFaceBehavior(config)
  , _iConfig(config)
{
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSayName::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  if( !_iConfig.dontKnowText.empty() )
  {
    delegates.insert( _iConfig.ttsBehavior.get() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSayName::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    JsonKeys::kDontKnowNameAnimation,
    JsonKeys::kKnowNameAnimation,
    JsonKeys::kDontKnowText,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSayName::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActiveScope->insert( {VisionMode::DetectingFaces, EVisionUpdateFrequency::High} );
  
  // Avoid marker detection to improve performance
  // TODO: Remove with VIC-6838
  modifiers.visionModesForActiveScope->insert( { VisionMode::DisableMarkerDetection, EVisionUpdateFrequency::High } );
  
  modifiers.behaviorAlwaysDelegates = true;
}
  
void BehaviorSayName::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(DefaultTextToSpeechLoop),
                                  BEHAVIOR_CLASS(TextToSpeechLoop),
                                  _iConfig.ttsBehavior );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSayName::WantsToBeActivatedBehavior() const
{
  return GetTargetFace().IsValid();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSayName::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace( GetTargetFace() );
  if(facePtr != nullptr){
    
    const bool nameKnown = facePtr->HasName();
    if( nameKnown )
    {
      // TODO: Use animation + TTS behavior instead of SayTextAction with animation containing special TTS keyframe?
      const std::string text = facePtr->GetName() + "!";
      auto* sayNameAction = new SayTextAction( text );
      sayNameAction->SetAnimationTrigger( _iConfig.knowNameAnimation );
      DelegateIfInControl( sayNameAction );
    }
    else
    {
      auto* animAction = new TriggerLiftSafeAnimationAction( _iConfig.dontKnowNameAnimation );
      
      const bool haveDontKnowText = !_iConfig.dontKnowText.empty();
      if(haveDontKnowText)
      {
        // Play the animation and then say the text with TTS
        _iConfig.ttsBehavior->SetTextToSay( _iConfig.dontKnowText ); // set text in advance to begin generation
        
        DelegateIfInControl( animAction, [this]() {
          if(ANKI_VERIFY( _iConfig.ttsBehavior->WantsToBeActivated(),
                         "BehaviorSayName.OnBehaviorActivated.NoTTS",""))
          {
            DelegateIfInControl( _iConfig.ttsBehavior.get() );
          }
        });
      }
      else
      {
        // Just play the animation
        DelegateIfInControl( animAction );
      }
    }
  }
}

}
}

