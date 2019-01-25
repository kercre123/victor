/**
 * File: BehaviorBoxDemoOpenStream.cpp
 *
 * Author: Brad
 * Created: 2019-01-24
 *
 * Description: Open a stream to chipper for intents based on a touch event
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/box/behaviorBoxDemoOpenStream.h"

#include "clad/cloud/mic.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"

namespace Anki {
namespace Vector {
  
namespace {
constexpr const float kTimeout_s = 10.0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoOpenStream::BehaviorBoxDemoOpenStream(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoOpenStream::~BehaviorBoxDemoOpenStream()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBoxDemoOpenStream::WantsToBeActivatedBehavior() const
{
  // don't run on the left side to avoid fighting with pairing mode 
  const bool onLeft = GetBEI().GetOffTreadsState() == OffTreadsState::OnLeftSide;
  return !onLeft;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoOpenStream::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.behaviorAlwaysDelegates = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoOpenStream::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoOpenStream::OnBehaviorActivated() 
{

  GetBehaviorComp<UserIntentComponent>().StartWakeWordlessStreaming(CloudMic::StreamType::Normal, false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoOpenStream::BehaviorUpdate() 
{
  // TODO: monitor for things you care about here
  if( IsActivated() ) {

    if( GetActivatedDuration() > kTimeout_s ) {
      PRINT_NAMED_WARNING("BehaviorBoxDemoOpenStream.Timeout",
                          "behavior has been active %f seconds without intent, bailing",
                          GetActivatedDuration());
      CancelSelf();
      return;
    }

    UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();
    if( uic.IsAnyUserIntentPending() ){
      PRINT_CH_INFO("Behaviors", "BehaviorBoxDemoOpenStream.GotIntent", "%s: got intent, stopping",
                    GetDebugLabel().c_str());
      CancelSelf();
      return;
    }
  }
}

}
}
