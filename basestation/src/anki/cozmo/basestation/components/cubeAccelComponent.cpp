/**
 * File: cubeAccelComponent.cpp
 *
 * Author: Al Chaussee
 * Created: 04/10/2017
 *
 * Description: Manages streamed object accel data
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/components/cubeAccelComponent.h"

#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

CubeAccelComponent::CubeAccelComponent(Robot& robot)
: _robot(robot)
{
  _eventHandler = robot.GetRobotMessageHandler()->Subscribe(robot.GetID(),
                                                            RobotInterface::RobotToEngineTag::objectAccel,
                                                            std::bind(&CubeAccelComponent::HandleObjectAccel, this, std::placeholders::_1));
}

CubeAccelComponent::~CubeAccelComponent()
{
  
}

void CubeAccelComponent::AddListener(const ObjectID& objectID,
                                     const std::shared_ptr<CubeAccelListeners::ICubeAccelListener>& listener,
                                     const TimeStamp_t& windowSize_ms)
{
  auto iter = _objectAccelHistory.find(objectID);
  if(iter == _objectAccelHistory.end())
  {
    // No AccelHistory for this object so add one
    AccelHistory accelHistory;
    auto pair = _objectAccelHistory.emplace(objectID, std::move(accelHistory));
    DEV_ASSERT(pair.second, "CubeAccelComponent.AddListener.EmplaceFailed");
    iter = pair.first;
  }
  
  // If there are no listeners of this object's accel data then streaming
  // is not enabled for the object so enable it
  if(iter->second.listeners.empty())
  {
    const ActiveObject* obj = _robot.GetBlockWorld().GetConnectedActiveObjectByID(objectID);
    if(obj != nullptr)
    {
      PRINT_CH_INFO("CubeAccelComponent",
                    "CubeAccelComponent.AddListener.StartingObjectAccelStream",
                    "ObjectID %d (activeID %d)",
                    objectID.GetValue(),
                    obj->GetActiveID());
      
      _robot.SendMessage(RobotInterface::EngineToRobot(StreamObjectAccel(obj->GetActiveID(), true)));
    }
  }
  
  // TODO Run listener over all of history if there is history? CullToWindowSize() first in case history is old
  // Add this listener to our set of listeners for this object
  iter->second.listeners.insert(listener);
  
  // Make sure window size is never smaller than what any one call to AddListener requested
  iter->second.windowSize_ms = MAX(iter->second.windowSize_ms, windowSize_ms);
}

bool CubeAccelComponent::RemoveListener(const ObjectID& objectID,
                                        const std::shared_ptr<CubeAccelListeners::ICubeAccelListener>& listener)
{
  auto accelHistory = _objectAccelHistory.find(objectID);
  if(accelHistory != _objectAccelHistory.end())
  {
    auto iter = accelHistory->second.listeners.find(listener);
    
    // If the listener is in the set, remove it
    if(iter != accelHistory->second.listeners.end())
    {
      accelHistory->second.listeners.erase(iter);
      
      // TODO: Consider recalculating accelHistory->second.windowSize_ms after removing a listener
      // This would require storing windowSize_ms per listener
      
      // If there are no more listeners then disable object accel streaming
      if(accelHistory->second.listeners.empty())
      {
        const ActiveObject* obj = _robot.GetBlockWorld().GetConnectedActiveObjectByID(accelHistory->first);
        if(obj != nullptr)
        {
          PRINT_CH_INFO("CubeAccelComponent",
                        "CubeAccelComponent.RemoveListener.StoppingObjectAccelStream",
                        "ObjectID %d (activeID %d)",
                        obj->GetID().GetValue(),
                        obj->GetActiveID());
          
          _robot.SendMessage(RobotInterface::EngineToRobot(StreamObjectAccel(obj->GetActiveID(), false)));
        }
        
        // Reset window size to default value
        accelHistory->second.windowSize_ms = kDefaultWindowSize_ms;
      }
      return true;
    }
  }
  return false;
}

void CubeAccelComponent::HandleObjectAccel(const AnkiEvent<RobotInterface::RobotToEngine>& msg)
{
  const ObjectAccel& payload = msg.GetData().Get_objectAccel();
  const ActiveObject* object = _robot.GetBlockWorld().GetConnectedActiveObjectByActiveID(payload.objectID);
  DEV_ASSERT(object != nullptr, "CubeAccelComponent.HandleObjectAccel.GetAccelForUnconnectedObject");
  
  const uint32_t objectID = object->GetID();
  const auto& iter = _objectAccelHistory.find(objectID);
  // TODO (Al) Switch to "if with ANKI_VERIFY" when all systems using streaming object accel data are updated
  // to go through this component
  // We should have an entry in _objectAccelHistory for the object this accel data is coming from
# if 0
  if(ANKI_VERIFY(iter != _objectAccelHistory.end(),
                 "CubeAccelComponent.HandleObjectAccel.NoObjectIDInHistory",
                 "No accel history for objectID %u",
                 objectID))
# else
  if(iter != _objectAccelHistory.end())
# endif
  {
    // Add accel data to history
    iter->second.history[payload.timestamp] = payload.accel;
    
    // Update all of the listeners with the accel data
    for(auto& listener : iter->second.listeners)
    {
      listener->Update(payload.accel);
    }
    
    CullToWindowSize(iter->second);
  }
  
  // Forward to game and viz
  _robot.Broadcast(ExternalInterface::MessageEngineToGame(ObjectAccel(payload.timestamp, objectID, payload.accel)));
  _robot.GetContext()->GetVizManager()->SendObjectAccelState(objectID, payload.accel);
}
  
void CubeAccelComponent::CullToWindowSize(AccelHistory& accelHistory)
{
  if(accelHistory.history.size() > 1)
  {
    const TimeStamp_t mostRecentTime = accelHistory.history.rbegin()->first;
    
    if(mostRecentTime < accelHistory.windowSize_ms)
    {
      return;
    }
    
    const TimeStamp_t oldestAllowedTime = mostRecentTime - accelHistory.windowSize_ms;
    const auto it = accelHistory.history.lower_bound(oldestAllowedTime);
    
    if(!accelHistory.history.empty() && it != accelHistory.history.begin())
    {
      accelHistory.history.erase(accelHistory.history.begin(), it);
    }
  }
}
  
}
}
