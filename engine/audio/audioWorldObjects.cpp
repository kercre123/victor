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
  
  _blocks[0].WorldObjectType = (int8_t)Audio::WorldObjectType::Block_1;
  _blocks[1].WorldObjectType = (int8_t)Audio::WorldObjectType::Block_2;
  _blocks[2].WorldObjectType = (int8_t)Audio::WorldObjectType::Block_3;
  
//  _blockActive[0].ob
}

void UpdateWorldObject(const ObservableObject* activeObj, WorldObject& worldObj)
{
  worldObj.DidUpdated = true;
  const auto& translation = activeObj->GetPose().GetTranslation();
  worldObj.Xpos = translation.x();
  worldObj.Ypos = translation.y();
}
  
void AudioWorldObjects::Update()
{
  printf("\nAudioWorldObjects::Update\n");
  // Get Robot Location
  const Pose3d& robotOrgin = _robot.GetPose();
  const auto& translation = robotOrgin.GetTranslation();
  Audio::UpdateWorldObjectPosition posMsg;
  posMsg.xPos = translation.x();
  posMsg.yPos = translation.y();
  posMsg.orientationRad = robotOrgin.GetRotationAngle<'Z'>().ToFloat();;
  posMsg.worldObject = Audio::WorldObjectType::Robot;
  _robot.SendMessage(RobotInterface::EngineToRobot(std::move( posMsg )));
  
  printf("\nAudioWorldObjects.Update.Robot Pos X %f   Y %f\n", translation.x(), translation.y());
  
  
  // Get Blocks in World Objects
  std::vector<const ObservableObject*> blocks;
//  _robot.GetBlockWorld().GetLocatedObjectByID(<#const Anki::ObjectID &objectID#>)
  _robot.GetBlockWorld().FindLocatedMatchingObjects(*_blockFilter.get(), blocks);
  
  for (const ObservableObject* activeObj : blocks) {
    printf("\nActive Obj Id %d Family %s Type %s validPose %c\n", activeObj->GetID().GetValue(),
           EnumToString(activeObj->GetFamily()), EnumToString(activeObj->GetType()),
           activeObj->HasValidPose() ? 'Y' : 'N');
    
    if ( !activeObj->HasValidPose() ) {
      continue;
    }
    
    switch (activeObj->GetType()) {
      case ObjectType::Block_LIGHTCUBE1:
      {
        UpdateWorldObject(activeObj, _blocks[0]);
      }
        break;
      case ObjectType::Block_LIGHTCUBE2:
      {
        UpdateWorldObject(activeObj, _blocks[1]);
      }
        break;
      case ObjectType::Block_LIGHTCUBE3:
      {
        UpdateWorldObject(activeObj, _blocks[2]);
      }
        break;
      default:
        break;
    }
  }
  
  // Send Block messages
  for (int idx = 0; idx < kBlockCount; ++idx) {
    auto& block = _blocks[idx];
    if (block.DidUpdated) {
      Audio::UpdateWorldObjectPosition posMsg;
      posMsg.xPos = block.Xpos;
      posMsg.yPos = block.Ypos;
      posMsg.orientationRad = 0;
      posMsg.worldObject = (Audio::WorldObjectType)block.WorldObjectType;
      _robot.SendMessage(RobotInterface::EngineToRobot(std::move( posMsg )));
      if (!block.IsActive) {
//        _robot.GetAudioClient()->PostEvent(AudioMetaData::GameEvent::GenericEvent::Invalid,  // START
//                                           AudioMetaData::GameObjectType::Cozmo_OnDevice);
        block.IsActive = true;
      }
    }
    // Object was not found
    else {
      if (block.IsActive) {
//        _robot.GetAudioClient()->PostEvent(AudioMetaData::GameEvent::GenericEvent::Invalid,  // STOP
//                                           AudioMetaData::GameObjectType::Cozmo_OnDevice);
        block.IsActive = false;
      }
    }
  }
  
  
  
}

} // Audio
} // Cozmo
} // Anki
