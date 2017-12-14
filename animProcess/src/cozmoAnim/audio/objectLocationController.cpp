/**
 * File: objectLocationController.cpp
 *
 * Author: Jordan Rivas
 * Created: 09/07/2017
 *
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "cozmoAnim/audio/cozmoAudioController.h"
#include "cozmoAnim/audio/objectLocationController.h"
#include "audioEngine/audioTypeTranslator.h"


#include "clad/audio/audioPositionMessage.h"

#include "anki/common/basestation/math/rotation.h"

//#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/matrix_impl.h"

//#include "anki/common/basestation/utils/data/dataPlatform.h"
//#include "audioEngine/audioScene.h"
//#include "audioEngine/soundbankLoader.h"
//#include "clad/audio/audioGameObjectTypes.h"
//#include "cozmoAnim/cozmoAnimContext.h"
//#include "util/console/consoleInterface.h"
//#include "util/environment/locale.h"
//#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
//#include "util/math/numericCast.h"
//#include "util/time/universalTime.h"
//#include <sstream>
//

#include <unordered_map>


namespace {
using namespace Anki::Cozmo::Audio;
using namespace Anki::AudioMetaData;
static const std::unordered_map< Anki::Cozmo::Audio::WorldObjectType,
                                 Anki::AudioMetaData::GameObjectType,
                                 Anki::Util::EnumHasher > WorldObjectMap
{
  { WorldObjectType::NoObject,  GameObjectType::Invalid },
  { WorldObjectType::Robot,     GameObjectType::Cozmo_OnDevice },
  { WorldObjectType::Block_1,   GameObjectType::Block_1 },
  { WorldObjectType::Block_2,   GameObjectType::Block_2 },
  { WorldObjectType::Block_3,   GameObjectType::Block_3 },
  { WorldObjectType::Charger,   GameObjectType::Charger },
};
}



namespace Anki {
namespace Cozmo {
namespace Audio {



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ObjectLocationController
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectLocationController::ObjectLocationController( CozmoAudioController& audioController )
: _audioController(audioController)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectLocationController::ObjectLocationControllerInit()
{
  // Set Robot as listner and init position
  const auto robotGameObj = AudioEngine::ToAudioGameObject(AudioMetaData::GameObjectType::Cozmo_OnDevice);
  _audioController.SetListener( robotGameObj );
  AudioEngine::AudioPosition position;
  position.position.x = position.position.y = position.position.z = 0.0f;
  position.orientationFront.z = 1.0f;
  position.orientationFront.y = position.orientationFront.x = 0.0f;
  position.orientationTop.y = 1.0f;
  position.orientationTop.x = position.orientationTop.z = 0.0f;
  _audioController.SetPosition( robotGameObj, position );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectLocationController::HandleMessage(const Anki::Cozmo::Audio::AddRemoveWorldObject& msg)
{
  printf("ObjectLocationController::HandleMessage(const Anki::Cozmo::Audio::AddRemoveWorldObject& msg)\n");
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectLocationController::HandleMessage(const Anki::Cozmo::Audio::UpdateWorldObjectPosition& msg)
{
//  printf("ObjectLocationController::HandleMessage(const Anki::Cozmo::Audio::UpdateWorldObjectPosition& msg)\n");
  
  const auto& it = WorldObjectMap.find( msg.worldObject );
  if ( it == WorldObjectMap.end() ) {
    PRINT_NAMED_ERROR("ObjectLocationController.HandleMessage", "Can NOT find World Object Type %s",
                      EnumToString( msg.worldObject ));
    return;
  }
 
  // Translate to wwise coordinate system
  // From Right hand to Left
  AudioEngine::AudioPosition position;
  position.position.z = msg.xPos;
  position.position.x = -msg.yPos;
  position.position.y = 0.0f;
  
  // Translate orientation to vector
  RotationMatrix2d matrix(msg.orientationRad);
  Vec2f vector(1.f, 0.f);
  Vec2f result = matrix * vector;
//  result__.ToString();
  
  position.orientationFront.z = result.x();
  position.orientationFront.x = -result.y();
  position.orientationFront.y = 0.0f;
  position.orientationTop.y = 1.0f;
  position.orientationTop.x = position.orientationTop.z = 0.0f;
  
  _audioController.SetPosition( AudioEngine::ToAudioGameObject(it->second), position );
}


} // Audio
} // Cozmo
} // Anki
