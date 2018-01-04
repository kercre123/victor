/**
 * File: behaviorComponentCloudReceiver.cpp
 *
 * Author: Kevin M. Karol
 * Created: 10/30/17
 *
 * Description: Handles messages from the cloud that relate to the behavior component
 * And stores the derived information for the behavior component to access and use
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviorComponentCloudReceiver.h"

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"

#include "util/console/consoleInterface.h"
#include "util/helpers/fullEnumToValueArrayChecker.h"



namespace Anki {
namespace Cozmo {

using FullCloudIntentArray = Util::FullEnumToValueArrayChecker::FullEnumToValueArray<CloudIntent, std::string, CloudIntent::Count>;
using Util::FullEnumToValueArrayChecker::IsSequentialArray; // import IsSequentialArray to this namespace

const FullCloudIntentArray cloudStringMap{
  {CloudIntent::AnyTrick,        "intent_play_anytrick"},
  {CloudIntent::BeQuiet,         "intent_imperative_quiet"},
  {CloudIntent::ComeHere,        "intent_imperative_come"},
  {CloudIntent::Goodbye,         "intent_greeting_goodbye"},
  {CloudIntent::Hello,           "intent_greeting_hello"},
  {CloudIntent::HowAreYou,       "intent_status_feeling"},
  {CloudIntent::ReturnToCharger, "intent_system_charger"},
  {CloudIntent::Sleep,           "intent_system_sleep"},
  {CloudIntent::TriggerDetected, "TriggerDetected"},
  {CloudIntent::WakeUp,          "intent_system_wake"}
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorComponentCloudReceiver::BehaviorComponentCloudReceiver(Robot& robot)
: _server(std::bind(&BehaviorComponentCloudReceiver::AddPendingIntent, this, std::placeholders::_1), 
                    12345 + robot.GetID() )  // Offset port by robotID so that we can run sims with multiple robots
{
  if(robot.HasExternalInterface()){
    auto fakeTriggerWordCallback = [this](const GameToEngineEvent& event) {
        PRINT_CH_INFO("BehaviorSystem","BehaviorComponentCloudReceiver.ReceivedFakeTriggerWordDetected","");        
        AddPendingIntent(CloudIntentToString(CloudIntent::TriggerDetected));
    };
    auto triggerWordCallback = [this](const AnkiEvent<RobotInterface::RobotToEngine>& event){
        PRINT_CH_INFO("BehaviorSystem","BehaviorComponentCloudReceiver.TriggerWordDetected","");                
        AddPendingIntent(CloudIntentToString(CloudIntent::TriggerDetected));
    };

    auto fakeCloudIntentCallback = [this](const GameToEngineEvent& event) {
        PRINT_CH_INFO("BehaviorSystem","BehaviorComponentCloudReceiver.FakeCloudIntentReceived","");
        std::string intentionalCopy = event.GetData().Get_FakeCloudIntent().intent;                        
        AddPendingIntent(std::move(intentionalCopy));
    };


    if(robot.HasExternalInterface()){
        auto EI = robot.GetExternalInterface();
        _eventHandles.push_back(
            EI->Subscribe(GameToEngineTag::FakeTriggerWordDetected, fakeTriggerWordCallback));
        _eventHandles.push_back(
            EI->Subscribe(GameToEngineTag::FakeCloudIntent, fakeCloudIntentCallback));
    }
    if(robot.GetRobotMessageHandler() != nullptr){
      _eventHandles.push_back(robot.GetRobotMessageHandler()->Subscribe(robot.GetID(),
                                                  RobotInterface::RobotToEngineTag::triggerWordDetected,
                                                  triggerWordCallback));
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorComponentCloudReceiver::IsIntentPending(CloudIntent intent)
{
  std::lock_guard<std::mutex> lock{_mutex};  
  if(_pendingIntents.empty()){
      return false;
  }
  const std::string& intentStr = cloudStringMap[Util::EnumToUnderlying(intent)].Value();
  for(auto& pendingStr: _pendingIntents){
    if(pendingStr == intentStr){
      return true;
    }
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponentCloudReceiver::ClearIntentIfPending(CloudIntent intent)
{
  std::lock_guard<std::mutex> lock{_mutex};  
  if(_pendingIntents.empty()){
    return;
  }
  const std::string& intentStr = cloudStringMap[Util::EnumToUnderlying(intent)].Value();
  for(auto iter = _pendingIntents.begin(); iter != _pendingIntents.end(); iter++){
    if((*iter) == intentStr){
      _pendingIntents.erase(iter);
      return;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponentCloudReceiver::AddPendingIntent(std::string&& intent)
{
    std::lock_guard<std::mutex> lock{_mutex};
    // handle message here; wrap other calls that access/modify
    // fields modified here in a mutex lock as well
    _pendingIntents.push_back(std::move(intent));
}


} // namespace Cozmo
} // namespace Anki
