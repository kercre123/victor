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
#include <memory>
#include <vector>


namespace Anki {
namespace Cozmo {
class BlockWorldFilter;
class Robot;

namespace Audio {

struct WorldObject
{
  bool IsActive = false;
  bool DidUpdated = false;
  uint8_t WorldObjectType = 0;
  float Xpos = 0.0f;
  float Ypos = 0.0f;
};

const int kBlockCount = 3;
  
class AudioWorldObjects {
public:
  
  AudioWorldObjects( Robot& robot );


  void Update();


private:
  Robot& _robot;
  std::unique_ptr<BlockWorldFilter> _blockFilter;
  
  
  WorldObject _blocks[kBlockCount];
  
  

};

  


} // Audio
} // Cozmo
} // Anki



#endif /* __Cozmo_Basestation_AudioWorldObjects_H__ */
