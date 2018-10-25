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
#include "coretech/common/engine/utils/timer.h"
#include "coretech/vision/engine/trackedFace.h"
#include "engine/actions/animActions.h"
#include "engine/actions/sayTextAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"
#include "engine/faceWorld.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"
#include "util/logging/DAS.h"

namespace Anki {
namespace Vector {
  
namespace JsonKeys {
  
static const char * const kDontKnowNameAnimation = "dontKnowNameAnimation";
static const char * const kKnowNameAnimation = "knowNameAnimation";
static const char * const kDontKnowText = "dontKnowText";
static const char * const kWaitForRecognitionMaxTime = "waitForRecognitionMaxTime_sec";
  
}
  
#define LOG_CHANNEL "FaceRecognizer"

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
  
  JsonTools::GetValueOptional(config, JsonKeys::kWaitForRecognitionMaxTime, waitForRecognitionMaxTime_sec);
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
    JsonKeys::kWaitForRecognitionMaxTime,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSayName::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActiveScope->insert( {VisionMode::DetectingFaces, EVisionUpdateFrequency::High} );
  
  // Assumption is that we're already looking at the face, so use cropping for better efficiency
  modifiers.visionModesForActiveScope->insert( {VisionMode::CroppedFaceDetection, EVisionUpdateFrequency::High} );
  
  // Avoid marker detection to improve performance
  // TODO: Remove with VIC-6838
  modifiers.visionModesForActiveScope->insert( { VisionMode::DisableMarkerDetection, EVisionUpdateFrequency::High } );

  // No longer true: can wait on recognition
  modifiers.behaviorAlwaysDelegates = false;
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
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSayName::BehaviorUpdate()
{
  if(!IsActivated() || IsControlDelegated())
  {
    return;
  }
  
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace( GetTargetFace() );
  if(facePtr == nullptr)
  {
    LOG_ERROR("BehaviorSayName.OnBehaviorActivated.InvalidFace", "TargetFaceID:%s",
              GetTargetFace().GetDebugStr().c_str());
    Finish();
    return;
  }
  
  if(_dVars.waitingForRecognition)
  {
    const bool nameKnown = facePtr->HasName();
    if( nameKnown )
    {
      LOG_INFO("BehaviorSayName.Update.NameKnown", "CurrentTick:%zu", BaseStationTimer::getInstance()->GetTickCount());
      SayName(facePtr->GetName());
    }
    else
    {
      _dVars.waitingForRecognition = (facePtr->GetID() < 0);
      
      if(_dVars.waitingForRecognition)
      {
        // Still waiting: see if we've timed out
        const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        if((currentTime - GetTimeActivated_s()) > _iConfig.waitForRecognitionMaxTime_sec)
        {
          DASMSG(behavior_sayname_recognition_timed_out, "behavior.sayname.recognition_timeout",
                 "Behavior timed out waiting for tracked face to complete recognition");
          DASMSG_SEND();
          SayDontKnow();
        }
        else
        {
          LOG_DEBUG("BehaviorSayName.Update.WaitingForRecognition", "Start:%.1f Current:%.1f",
                    GetTimeActivated_s(), currentTime);
        }
      }
      else
      {
        // Recognition complete, but we apparently don't know the name (otherwise nameKnown would be true above)
        SayDontKnow();
      }
    }
  }
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSayName::SayName(const std::string& name)
{
  DEV_ASSERT(!_dVars.waitingForRecognition, "BehaviorSayName.SayName.StillWaitingForRecognition");
  
  DASMSG(behavior_sayname_name_known, "behavior.sayname.name_known",
         "SayName behavior resulted in saying the name");
  DASMSG_SEND();
  
  // TODO: Use animation + TTS behavior instead of SayTextAction with animation containing special TTS keyframe?
  const std::string text = name + "!";
  auto* sayNameAction = new SayTextAction( text );
  sayNameAction->SetAnimationTrigger( _iConfig.knowNameAnimation );
  DelegateIfInControl( sayNameAction, &BehaviorSayName::Finish );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSayName::SayDontKnow()
{
  DEV_ASSERT(!_dVars.waitingForRecognition, "BehaviorSayName.SayName.StillWaitingForRecognition");
  
  auto* animAction = new TriggerLiftSafeAnimationAction( _iConfig.dontKnowNameAnimation );
  
  const bool haveDontKnowText = !_iConfig.dontKnowText.empty();
  if(haveDontKnowText)
  {
    // Set text in advance to begin generation
    _iConfig.ttsBehavior->SetTextToSay( _iConfig.dontKnowText );
  }
  
  // Play the animation and then say the text with TTS, if there is any
  DelegateIfInControl( animAction, [this]() {
    const bool haveDontKnowText = !_iConfig.dontKnowText.empty();
    if(_dVars.waitingForRecognition) {
      // Now that animation has played, eating a little more time, check one last time
      // to see if the face has a name yet. Say it if so.
      const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace( GetTargetFace() );
      if(facePtr->HasName()) {
        DASMSG(behavior_sayname_switch_to_do_know, "behavior.sayname.switch_to_do_know",
               "Name realized after initially playing 'don't know' animation");
        DASMSG_SEND();
        if(haveDontKnowText) {
          // Remember to clear the generated TTS
          _iConfig.ttsBehavior->ClearTextToSay();
        }
        SayName(facePtr->GetName());
        return;
      }
    }
    
    // TODO: add ability to tell FaceWorld to log DAS for unnamed->named ID update that occurs for this face later
    DASMSG(behavior_sayname_dont_know, "behavior.sayname.dont_know",
           "SayName behavior resulted in 'don't know' response");
    DASMSG_SEND();
    
    if(haveDontKnowText && ANKI_VERIFY( _iConfig.ttsBehavior->WantsToBeActivated(),
                                       "BehaviorSayName.OnBehaviorActivated.NoTTS",""))
    {
      DelegateIfInControl( _iConfig.ttsBehavior.get(), &BehaviorSayName::Finish );
    }
    else {
      Finish();
    }
  });
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSayName::Finish()
{
  _dVars.waitingForRecognition = false;
  CancelSelf();
}
  
}
}

