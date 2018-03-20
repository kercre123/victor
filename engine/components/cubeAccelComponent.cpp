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

#include "engine/components/cubeAccelComponent.h"

#include "engine/activeObject.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "coretech/common/engine/utils/timer.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

namespace {
Anki::Cozmo::CubeAccelComponent* sThis = nullptr;
}
  
#if REMOTE_CONSOLE_ENABLED
namespace {
  
CONSOLE_VAR(u32, kCubeAccelWhichLightCube, "CubeAccelComponent.FakeAccel", 1); // LightCube 1, 2, or 3
CONSOLE_VAR(f32, kCubeAccelMagnitude_mm_s_s, "CubeAccelComponent.FakeAccel", 3500.0f);
  
void FakeCubeAcceleration(ConsoleFunctionContextRef context)
{
  const TimeStamp_t timestamp = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  uint32_t whichLightCube = kCubeAccelWhichLightCube;
  ActiveAccel accel(kCubeAccelMagnitude_mm_s_s, 0,0);
  ObjectAccel payload(timestamp,
                       0,
                       accel);
  
  if(sThis != nullptr){
    sThis->Dev_HandleObjectAccel(whichLightCube, payload);
  }
}

void FakeCubeShake(ConsoleFunctionContextRef context)
{
  uint32_t whichLightCube = kCubeAccelWhichLightCube;

  const TimeStamp_t previousTimestamp = BaseStationTimer::getInstance()->GetCurrentTimeStamp() - 1000;
  ActiveAccel positiveAccel(kCubeAccelMagnitude_mm_s_s, 0,0);
  ObjectAccel positivePayload(previousTimestamp,
                              0,
                              positiveAccel);
  
  const TimeStamp_t currentTimestamp = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  ActiveAccel negativeAccel(-kCubeAccelMagnitude_mm_s_s, 0,0);
  ObjectAccel negativePayload(currentTimestamp,
                              0,
                              negativeAccel);
  
  if(sThis != nullptr){
    sThis->Dev_HandleObjectAccel(whichLightCube, positivePayload);
    sThis->Dev_HandleObjectAccel(whichLightCube, negativePayload);
  }
}
  
CONSOLE_FUNC(FakeCubeAcceleration, "CubeAccelComponent.FakeAccel");
CONSOLE_FUNC(FakeCubeShake, "CubeAccelComponent.FakeAccel");

}
#endif
  
  
  
  
CubeAccelComponent::CubeAccelComponent()
: IDependencyManagedComponent(this, RobotComponentID::CubeAccel)
{  
  sThis = this;
}

CubeAccelComponent::~CubeAccelComponent()
{
  sThis = nullptr;
}

void CubeAccelComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) 
{
  _robot = robot;
  // Subscribe to messages
  if( _robot->HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*_robot->GetExternalInterface(), *this, _eventHandlers);
    using namespace ExternalInterface;
    helper.SubscribeEngineToGame<MessageEngineToGameTag::ObjectConnectionState>();
  }
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
    const ActiveObject* obj = _robot->GetBlockWorld().GetConnectedActiveObjectByID(objectID);
    if(obj != nullptr)
    {
      PRINT_CH_INFO("CubeAccelComponent",
                    "CubeAccelComponent.AddListener.StartingObjectAccelStream",
                    "ObjectID %d (activeID %d)",
                    objectID.GetValue(),
                    obj->GetActiveID());
      
      //_robot->GetCubeCommsComponent().SetStreamObjectAccel(obj->GetActiveID(), true);
    }else{
      PRINT_NAMED_WARNING("CubeAccelComponent.AddListener.InvalidObject",
                          "Object id %d is not connected",
                          objectID.GetValue());
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
        const ActiveObject* obj = _robot->GetBlockWorld().GetConnectedActiveObjectByID(accelHistory->first);
        if(obj != nullptr)
        {
          PRINT_CH_INFO("CubeAccelComponent",
                        "CubeAccelComponent.RemoveListener.StoppingObjectAccelStream",
                        "ObjectID %d (activeID %d)",
                        obj->GetID().GetValue(),
                        obj->GetActiveID());
          
          //_robot->GetCubeCommsComponent().SetStreamObjectAccel(obj->GetActiveID(), false);
        }
        
        // Reset window size to default value
        accelHistory->second.windowSize_ms = kDefaultWindowSize_ms;
      }
      return true;
    }
  }
  return false;
}

  
void CubeAccelComponent::HandleObjectAccel(const ObjectAccel& objectAccel)
{
  const ActiveObject* object = _robot->GetBlockWorld().GetConnectedActiveObjectByActiveID(objectAccel.objectID);
  if(object == nullptr)
  {
    // It's possible to receive an objectAccel message before the cube
    // re-connection message resulting in the object not being in block world
    // So assert that we don't have any tracking data still valid on the cube
    // we can't find
    auto iter = _objectAccelHistory.find(objectAccel.objectID);
    DEV_ASSERT_MSG((iter == _objectAccelHistory.end()) ||
                   iter->second.listeners.empty(),
                   "CubeAccelComponent.HandleObjectAccel.NoConnectedObject",
                   "Object %d is not in block world, but we still have data on it",
                   objectAccel.objectID);
    return;
  }
  
  const uint32_t objectID = object->GetID();
  const auto& iter = _objectAccelHistory.find(objectID);

  // We should have an entry in _objectAccelHistory for the object this accel data is coming from
  if(ANKI_VERIFY(iter != _objectAccelHistory.end(),
                 "CubeAccelComponent.HandleObjectAccel.NoObjectIDInHistory",
                 "No accel history for objectID %u",
                 objectID))
  {
    // Add accel data to history
    iter->second.history[objectAccel.timestamp] = objectAccel.accel;
    
    // Update all of the listeners with the accel data
    for(auto& listener : iter->second.listeners)
    {
      listener->Update(objectAccel.accel);
    }
    
    CullToWindowSize(iter->second);
  }
  
  // Forward to game and viz
  _robot->Broadcast(ExternalInterface::MessageEngineToGame(ObjectAccel(objectAccel.timestamp, objectID, objectAccel.accel)));
  _robot->GetContext()->GetVizManager()->SendObjectAccelState(objectID, objectAccel.accel);
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

void CubeAccelComponent::Dev_HandleObjectAccel(const u32 whichLightCubeType, ObjectAccel& objectAccel)
{
  static const std::map<u32, ObjectType> kLightCubeTypeToObjectType = {
    {1, ObjectType::Block_LIGHTCUBE1},
    {2, ObjectType::Block_LIGHTCUBE2},
    {3, ObjectType::Block_LIGHTCUBE3}
  };
  
  const auto& objectTypeIter = kLightCubeTypeToObjectType.find(whichLightCubeType);
  if(objectTypeIter != kLightCubeTypeToObjectType.end())
  {
    BlockWorldFilter filter;
    filter.SetAllowedTypes({objectTypeIter->second});

    const ObservableObject* object = _robot->GetBlockWorld().FindConnectedActiveMatchingObject(filter);
    if(object != nullptr)
    {
      objectAccel.objectID = object->GetID();
      HandleObjectAccel(objectAccel);
    }
  }
}
  
  
template<>
void CubeAccelComponent::HandleMessage(const ObjectConnectionState& msg)
{
  if(msg.connected)
  {
    auto iter = _objectAccelHistory.find(msg.objectID);
    DEV_ASSERT_MSG((iter == _objectAccelHistory.end()) ||
                   iter->second.listeners.empty(),
                   "CubeAccelComponent.HandleMessage.ObjectConnectionState",
                   "Connecting to object %d which has a dangling listener",
                   msg.objectID);
  }
}
  
}
}
