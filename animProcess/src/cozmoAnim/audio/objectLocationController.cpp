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
#include "clad/types/robotStatusAndActions.h"

#include "anki/common/basestation/math/rotation.h"

#include "anki/common/basestation/math/matrix_impl.h"

#include "anki/cozmo/shared/cozmoConfig.h"

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

namespace {
  const auto kRobotGameObj = AudioEngine::ToAudioGameObject(AudioMetaData::GameObjectType::Cozmo_OnDevice);
}

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
  _audioController.SetListener( kRobotGameObj );
  AudioEngine::AudioPosition position;
  position.position.x = position.position.y = position.position.z = 0.0f;
  position.orientationFront.z = 1.0f;
  position.orientationFront.y = position.orientationFront.x = 0.0f;
  position.orientationTop.y = 1.0f;
  position.orientationTop.x = position.orientationTop.z = 0.0f;
  _audioController.SetPosition( kRobotGameObj, position );
  
  // Start Robot Movement Event
  const auto robotMovEvent = AudioEngine::ToAudioEventId(AudioMetaData::GameEvent::GenericEvent::Play__Sfx__Robot_Movement);
  _audioController.PostAudioEvent(robotMovEvent, kRobotGameObj);
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
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectLocationController::ProcessRobotState(const RobotState& stateMsg)
{
  float driveSpeed = (stateMsg.lwheel_speed_mmps + stateMsg.rwheel_speed_mmps) / 2;
  float turnSpeed =(stateMsg.lwheel_speed_mmps - stateMsg.rwheel_speed_mmps);
  
  static const double radToAngle = 57.295779513082321;
  static float prevHeadAng = stateMsg.headAngle;
  static float prevLiftAng = stateMsg.liftAngle;
  
  float headAngDif = stateMsg.headAngle - prevHeadAng;
  float liftAngDif = stateMsg.liftAngle - prevLiftAng;
  prevHeadAng = stateMsg.headAngle;
  prevLiftAng = stateMsg.liftAngle;
  
  printf("\nRobotState: L_Wheel %f R_Wheel %f Speed %f  Turn Speed %f \nHeadAng %f angDiff %f --- LiftAng %f angDiff %f\n",
         stateMsg.lwheel_speed_mmps, stateMsg.rwheel_speed_mmps, driveSpeed, turnSpeed,
         stateMsg.headAngle * radToAngle, headAngDif * radToAngle,
         stateMsg.liftAngle * radToAngle, liftAngDif * radToAngle);
//  stateMsg.lwheel_speed_mmps
  
  using namespace AudioEngine;
  using namespace AudioMetaData::GameParameter;
  static const auto driveSpeedId = ToAudioParameterId(ParameterType::Drive_Speed);
  static const auto turnSpeedId = ToAudioParameterId(ParameterType::Turn_Speed);
  static const auto headAngleId = ToAudioParameterId(ParameterType::Head_Angle);
  static const auto headSpeedId = ToAudioParameterId(ParameterType::Head_Speed);
  static const auto liftAngleId = ToAudioParameterId(ParameterType::Lift_Angle);
  static const auto liftSpeedId = ToAudioParameterId(ParameterType::Lift_Speed);
  
  
  _audioController.SetParameter(driveSpeedId, driveSpeed, kInvalidAudioGameObject);
  _audioController.SetParameter(turnSpeedId, turnSpeed, kInvalidAudioGameObject);
  
  _audioController.SetParameter(headAngleId, (stateMsg.headAngle * radToAngle), kInvalidAudioGameObject);
  _audioController.SetParameter(headSpeedId, (headAngDif * radToAngle), kInvalidAudioGameObject);
  _audioController.SetParameter(liftAngleId, (stateMsg.liftAngle * radToAngle), kInvalidAudioGameObject);
  _audioController.SetParameter(liftSpeedId, (liftAngDif * radToAngle), kInvalidAudioGameObject);
  
  
}
  


} // Audio
} // Cozmo
} // Anki
