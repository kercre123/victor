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

#include "engine/components/cubes/cubeAccelComponent.h"

#include "engine/activeObject.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubes/cubeAccelListeners/iCubeAccelListener.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

#include "clad/externalInterface/messageCubeToEngine.h"

#include "coretech/common/engine/utils/timer.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
CubeAccelComponent::CubeAccelComponent()
: IDependencyManagedComponent(this, RobotComponentID::CubeAccel)
{
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


void CubeAccelComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  // Check to see if anyone is using the listeners, and if not, remove them.
  for (auto& mapEntry : _listenerMap) {
    const auto objId = mapEntry.first;
    auto& listenerSet = mapEntry.second;
    for (auto iter = listenerSet.begin() ; iter != listenerSet.end() ; ) {
      if (iter->use_count() <= 1) {
        PRINT_NAMED_INFO("CubeAccelComponent.UpdateDependent.RemovingListener",
                         "Removing listener for objectID %d since no one is using it",
                         objId.GetValue());
        iter = listenerSet.erase(iter);
      } else {
        ++iter;
      }
    }
  }
}


bool CubeAccelComponent::AddListener(const ObjectID& objectID,
                                     const std::shared_ptr<CubeAccelListeners::ICubeAccelListener>& listener)
{
  const ActiveObject* obj = _robot->GetBlockWorld().GetConnectedActiveObjectByID(objectID);
  if (obj == nullptr) {
    PRINT_NAMED_WARNING("CubeAccelComponent.AddListener.InvalidObject",
                        "Object id %d is not connected",
                        objectID.GetValue());
    return false;
  }
  
  // Add this listener to our set of listeners for this object
  auto& listenersSet = _listenerMap[objectID];
  const auto resultPair = listenersSet.insert(listener);
  const bool successfullyInserted = resultPair.second;
  if (!successfullyInserted) {
    PRINT_NAMED_WARNING("CubeAccelComponent.AddListener.InsertFailed",
                        "Failed to insert listener for object %d. Does this listener already exist in the set?",
                        objectID.GetValue());
  }
  return successfullyInserted;
}


void CubeAccelComponent::HandleCubeAccelData(const ActiveID& activeID,
                                             const CubeAccelData& accelData)
{
  const ActiveObject* object = _robot->GetBlockWorld().GetConnectedActiveObjectByActiveID(activeID);
  if (object == nullptr) {
    DEV_ASSERT(false, "CubeAccelComponent.HandleCubeAccelData.NoConnectedObject");
    return;
  }
  
  const uint32_t objectID = object->GetID();
  
  // Check for taps
  if (accelData.tap_count != 0) {
    using namespace ExternalInterface;
    ObjectTapped objectTapped;
    objectTapped.timestamp = _robot->GetLastMsgTimestamp(); // TODO: Need a better timestamp here
    objectTapped.objectID  = objectID;
    objectTapped.numTaps   = accelData.tap_count;
    objectTapped.tapTime   = accelData.tap_time;
    objectTapped.tapNeg    = accelData.tap_neg;
    objectTapped.tapPos    = accelData.tap_pos;
    
    // Forward to game
    _robot->Broadcast(MessageEngineToGame(std::move(objectTapped)));
  }
  
  // Convert raw accelerometer data to mm/s^2
  auto rawAccelToMmps = [](const s16 rawAccel) {
    // Raw accel is an s16 with range -4g to +4g
    const float accelG = rawAccel / std::numeric_limits<s16>::max() * 4.f;
    return accelG * 9810.f;
  };
  ActiveAccel accel;
  accel.x = rawAccelToMmps(accelData.accel[0]);
  accel.y = rawAccelToMmps(accelData.accel[1]);
  accel.z = rawAccelToMmps(accelData.accel[2]);
  
  const auto& iter = _listenerMap.find(objectID);

  if(iter != _listenerMap.end()) {
    // Update all of the listeners with the accel data
    for(auto& listener : iter->second) {
      listener->Update(accel);
    }
  }
}
  
  
template<>
void CubeAccelComponent::HandleMessage(const ExternalInterface::ObjectConnectionState& msg)
{
  // TODO: This handler will create/destroy always-on listeners for ObjectMoved, ObjectUpAxisChanged, etc.
}
  
}
}
