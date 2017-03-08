/**
 * File: blockTapFilterComponent.cpp
 *
 * Author: Molly Jameson
 * Created: 2016-07-07
 *
 * Description: A component to manage time delays to only send the most intense taps
 *               from blocks sent close together, since the other taps were likely noise
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/components/blockTapFilterComponent.h"

#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/utils/cozmoFeatureGate.h"
#include "anki/common/basestation/utils/timer.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/math/math.h"
#include "util/transport/connectionStats.h"

CONSOLE_VAR(int16_t, kTapIntensityMin, "TapFilter.IntesityMin", 60);
CONSOLE_VAR(Anki::TimeStamp_t, kTapWaitOffset_ms, "TapFilter.WaitOffsetTime", 75);
CONSOLE_VAR(Anki::TimeStamp_t, kDoubleTapTime_ms, "TapFilter.DoubleTapTime", 500);
CONSOLE_VAR(Anki::TimeStamp_t, kIgnoreMoveTimeAfterDoubleTap_ms, "TapFilter.IgnoreMoveTimeAfterDoubleTap", 500);
CONSOLE_VAR(bool, kCanDoubleTapDirtyPoses, "DoubleTap", true);
CONSOLE_VAR(bool, kIgnoreMovementWhileWaitingForDoubleTap, "DoubleTap", true);

namespace Anki {
namespace Cozmo {

BlockTapFilterComponent::BlockTapFilterComponent(Robot& robot)
  : _robot(robot)
  ,_enabled(true)
  ,_waitToTime(0)
{
  if( _robot.GetContext()->GetRobotManager()->GetMsgHandler() )
  {
    _robotToEngineSignalHandle.push_back(_robot.GetContext()->GetRobotManager()->GetMsgHandler()->Subscribe(_robot.GetID(),
                                                                                                     RobotInterface::RobotToEngineTag::activeObjectTapped,
                                                                                                     std::bind(&BlockTapFilterComponent::HandleActiveObjectTapped, this, std::placeholders::_1)));
    
    _robotToEngineSignalHandle.push_back(_robot.GetContext()->GetRobotManager()->GetMsgHandler()->Subscribe(_robot.GetID(),
                                                                                                            RobotInterface::RobotToEngineTag::activeObjectMoved,
                                                                                                            std::bind(&BlockTapFilterComponent::HandleActiveObjectMoved, this, std::placeholders::_1)));
    
    _robotToEngineSignalHandle.push_back(_robot.GetContext()->GetRobotManager()->GetMsgHandler()->Subscribe(_robot.GetID(),
                                                                                                           RobotInterface::RobotToEngineTag::activeObjectStopped,
                                                                                                           std::bind(&BlockTapFilterComponent::HandleActiveObjectStopped, this, std::placeholders::_1)));
    
  }
  // Null for unit tests
  if( _robot.GetContext()->GetExternalInterface() )
  {
    _gameToEngineSignalHandle = (_robot.GetContext()->GetExternalInterface()->Subscribe(
                                                                                        ExternalInterface::MessageGameToEngineTag::EnableBlockTapFilter,
                                                                                        std::bind(&BlockTapFilterComponent::HandleEnableTapFilter, this, std::placeholders::_1)));
  }
  
#if ANKI_DEV_CHEATS
  if( _robot.GetContext()->GetExternalInterface() )
  {
    _debugGameToEngineSignalHandle =(_robot.GetContext()->GetExternalInterface()->Subscribe(
                                                                                           ExternalInterface::MessageGameToEngineTag::GetBlockTapFilterStatus,
                                                                                           std::bind(&BlockTapFilterComponent::HandleSendTapFilterStatus, this, std::placeholders::_1)));;
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlockTapFilterComponent::Update()
{
  ANKI_CPU_PROFILE("BlockTapFilterComponent::Update");
  
  if( !_tapInfo.empty() )
  {
    const TimeStamp_t currTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    if( currTime > _waitToTime )
    {
      std::list<ObjectTapped>::iterator highIter = _tapInfo.begin();
      int16_t highIntensity = ((*highIter).tapPos - (*highIter).tapNeg);
      // grab with highest intensity from recent taps after set amount of time from first tap in group.
      for (std::list<ObjectTapped>::iterator it=_tapInfo.begin(); it != _tapInfo.end(); ++it)
      {
        int16_t currIntensity = ((*it).tapPos - (*it).tapNeg);
        if( highIntensity < currIntensity)
        {
          highIter = it;
          highIntensity = currIntensity;
        }
      }
      PRINT_CH_INFO("BlockPool","BlockTapFilterComponent.Update","intensity %d time: %d id: %d",highIntensity,currTime,(*highIter).objectID);
      _robot.Broadcast(ExternalInterface::MessageEngineToGame(ObjectTapped(*highIter)));
      _tapInfo.clear();
    }
  }
  
  for(auto& doubleTapInfo : _doubleTapObjects)
  {
    const TimeStamp_t curTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    const bool ignoreMovementTimePassed = doubleTapInfo.second.ignoreNextMoveTime <= curTime;
    
    // If we were ignoring move messages but the timeout has expired mark the object as dirty
    if(doubleTapInfo.second.isIgnoringMoveMessages &&
       ignoreMovementTimePassed)
    {
      doubleTapInfo.second.isIgnoringMoveMessages = false;
      
      BlockWorldFilter filter;
      filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
      filter.SetFilterFcn([&doubleTapInfo](const ObservableObject* object) {
        return object->IsActive() && (object->GetID() == doubleTapInfo.first);
      });
      
      std::vector<ObservableObject *> matchingObjects;
      _robot.GetBlockWorld().FindLocatedMatchingObjects(filter, matchingObjects);
      
      const ActiveObject* tappedObject = _robot.GetBlockWorld().GetConnectedActiveObjectByActiveID( doubleTapInfo.first );
      if ( nullptr != tappedObject ) {
        PRINT_CH_DEBUG("BlockTapFilterComponent", "BlockTapFilterComponent.Update.ExpiredTap",
                       "Marking object %d as dirty due to tap timeout",
                        tappedObject->GetID().GetValue());
      }
      
      for(auto& object : matchingObjects)
      {
        _robot.GetObjectPoseConfirmer().MarkObjectDirty(object);
      }
    }
  }
}
  
void BlockTapFilterComponent::HandleEnableTapFilter(const AnkiEvent<ExternalInterface::MessageGameToEngine>& message)
{
  if( message.GetData().GetTag() == ExternalInterface::MessageGameToEngineTag::EnableBlockTapFilter)
  {
    const Anki::Cozmo::ExternalInterface::EnableBlockTapFilter& msg = message.GetData().Get_EnableBlockTapFilter();
    _enabled = msg.enable;
    PRINT_CH_INFO("BlockPool","BlockTapFilterComponent.HandleEnableTapFilter","on %d",_enabled);
  }
}
  
#if ANKI_DEV_CHEATS
void BlockTapFilterComponent::HandleSendTapFilterStatus(const AnkiEvent<ExternalInterface::MessageGameToEngine>& message)
{
  if( message.GetData().GetTag() == ExternalInterface::MessageGameToEngineTag::GetBlockTapFilterStatus)
  {
    _robot.Broadcast(ExternalInterface::MessageEngineToGame(
                                        Anki::Cozmo::ExternalInterface::BlockTapFilterStatus(_enabled,kTapIntensityMin,kTapWaitOffset_ms)));
  }
}
#endif
  
void BlockTapFilterComponent::HandleActiveObjectTapped(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  // We make a copy of this message so we can update the object ID before broadcasting
  ObjectTapped payload = message.GetData().Get_activeObjectTapped();
  
  // Taps below threshold should be filtered and ignored.
  int16_t intensity = payload.tapPos - payload.tapNeg;
  if (intensity <= kTapIntensityMin)
  {
    PRINT_CH_INFO("BlockPool", "BlockTapFilterComponent.HandleEnableTapFilter.Ignored",
                  "Tap ignored %d <= %d",intensity,kTapIntensityMin);
    return;
  }

  // find connected object by activeID
  ActiveID tappedActiveID = payload.objectID;
  const ActiveObject* tappedObject = _robot.GetBlockWorld().GetConnectedActiveObjectByActiveID( tappedActiveID );

  if(nullptr == tappedObject)
  {
    PRINT_NAMED_WARNING("BlockTapFilterComponent.HandleActiveObjectTapped.UnknownActiveID",
                        "Could not find match for active object ID %d", payload.objectID);
    return;
  }
  
  const Anki::TimeStamp_t engineTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  PRINT_CH_INFO("BlockPool","BlockTapFilterComponent.HandleActiveObjectTapped.MessageActiveObjectTapped",
                "Received message that %s %d (Active ID %d) was tapped %d times "
                "(robotTime %d, tapTime %d, intensity: %d, engineTime: %d).",
                EnumToString(tappedObject->GetType()),
                tappedObject->GetID().GetValue(), payload.objectID, payload.numTaps,
                payload.timestamp, payload.tapTime, intensity, engineTime);
  
  // Update the ID to be the blockworld ID before broadcasting
  payload.objectID = tappedObject->GetID();
  payload.robotID = _robot.GetID();
  // In the simulator, taps are soft and also webots doesn't simulate the phantom taps.
  if (!_enabled || !_robot.IsPhysical())
  {
    // Do not filter any taps if block tap filtering was disabled.
    _robot.Broadcast(ExternalInterface::MessageEngineToGame(ObjectTapped(payload)));
  }
  else
  {
    // A new "group" of taps is coming in, evaluate after a certain amount of time after the first one
    if( _tapInfo.empty() )
    {
      // Potentially we could add more time based on LatencyAvg if we wanted to track that in the
      // shipping app. Latency should be higher on lower end devices
      _waitToTime = engineTime + kTapWaitOffset_ms;
    }

    _tapInfo.emplace_back(std::move(payload));
  }
  
  CheckForDoubleTap(payload.objectID);
}

void BlockTapFilterComponent::HandleActiveObjectMoved(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto& payload = message.GetData().Get_activeObjectMoved();
  // In the message coming from the robot, the objectID is the slot the object is connected on which is its
  // engine activeID
  const ObservableObject* object = _robot.GetBlockWorld().GetConnectedActiveObjectByActiveID(payload.objectID);
  if( object == nullptr )
  {
    PRINT_NAMED_WARNING("BlockTapFilterComponent.HandleActiveObjectMoved.ObjectIDNull",
                        "Could not find match for active object ID %d", payload.objectID);
    return;
  }
  
  const auto& doubleTapInfo = _doubleTapObjects.find(object->GetID().GetValue());
  
  if(doubleTapInfo != _doubleTapObjects.end())
  {
    // If we have not started waiting for a double tap then mark this cube as moving
    // Prevents checking for double taps while a cube is moving and also prevents
    // considering a cube is moving while we are waiting for a potential double tap since
    // taps/double taps often cause moved messages
    if(doubleTapInfo->second.doubleTapTime == 0)
    {
      doubleTapInfo->second.isMoving = true;
    }
  }
  else
  {
    _doubleTapObjects.emplace(object->GetID(), DoubleTapInfo{0, true, 0});
  }
}

void BlockTapFilterComponent::HandleActiveObjectStopped(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto& payload = message.GetData().Get_activeObjectStopped();
  // In the message coming from the robot, the objectID is the slot the object is connected on which is its
  // engine activeID
  const ObservableObject* object = _robot.GetBlockWorld().GetConnectedActiveObjectByActiveID(payload.objectID);
  if( object == nullptr )
  {
    PRINT_NAMED_WARNING("BlockTapFilterComponent.HandleActiveObjectStopped.ObjectIDNull",
                        "Could not find match for active object ID %d", payload.objectID);
    return;
  }
  
  const auto& doubleTapInfo = _doubleTapObjects.find(object->GetID().GetValue());
  
  if(doubleTapInfo != _doubleTapObjects.end())
  {
    doubleTapInfo->second.isMoving = false;
  }
  else
  {
    _doubleTapObjects.emplace(object->GetID(), DoubleTapInfo{0, false, 0});
  }
}

bool BlockTapFilterComponent::ShouldIgnoreMovementDueToDoubleTap(const ObjectID& objectID)
{
  if(!kIgnoreMovementWhileWaitingForDoubleTap)
  {
    return false;
  }

  const auto& doubleTapInfo = _doubleTapObjects.find(objectID);
  
  if(doubleTapInfo != _doubleTapObjects.end())
  {
    return doubleTapInfo->second.ignoreNextMoveTime > BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  }
  return false;
}

void BlockTapFilterComponent::CheckForDoubleTap(const ObjectID& objectID)
{
  const TimeStamp_t currTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  
  auto doubleTapInfo = _doubleTapObjects.find(objectID);
  if(doubleTapInfo == _doubleTapObjects.end())
  {
    doubleTapInfo = _doubleTapObjects.emplace(objectID, DoubleTapInfo{0, false, 0}).first;
  }
  
  // Don't check for double taps while the cube is moving
  if(doubleTapInfo->second.isMoving)
  {
    doubleTapInfo->second.doubleTapTime = 0;
    return;
  }
  
  // We have been waiting for a double tap and just got a tap within the double tap wait time
  if(currTime < doubleTapInfo->second.doubleTapTime)
  {
    PRINT_CH_INFO("BlockPool", "BlockTapFilterComponent.Update.DoubleTap",
                  "Detected double tap id:%u", objectID.GetValue());
    
    // Is ObjectTapInteractions feature enabled
    if(_robot.GetContext() != nullptr &&
       _robot.GetContext()->GetFeatureGate() != nullptr &&
       _robot.GetContext()->GetFeatureGate()->IsFeatureEnabled(FeatureType::ObjectTapInteractions))
    {
      _robot.GetBehaviorManager().HandleObjectTapInteraction(objectID);
    }
    
    doubleTapInfo->second.doubleTapTime = 0;
    doubleTapInfo->second.isIgnoringMoveMessages = false;
  }
  // Start waiting for a double tap
  else
  {
    doubleTapInfo->second.doubleTapTime = currTime + kDoubleTapTime_ms;
    doubleTapInfo->second.ignoreNextMoveTime = currTime + kIgnoreMoveTimeAfterDoubleTap_ms;
    doubleTapInfo->second.isIgnoringMoveMessages = true;
  }
}
  

}
}
