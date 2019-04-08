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
#include "engine/ankiEventUtil.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/logging/logging.h"
#include "webServerProcess/src/webService.h"

namespace Anki {
namespace Vector {

#define LOG_CHANNEL "RSPE"

namespace{
  const std::string kWebVizModuleName = "socialpresence";
}

SocialPresenceEstimator::SocialPresenceEstimator()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::SocialPresenceEstimator)
{

}


SocialPresenceEstimator::~SocialPresenceEstimator() {}


void SocialPresenceEstimator::InitDependent(Vector::Robot *robot, const RobotCompMap &dependentComps)
{
  _robot = robot;

  if( ANKI_DEV_CHEATS ) {
    SubscribeToWebViz();
  }
}


void SocialPresenceEstimator::UpdateDependent(const RobotCompMap &dependentComps)
{
  //LOG_INFO("RSPE.UpdateDependent.Update", "RSPE Update ran."); // TODO: this can't stay in, obviously.
}


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

  auto onSubscribedBehaviors = [](const std::function<void(const Json::Value&)>& sendToClient) {
    // a client subscribed.
    // send them a list of events to spoof

    Json:: Value subscriptionData;
    auto& data = subscriptionData["info"];
    auto& events = data["events"];
    std::vector<std::string> tmp_event_names {"test1", "test2"};
    for( uint8_t i=0; i<static_cast<uint8_t>(tmp_event_names.size()); ++i ) {
      Json::Value eventEntry;
      eventEntry["eventName"] = tmp_event_names[i];

      //events.append( eventEntry );
      events[tmp_event_names[i]] = eventEntry;
    }

    sendToClient( subscriptionData );
  };

  auto onDataBehaviors = [](const Json::Value& data, const std::function<void(const Json::Value&)>& sendToClient) {
    // if the client sent any emotion types, set them
    LOG_WARNING("RSPE.SubscribeToWebViz.onDataBehaviors.GotEvent", "RSPE got WebViz event %s", data["eventName"].asString().c_str());
  };

  _signalHandles.emplace_back( webService->OnWebVizSubscribed( kWebVizModuleName ).ScopedSubscribe( onSubscribedBehaviors ) );
  _signalHandles.emplace_back( webService->OnWebVizData( kWebVizModuleName ).ScopedSubscribe( onDataBehaviors ) );
}

/*
void MoodManager::SendDataToWebViz(const CozmoContext* context, const std::string& emotionEvent)
{
  if( nullptr == context ) {
    return;
  }

  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  const auto* webService = context->GetWebService();
  if( nullptr != webService && webService->IsWebVizClientSubscribed(kWebVizModuleName)) {

    Json::Value data;
    data["time"] = currentTime_s;

    data["RSPI"] = 0; // TODO: temporary, obvs

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

    webService->SendToWebViz( kWebVizModuleName, data );
  }

  _lastWebVizSendTime_s = currentTime_s;
}
*/

}
}