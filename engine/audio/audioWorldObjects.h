/*
 * File: audioWorldObjects.h
 *
 * Author: Jordan Rivas
 *
 *
 * Copyright: Anki, Inc. 2017
 */


#ifndef __Cozmo_Basestation_AudioWorldObjects_H__
#define __Cozmo_Basestation_AudioWorldObjects_H__


//#include "audioEngine/multiplexer/audioMuxClient.h"
//#include "engine/events/ankiEvent.h"

#include "clad/audio/audioEventTypes.h"
#include "clad/audio/audioGameObjectTypes.h"
#include <memory>
#include <vector>
#include <unordered_map>


namespace Anki {
namespace Cozmo {
class BlockWorldFilter;
class Robot;

namespace Audio {

struct WorldObject
{
  bool IsActive;
  bool DidUpdate;
  AudioMetaData::GameObjectType GameObject;
  AudioMetaData::GameEvent::GenericEvent StartEvent;
  AudioMetaData::GameEvent::GenericEvent EndEvent;
  float Xpos;
  float Ypos;
  
  WorldObject(AudioMetaData::GameObjectType gameObject,
              AudioMetaData::GameEvent::GenericEvent startEvent,
              AudioMetaData::GameEvent::GenericEvent endEvent)
  : IsActive(false)
  , DidUpdate(false)
  , GameObject(gameObject)
  , StartEvent(startEvent)
  , EndEvent(endEvent)
  , Xpos( 0.0f )
  , Ypos( 0.0f ) { }
};

const int kBlockCount = 3;
  
class AudioWorldObjects {
public:
  
  AudioWorldObjects( Robot& robot );


  void Update();


private:
  Robot& _robot;
  std::unique_ptr<BlockWorldFilter> _blockFilter;
  
  std::unordered_map<int32_t, WorldObject> _objects;

};

  


} // Audio
} // Cozmo
} // Anki



#endif /* __Cozmo_Basestation_AudioWorldObjects_H__ */
