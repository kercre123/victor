/**
 * File: BehaviorSimpleVoiceResponse.cpp
 *
 * Author: Brad
 * Created: 2019-03-29
 *
 * Description: A behavior that can perform a simple response to a number of different voice commands based on
 * the simple response mapping in user_intents.json
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/behaviorSimpleVoiceResponse.h"

#include "clad/types/behaviorComponent/userIntent.h"
#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/moodSystem/moodManager.h"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSimpleVoiceResponse::BehaviorSimpleVoiceResponse(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSimpleVoiceResponse::~BehaviorSimpleVoiceResponse()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSimpleVoiceResponse::WantsToBeActivatedBehavior() const
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  const bool pendingIntent = uic.IsUserIntentPending(USER_INTENT(simple_voice_response));
  return pendingIntent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSimpleVoiceResponse::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSimpleVoiceResponse::OnBehaviorActivated()
{
  const UserIntentPtr intentData = SmartActivateUserIntent(USER_INTENT(simple_voice_response));
  const MetaUserIntent_SimpleVoiceResponse& response = intentData->intent.Get_simple_voice_response();

  if( !response.emotion_event.empty() ) {
    GetBEI().GetMoodManager().TriggerEmotionEvent(response.emotion_event);
  }

  if( response.active_feature != ActiveFeature::NoFeature ) {
    SmartSetActiveFeatureOnActivated( response.active_feature );
  }

  DelegateIfInControl(new PlayAnimationGroupAction(response.anim_group));
}

}
}
