/**
 * File: socialPresenceEstimator.cpp
 *
 * Author: Andrew Stout
 * Created: 2019-04-02
 *
 * Description: Estimates whether someone receptive to social engagement is available.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#include "socialPresenceEstimator.h"

#include "clad/externalInterface/messageGameToEngine.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/ankiEventUtil.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "util/console/consoleInterface.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/logging/logging.h"
#include "webServerProcess/src/webService.h"

namespace Anki {
namespace Vector {

#define LOG_CHANNEL "RSPE"

namespace {
  float kEpsilon = 0.01; // threshold for calling a SocialPresenceEvent's value zero
}


float ExponentialDecay::operator()(float value, float dt_s)
{
  float ret = value * pow((1.0 - _ratePerSec), dt_s);
  // should zero out when we get close to zero
  if (fabs(ret) < kEpsilon) {
    ret = 0.0f;
  }
  return ret;
}

float PowerDecay::operator()(float value, float dt_s)
{
  if (fabs(value) < kEpsilon) {
    return 0.0f;
  }
  if (value >= 1.0) {
    value = 0.99;
  }
  if (value <= -1.0) {
    value = -0.99;
  }
  float sign = value >= 0.0 ? 1.0 : -1.0;
  float power = 1.0 + (dt_s*(_power - 1.0));
  float mag = pow(fabs(value), power);
  float ret = sign * mag;
  return ret;
}




SocialPresenceEvent::SocialPresenceEvent(std::string name,
    std::shared_ptr<IDecayFunction> decayFunction,
    float independentEffect,
    float independentEffectMax,
    float reinforcementEffect,
    float reinforcementEffectMax,
    bool resetPriorOnTrigger)
: _value(0.0),
  _name(name),
  _independentEffect(independentEffect),
  _independentEffectMax(independentEffectMax),
  _reinforcementEffect(reinforcementEffect),
  _reinforcementEffectMax(reinforcementEffectMax),
  _resetPriorOnTrigger(resetPriorOnTrigger)
{
  _decay = decayFunction;
}

SocialPresenceEvent::~SocialPresenceEvent()
{

}


void SocialPresenceEvent::Update(float dt_s)
{
  //LOG_WARNING("SocialPresenceEvent.Update.AboutToCallDecay", "");
  _value = (*_decay)(_value, dt_s);
  //LOG_WARNING("SocialPresenceEvent.Update.SuccessfullyCalledDecay", "");
}

void SocialPresenceEvent::Trigger(float& rspi) {
  if (rspi > _independentEffectMax) {
    // reinforcement
    _value = _reinforcementEffect;
  } else /*if (rspi >= 0.0)*/ {
    // independent effect
    _value = _independentEffect;
  }
  // if rspi < 0, trigger is inhibited
}




namespace{
  const std::string kWebVizModuleName = "socialpresence";
  const float kMinRSPIUpdatePeriod_s = 0.1;

  CONSOLE_VAR(float, kRSPE_WebVizPeriod_s, "SocialPresenceEstimator", 0.5f);
}

SocialPresenceEstimator::SocialPresenceEstimator()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::SocialPresenceEstimator),
  _rspi(0.0f)
{

}


SocialPresenceEstimator::~SocialPresenceEstimator()
{
  // huh. We don't have access to the UserIntentComponent here, so we can't unsubscribe.
  // TODO: Need a smart handle or something, which I'm sure exists somewhere...but this is just a prototype at this point.
}


void SocialPresenceEstimator::InitDependent(Vector::Robot *robot, const RobotCompMap &dependentComps)
{
  _robot = robot;

  // subscribe to inputs
  // new user intent
  auto& uic = dependentComps.GetComponent<AIComponent>().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
  _newUserIntentHandle = uic.RegisterNewUserIntentCallback( std::bind( &SocialPresenceEstimator::OnNewUserIntent, this) );
  // various vision-based things
  _faceHandle = _robot->GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotObservedFace,
      std::bind( &SocialPresenceEstimator::OnRobotObservedFace, this, std::placeholders::_1) );


  if( ANKI_DEV_CHEATS ) {
    SubscribeToWebViz();
  }

}


void SocialPresenceEstimator::UpdateDependent(const RobotCompMap& dependentComps)
{
  const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // update inputs here

  UpdateRSPI();

  if( ANKI_DEV_CHEATS && dependentComps.HasComponent<ContextWrapper>() ) {
    if( (currentTime - _lastWebVizSendTime_s) > kRSPE_WebVizPeriod_s ) {
      SendDataToWebViz(dependentComps.GetComponent<ContextWrapper>().context);
    }
  }
}


void SocialPresenceEstimator::UpdateInputs(const RobotCompMap& dependentComps)
{
  // poll the inputs we need to poll

  // let's start with user intent
  // just check IsAnyUserIntentPending? I don't see a way to get notified when it arrives
  // would need to keep track of whether it's new.
  // alternative: add a callback method to UserIntentComponent
}


void SocialPresenceEstimator::UpdateRSPI()
{
  // get current time
  const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  // compute dt_s
  const float dt_s = currentTime - _lastInputEventsUpdateTime_s;
  // limit update rate
  if (dt_s >= kMinRSPIUpdatePeriod_s) {

    // update all (singleton) input events
    for (SocialPresenceEvent* inputEvent : _inputEvents) {
      inputEvent->Update(dt_s);
    }
    // update all (dynamic) input events
    // cull any expired dynamic input events
    // update RSPI: iterate through all input events, summing their values
    // TODO (AS): I'm not totally happy with this implementation yet: hard to follow, at least.
    float newRSPI = 0;
    for (SocialPresenceEvent* inputEvent : _inputEvents) {
      //LOG_WARNING("SocialPresenceEstimator.UpdateRSPI.inputEventIteration",
      //    "inputEvent name %s value %f", inputEvent->GetName().c_str(), inputEvent->GetValue());
      if (_rspi >= inputEvent->GetIndependentEffectMax()) {
        newRSPI = fmax(-1.0, fmin(1.0, fmin( fmax(newRSPI, inputEvent->GetReinforcementEffectMax()),
                                             (newRSPI + inputEvent->GetValue()) ) ));
      } else {
        newRSPI = fmax(-1.0, fmin(1.0, fmin( fmax(newRSPI, inputEvent->GetIndependentEffectMax()),
                                             (newRSPI + inputEvent->GetValue()) ) ));
      }
    }
    _rspi = newRSPI;

    _lastInputEventsUpdateTime_s = currentTime;
  }
}


void SocialPresenceEstimator::TriggerInputEvent(SocialPresenceEvent* inputEvent) {
  LOG_WARNING("SocialPresenceEstimator.TriggerInputEvent.Triggering",
      "Triggering input event %s", inputEvent->GetName().c_str());
  if (inputEvent->GetReset()) {
    // reset all inputEvents
    for (auto* inputEvent : _inputEvents) {
      inputEvent->Reset();
    }
  }
  inputEvent->Trigger(_rspi);
  LOG_WARNING("SocialPresenceEstimator.TriggerInputEvent.TriggeredValue",
      "Triggered value of %s: %f", inputEvent->GetName().c_str(), inputEvent->GetValue());
}


// ******** Input Event Handlers ********

void SocialPresenceEstimator::OnNewUserIntent()
{
  LOG_WARNING("SocialPresenceEstimator.OnNewUserIntent.GotCallback", "");
  // Do some kind of DAS logging/data collection

  // trigger input event
  TriggerInputEvent(&_SPEUserIntent);
}


void SocialPresenceEstimator::OnRobotObservedFace(const AnkiEvent<ExternalInterface::MessageEngineToGame>& msg)
{
  LOG_WARNING("SocialPresenceEstimator.OnRobotObservedFace.GotCallback", "");
  // Do some kind of DAS logging/data collection

  // we might want to do dynamic event creation here, for different face identities.
  // For now we treat all faces as the same event

  TriggerInputEvent(&_SPEFace);
}


// ******** WebViz Stuff *********

void SocialPresenceEstimator::SubscribeToWebViz()
{
  if( _robot == nullptr ) {
    return;
  }
  const auto* context = _robot->GetContext();
  if( context == nullptr ) {
    return;
  }
  auto* webService = context->GetWebService();
  if( webService == nullptr ) {
    return;
  }

  auto onSubscribedBehaviors = [this](const std::function<void(const Json::Value&)>& sendToClient) {
    // a client subscribed.
    // send them a list of events to spoof

    Json:: Value subscriptionData;
    auto& data = subscriptionData["info"];
    auto& events = data["events"];
    for (auto* inputEvent : _inputEvents) {
      Json::Value eventEntry;
      const std::string eventName = inputEvent->GetName();
      eventEntry["eventName"] = eventName;
      events[eventName] = eventEntry;
    }

    sendToClient( subscriptionData );
  };

  auto onDataBehaviors = [this](const Json::Value& data, const std::function<void(const Json::Value&)>& sendToClient) {
    // if the client sent any emotion types, set them
    LOG_WARNING("RSPE.SubscribeToWebViz.onDataBehaviors.GotEvent", "RSPE got WebViz event %s", data["eventName"].asString().c_str());
    // TODO: a name : inputEvent map would be more efficient
    for (auto* inputEvent : _inputEvents) {
      LOG_WARNING("RSPE.SubscribeToWebViz.onDataBehaviors.nameComp",
          "got name %s, compare to %s", data["eventName"].asString().c_str(), inputEvent->GetName().c_str());
      if (data["eventName"].asString() == inputEvent->GetName()) {
        LOG_WARNING("RSPE.SubscribeToWebViz.onDataBehaviors.trigger", "triggering %s", inputEvent->GetName().c_str());
        TriggerInputEvent(inputEvent);
        break;
      }
    }
  };

  _signalHandles.emplace_back( webService->OnWebVizSubscribed( kWebVizModuleName ).ScopedSubscribe( onSubscribedBehaviors ) );
  _signalHandles.emplace_back( webService->OnWebVizData( kWebVizModuleName ).ScopedSubscribe( onDataBehaviors ) );
}


void SocialPresenceEstimator::SendDataToWebViz(const CozmoContext* context)
{
  if( nullptr == context ) {
    LOG_WARNING("RSPE.SendDataToWebViz.NoContext", "");
    return;
  }

  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  const auto* webService = context->GetWebService();
  if( nullptr != webService && webService->IsWebVizClientSubscribed(kWebVizModuleName)) {

    Json::Value data;
    data["time"] = currentTime_s;

    auto& graphData = data["graphData"];
    Json::Value rspi;
    rspi["name"] = "RSPI";
    rspi["value"] = _rspi;
    graphData.append(rspi);
    for (auto* inputEvent : _inputEvents) {
      Json::Value iedata;
      iedata["name"] = inputEvent->GetName();
      iedata["value"] = inputEvent->GetValue();
      graphData.append(iedata);
    }
    /*
    auto& moodData = data["moods"];

    for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
    {
      const EmotionType emotionType = (EmotionType)i;
      const float val = GetEmotionByIndex(i).GetValue();

      Json::Value entry;
      entry["emotion"] = EmotionTypeToString( emotionType );
      entry["value"] = val;
      moodData.append( entry );
    }

    if( ! emotionEvent.empty() ) {
      data["emotionEvent"] = emotionEvent;
    }

    data["simpleMood"] = SimpleMoodTypeToString(GetSimpleMood());
    */
    webService->SendToWebViz( kWebVizModuleName, data );
  }

  _lastWebVizSendTime_s = currentTime_s;
}




}
}