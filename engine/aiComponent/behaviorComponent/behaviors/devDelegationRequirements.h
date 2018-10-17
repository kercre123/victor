/**
 * File: devDelegationRequirements.h
 *
 * Author: ross
 * Created: 2018 September 19
 *
 * Description: Allows behaviors to perform actions prior to being force activated for dev purposes
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_DevDelegationRequirements_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_DevDelegationRequirements_H__

#include "clad/types/behaviorComponent/behaviorClasses.h"
#include "clad/types/behaviorComponent/userIntent.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/meetCozmo/behaviorRespondToRenameFace.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToMotion.h"
#include "engine/aiComponent/behaviorComponent/behaviors/robotDrivenDialog/behaviorPromptUserForVoiceCommand.h"
#include "engine/components/settingsManager.h"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<BehaviorClass Class>
void ApplyForceDelegationRequirements( ICozmoBehavior* behavior, BehaviorExternalInterface& bei ){}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Put your DEV delegation requirements here. Feel free to make a separate file that is included by this one.
// If this should only be done in unit tests, use testDelegationRequirements.h

template <>
void ApplyForceDelegationRequirements<BehaviorClass::PromptUserForVoiceCommand>( ICozmoBehavior* behavior,
                                                                                 BehaviorExternalInterface& bei )
{
  BehaviorPromptUserForVoiceCommand* castPtr = static_cast<BehaviorPromptUserForVoiceCommand*>(behavior);
  castPtr->SetPrompt( "hello" );
}
 
template <>
void ApplyForceDelegationRequirements<BehaviorClass::EyeColor>( ICozmoBehavior* behavior,
                                                                BehaviorExternalInterface& bei )
{
  auto& sm = bei.GetSettingsManager();
  external_interface::RobotSetting setting = external_interface::eye_color;
  bool b = false;
  sm.SetRobotSetting(setting, Json::Value{5}, false, b);
}
  
template <>
void ApplyForceDelegationRequirements<BehaviorClass::EnrollFace>( ICozmoBehavior* behavior,
                                                                  BehaviorExternalInterface& bei )
{
  auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
  UserIntent_MeetVictor meetVictor{"Cozmo"};
  UserIntent intent = UserIntent::Createmeet_victor(std::move(meetVictor));
  uic.DevSetUserIntentPending( std::move(intent) );
}
  
template <>
void ApplyForceDelegationRequirements<BehaviorClass::RespondToRenameFace>( ICozmoBehavior* behavior,
                                                                           BehaviorExternalInterface& bei )
{
  BehaviorRespondToRenameFace* castPtr = static_cast<BehaviorRespondToRenameFace*>(behavior);
  castPtr->SetName( "Bobert" );
}
  
template <>
void ApplyForceDelegationRequirements<BehaviorClass::LeaveAMessage>( ICozmoBehavior* behavior,
                                                                     BehaviorExternalInterface& bei )
{
  auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();

  UserIntent_RecordMessage intentData{"Bentley"};
  UserIntent intent = UserIntent::Createmessage_record(std::move(intentData));
  uic.DevSetUserIntentPending( std::move(intent) );
}
  
template <>
void ApplyForceDelegationRequirements<BehaviorClass::PlaybackMessage>( ICozmoBehavior* behavior,
                                                                       BehaviorExternalInterface& bei )
{
  auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();

  UserIntent_PlaybackMessage intentData{"Bentley"};
  UserIntent intent = UserIntent::Createmessage_playback(std::move(intentData));
  uic.DevSetUserIntentPending( std::move(intent) );
}
  
template <>
void ApplyForceDelegationRequirements<BehaviorClass::TakeAPhotoCoordinator>( ICozmoBehavior* behavior,
                                                                             BehaviorExternalInterface& bei )
{
  auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
  UserIntent intent = UserIntent::Createtake_a_photo({});
  uic.DevSetUserIntentPending( std::move(intent) );
}
  
template <>
void ApplyForceDelegationRequirements<BehaviorClass::CoordinateWeather>( ICozmoBehavior* behavior,
                                                                         BehaviorExternalInterface& bei )
{
  auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
  UserIntent_WeatherResponse weatherResponse;
  weatherResponse.condition = "scattered snow showers";
  UserIntent intent = UserIntent::Createweather_response(std::move(weatherResponse));
  uic.DevSetUserIntentPending( std::move(intent) );
}
  
template <>
void ApplyForceDelegationRequirements<BehaviorClass::DisplayWeather>( ICozmoBehavior* behavior,
                                                                      BehaviorExternalInterface& bei )
{
  auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
  UserIntent_WeatherResponse weatherResponse;
  weatherResponse.condition = "scattered snow showers";
  UserIntent intent = UserIntent::Createweather_response(std::move(weatherResponse));
  uic.DevSetUserIntentPending( std::move(intent) );
}

template <>
void ApplyForceDelegationRequirements<BehaviorClass::ReactToMotion>( ICozmoBehavior* behavior,
                                                                     BehaviorExternalInterface& bei )
{
  BehaviorReactToMotion* castPtr = static_cast<BehaviorReactToMotion*>(behavior);
  castPtr->DevAddFakeMotion();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ApplyForceDelegationRequirements( ICozmoBehavior* behavior, BehaviorExternalInterface& bei )
{
  #define ADD_CASE_FOR_BEHAVIOR( x ) case BehaviorClass::x: { ApplyForceDelegationRequirements<BehaviorClass::x>(behavior, bei); } break
  switch( behavior->GetClass() ) {

      ADD_CASE_FOR_BEHAVIOR(CoordinateWeather);
      ADD_CASE_FOR_BEHAVIOR(DisplayWeather);
      ADD_CASE_FOR_BEHAVIOR(EnrollFace);
      ADD_CASE_FOR_BEHAVIOR(EyeColor);
      ADD_CASE_FOR_BEHAVIOR(LeaveAMessage);
      ADD_CASE_FOR_BEHAVIOR(PlaybackMessage);
      ADD_CASE_FOR_BEHAVIOR(PromptUserForVoiceCommand);
      ADD_CASE_FOR_BEHAVIOR(ReactToMotion);
      ADD_CASE_FOR_BEHAVIOR(RespondToRenameFace);
      ADD_CASE_FOR_BEHAVIOR(TakeAPhotoCoordinator);
      
      
    default:
      break;
  }
  #undef ADD_CASE_FOR_BEHAVIOR
}
  
  
}
}

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_DevDelegationRequirements_H__
