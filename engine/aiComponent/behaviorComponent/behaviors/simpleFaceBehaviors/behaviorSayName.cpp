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


namespace Anki {
namespace Cozmo {
  
namespace{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSayName::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSayName::DynamicVariables::DynamicVariables()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSayName::BehaviorSayName(const Json::Value& config)
  : ISimpleFaceBehavior(config)
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSayName::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig.ttsBehavior.get() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSayName::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActiveScope->insert( {VisionMode::DetectingFaces, EVisionUpdateFrequency::High} );
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
    if( nameKnown ) {
      const std::string text = facePtr->GetName() + "!";
      auto* sayNameAction = new SayTextAction( text );
      sayNameAction->SetAnimationTrigger( AnimationTrigger::MeetVictorSayNameAgain );
      DelegateIfInControl( sayNameAction );
    } else {
      const std::string text = "eye dont know";
      _iConfig.ttsBehavior->SetTextToSay( text ); // set text in advance to begin generation
      auto* animAction = new TriggerLiftSafeAnimationAction( AnimationTrigger::MeetVictorSawWrongFace );
      DelegateIfInControl( animAction, [this](ActionResult result) {
        ANKI_VERIFY( _iConfig.ttsBehavior->WantsToBeActivated(), "BehaviorSayName.OnBehaviorActivated.NoTTS","");
        DelegateIfInControl(_iConfig.ttsBehavior.get() );
      });
    }
  
    
  }
}
  

}
}

