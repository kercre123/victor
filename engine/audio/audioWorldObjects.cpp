/*
 * File: audioWorldObjects.cpp
 *
 * Author: Jordan Rivas
 *
 * Copyright: Anki, Inc. 2017
 */


#include "engine/audio/audioWorldObjects.h"
#include "engine/audio/engineRobotAudioClient.h"
//#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/activeObject.h"

#include "clad/robotInterface/messageEngineToRobot.h"

//#include "engine/robotManager.h"
//#include "engine/robotInterface/messageHandler.h"
//#include "clad/externalInterface/messageGameToEngine.h"
//#include "clad/robotInterface/messageEngineToRobot.h"
//#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {  
namespace Audio {

//namespace AEM = AudioEngine::Multiplexer;
//namespace AMD = AudioMetaData;
AudioWorldObjects::AudioWorldObjects( Robot& robot )
: _robot( robot )
, _blockFilter(new BlockWorldFilter())
{
  _blockFilter->AddAllowedFamily(Anki::Cozmo::ObjectFamily::LightCube);
  // Setup world objects
  using namespace AudioMetaData;
  using namespace AudioMetaData::GameEvent;
  _objects.emplace(std::piecewise_construct,
                   std::forward_as_tuple((int32_t)ObjectType::Block_LIGHTCUBE1),
                   std::forward_as_tuple(WorldObject(GameObjectType::Block_1,
                                                     GenericEvent::Play__Sfx__Three_D_Generic_Object_01,
                                                     GenericEvent::Stop__Sfx__Three_D_Generic_Object_01)));
  _objects.emplace(std::piecewise_construct,
                   std::forward_as_tuple((int32_t)ObjectType::Block_LIGHTCUBE2),
                   std::forward_as_tuple(WorldObject(GameObjectType::Block_2,
                                                     GenericEvent::Play__Sfx__Three_D_Generic_Object_02,
                                                     GenericEvent::Stop__Sfx__Three_D_Generic_Object_02)));
  _objects.emplace(std::piecewise_construct,
                   std::forward_as_tuple((int32_t)ObjectType::Block_LIGHTCUBE3),
                   std::forward_as_tuple(WorldObject(GameObjectType::Block_3,
                                                     GenericEvent::Play__Sfx__Three_D_Generic_Object_03,
                                                     GenericEvent::Stop__Sfx__Three_D_Generic_Object_03)));
}

void UpdateWorldObject(const ObservableObject* activeObj, WorldObject& worldObj)
{
  worldObj.DidUpdate = true;
  const auto& translation = activeObj->GetPose().GetTranslation();
  worldObj.Xpos = translation.x();
  worldObj.Ypos = translation.y();
}
  
void AudioWorldObjects::Update()
{
//  printf("\nAudioWorldObjects::Update\n");
  // Get Robot Location
  const Pose3d& robotOrgin = _robot.GetPose();
  const auto& translation = robotOrgin.GetTranslation();
  Audio::UpdateWorldObjectPosition posMsg;
  posMsg.xPos = translation.x();
  posMsg.yPos = translation.y();
  posMsg.orientationRad = robotOrgin.GetRotationAngle<'Z'>().ToFloat();;
  posMsg.gameObject = AudioMetaData::GameObjectType::Cozmo_OnDevice;
  _robot.SendMessage(RobotInterface::EngineToRobot(std::move( posMsg )));
  
//  printf("\nAudioWorldObjects.Update.Robot Pos X %f   Y %f\n", translation.x(), translation.y());
  
  
  // Get Blocks in World Objects
  std::vector<const ObservableObject*> blocks;
  _robot.GetBlockWorld().FindLocatedMatchingObjects(*_blockFilter.get(), blocks);
  
  for (const ObservableObject* activeObj : blocks) {
//    printf("\nActive Obj Id %d Family %s Type %s validPose %c\n", activeObj->GetID().GetValue(),
//           EnumToString(activeObj->GetFamily()), EnumToString(activeObj->GetType()),
//           activeObj->HasValidPose() ? 'Y' : 'N');
    
    if ( !activeObj->HasValidPose() ) {
      continue;
    }
    
    switch (activeObj->GetType()) {
      case ObjectType::Block_LIGHTCUBE1:
      case ObjectType::Block_LIGHTCUBE2:
      case ObjectType::Block_LIGHTCUBE3:
      {
        const auto it = _objects.find((int32_t)activeObj->GetType());
        if (it != _objects.end()) {
          UpdateWorldObject( activeObj, it->second );
        }
      }
        break;
      default:
        break;
    }
  }
  
  // Send Block messages
  for (auto& pairIt : _objects)
  {
    auto& obj = pairIt.second;
    if (obj.DidUpdate) {
      Audio::UpdateWorldObjectPosition posMsg;
      posMsg.xPos = obj.Xpos;
      posMsg.yPos = obj.Ypos;
      posMsg.orientationRad = 0;
      posMsg.gameObject = obj.GameObject;
      _robot.SendMessage(RobotInterface::EngineToRobot(std::move( posMsg )));
      if (!obj.IsActive) {
        _robot.GetAudioClient()->PostEvent(obj.StartEvent,  // START
                                           obj.GameObject); //AudioMetaData::GameObjectType::Cozmo_OnDevice);// obj.GameObject);
        obj.IsActive = true;
      }
    }
    // Object was not found
    else {
      if (obj.IsActive) {
        _robot.GetAudioClient()->PostEvent(obj.EndEvent,   // STOP
                                           obj.GameObject);
        obj.IsActive = false;
      }
    }
  }
  
  
  
}

} // Audio
} // Cozmo
} // Anki
