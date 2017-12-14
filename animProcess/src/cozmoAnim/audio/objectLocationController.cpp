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
  
  // Translate to wwise coordinate system
  // From Right hand to Left
  AudioEngine::AudioPosition position;
  position.position.z = msg.xPos;
  position.position.x = -msg.yPos;
  position.position.y = 0.0f;
  
  // Translate orientation to vector
  const RotationMatrix2d matrix(msg.orientationRad);
  const Vec2f vector(1.f, 0.f);
  const Vec2f result = matrix * vector;
  position.orientationFront.z = result.x();
  position.orientationFront.x = -result.y();
  position.orientationFront.y = 0.0f;
  position.orientationTop.y = 1.0f;
  position.orientationTop.x = position.orientationTop.z = 0.0f;
  auto gameObj = (AudioMetaData::GameObjectType::Invalid == msg.gameObject) // Properly interpret invalid type
                 ? AudioEngine::kInvalidAudioGameObject
                 : AudioEngine::ToAudioGameObject(msg.gameObject);
  
  _audioController.SetPosition( gameObj, position );
  
  // Also update the Cozmo_OnDevice as the listener position
  if (msg.gameObject == AudioMetaData::GameObjectType::Cozmo_OnDevice) {
    _audioController.SetPosition( AudioEngine::kInvalidAudioGameObject, position );
  }
}


} // Audio
} // Cozmo
} // Anki
