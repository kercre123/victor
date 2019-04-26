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
#include "clad/types/salientPointTypes.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/ankiEventUtil.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/mics/micDirectionHistory.h"
#include "engine/components/mics/micDirectionTypes.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "util/console/consoleInterface.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/logging/DAS.h"
#include "util/logging/logging.h"
#include "webServerProcess/src/webService.h"

namespace Anki {
namespace Vector {

#define LOG_CHANNEL "RSPE"

namespace {
  float kEpsilon = 0.01; // threshold for calling a SocialPresenceEvent's value zero
  float kmicPowerScoreThreshold = 2.0; // TODO: not tuned at all, and definitely will want to be.
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

// TODO: change this so that it applies the decay to a multiplier that starts at ~1.0, rather than directly on the value
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
  _value = (*_decay)(_value, dt_s);
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
  _newUserIntentHandle =
      uic.RegisterNewUserIntentCallback( std::bind( &SocialPresenceEstimator::OnNewUserIntent, this, std::placeholders::_1) );

  // various vision-based things
  _faceHandle = _robot->GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotObservedFace,
      std::bind( &SocialPresenceEstimator::OnRobotObservedFace, this, std::placeholders::_1) );
  _faceHandle = _robot->GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotObservedMotion,
      std::bind( &SocialPresenceEstimator::OnRobotObservedMotion, this, std::placeholders::_1) );
  _salientHandle = _robot->GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotObservedSalientPoint,
      std::bind( &SocialPresenceEstimator::OnRobotObservedSalientPoint, this, std::placeholders::_1) );

  _micPowerSampleHandle = dependentComps.GetComponent<MicComponent>().GetMicDirectionHistory().RegisterSoundReactor(
      std::bind( &SocialPresenceEstimator::OnMicPowerSample, this,
          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3) );

  if( ANKI_DEV_CHEATS ) {
    SubscribeToWebViz();
  }
}


void SocialPresenceEstimator::UpdateDependent(const RobotCompMap& dependentComps)
{
  const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  UpdateInputs(dependentComps);

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
  auto& touch = dependentComps.GetComponent<TouchSensorComponent>();
  PollTouch(touch);
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
  LOG_INFO("SocialPresenceEstimator.TriggerInputEvent.Triggering",
      "Triggering input event %s", inputEvent->GetName().c_str());
  if (inputEvent->GetReset()) {
    // reset all inputEvents
    for (auto* inputEvent : _inputEvents) {
      inputEvent->Reset();
    }
  }
  inputEvent->Trigger(_rspi);
  LOG_INFO("SocialPresenceEstimator.TriggerInputEvent.TriggeredValue",
      "Triggered value of %s: %f", inputEvent->GetName().c_str(), inputEvent->GetValue());
}


// DAS logging for data collection
void SocialPresenceEstimator::LogInputEvent(SocialPresenceEvent* inputEvent)
{
  DASMSG(rspe_inputEvent, "RSPE.InputEvent", "Receptive Social Presence Estimator Input Event");
  DASMSG_SET(s1, inputEvent->GetName(), "Event Name");
  // could add additional detail here if needed
  DASMSG_SEND();
}


// ******** Input Event Handlers ********

void SocialPresenceEstimator::OnNewUserIntent(const UserIntentTag tag)
{
  // filter out specific intents here
  if (tag == USER_INTENT(system_sleep)) {
    LOG_DEBUG("SocialPresenceEstimator.OnNewUserIntent.Specific", "system_sleep");
    LogInputEvent(&_SPESleep);
    TriggerInputEvent(&_SPESleep);
  } else if (tag == USER_INTENT(imperative_quiet)) {
    LOG_DEBUG("SocialPresenceEstimator.OnNewUserIntent.Specific", "imperative_quiet");
    LogInputEvent(&_SPEQuiet);
    TriggerInputEvent(&_SPEQuiet);
  } else if (tag == USER_INTENT(imperative_shutup)) {
    LOG_DEBUG("SocialPresenceEstimator.OnNewUserIntent.Specific", "imperative_shutup");
    LogInputEvent(&_SPEShutUp);
    TriggerInputEvent(&_SPEShutUp);
  } else {
    LogInputEvent(&_SPEUserIntent);
    TriggerInputEvent(&_SPEUserIntent);
  }
}


void SocialPresenceEstimator::OnRobotObservedFace(const AnkiEvent<ExternalInterface::MessageEngineToGame>& msg)
{
  // we might want to do dynamic event creation here, for different face identities.
  // For now we treat all faces as the same event
  LogInputEvent(&_SPEFace);
  TriggerInputEvent(&_SPEFace);
}


void SocialPresenceEstimator::OnRobotObservedMotion(const AnkiEvent<ExternalInterface::MessageEngineToGame>& msg)
{
  LogInputEvent(&_SPEMotion);
  TriggerInputEvent(&_SPEMotion);
}


void SocialPresenceEstimator::OnRobotObservedSalientPoint(const AnkiEvent<ExternalInterface::MessageEngineToGame>& msg)
{
  LOG_WARNING("SocialPresenceEstimator.OnRobotObservedSalientPoint.GotCallback", "");
  // distinguish between different salient points and dispatch
  DEV_ASSERT(ExternalInterface::MessageEngineToGameTag::RobotObservedSalientPoint == msg.GetData().GetTag(),
             "SocialPresenceEstimator.OnRobotObservedSalientPoint.MessageTypeNotHandled");
  const ExternalInterface::RobotObservedSalientPoint& spMsg = msg.GetData().Get_RobotObservedSalientPoint();
  switch(spMsg.salientPoint.salientType) {
    case Vision::SalientPointType::Person:
      LogInputEvent(&_SPEPerson);
      TriggerInputEvent(&_SPEPerson);
      break;
    case Vision::SalientPointType::Hand:
      LogInputEvent(&_SPEHand);
      TriggerInputEvent(&_SPEHand);
      break;
    default:
      LOG_WARNING("SocialPresenceEstimator.OnRobotObservedSalientPoint.UnhandledSalientType","");
  }
}


bool SocialPresenceEstimator::OnMicPowerSample(double micPowerLevel, MicDirectionConfidence conf, MicDirectionIndex dir)
{
  //LOG_WARNING("SocialPresenceEstimator.OnMicPowerSample.GotSample", "power score: %f", micPowerLevel);
  // probably we need some kind of threshold on the power score
  // TODO: can we make the power score an input to the event, with a proportional effect?
  if (micPowerLevel > kmicPowerScoreThreshold) {
    LOG_DEBUG("SocialPresenceEstimator.OnMicPowerSample.GotSampleAboveThreshold", "power score: %f", micPowerLevel);
    LogInputEvent(&_SPESound);
    TriggerInputEvent(&_SPESound);
  }
  return true;
}


// ******** Input Event pollers ********

void SocialPresenceEstimator::PollTouch(const TouchSensorComponent& touchSensorComponent)
{
  if (touchSensorComponent.GetIsPressed()) {
    LogInputEvent(&_SPETouch);
    TriggerInputEvent(&_SPETouch);
  }
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
    LOG_DEBUG("RSPE.SubscribeToWebViz.onDataBehaviors.GotEvent", "RSPE got WebViz event %s", data["eventName"].asString().c_str());
    // TODO: a name : inputEvent map would be more efficient
    for (auto* inputEvent : _inputEvents) {
      if (data["eventName"].asString() == inputEvent->GetName()) {
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

    webService->SendToWebViz( kWebVizModuleName, data );
  }

  _lastWebVizSendTime_s = currentTime_s;
}




}
}