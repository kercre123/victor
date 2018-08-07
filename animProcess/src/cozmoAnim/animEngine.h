/*
 * File:          cozmoAnim/animEngine.h
 * Date:          6/26/2017
 * Author:        Kevin Yoon
 *
 * Description:   A platform-independent container for spinning up all the pieces
 *                required to run Cozmo Animation Process.
 *
 */

#ifndef ANKI_COZMO_ANIM_ENGINE_H
#define ANKI_COZMO_ANIM_ENGINE_H

#include "json/json.h"

#include "coretech/common/shared/types.h"

// Forward declarations
namespace Anki {
  namespace Vector {
    class AnimContext;
    class AnimationStreamer;
    class StreamingAnimationModifier;
    class TextToSpeechComponent;

    namespace Audio {
      class CozmoAudioController;
      class MicrophoneAudioClient;
    } // Audio
    namespace RobotInterface {
      struct SetLocale;
      struct TextToSpeechPrepare;
      struct TextToSpeechDeliver;
      struct TextToSpeechPlay;
      struct TextToSpeechCancel;
    } // RobotInterface
  } // Cozmo
  namespace Util {
    namespace Data {
      class DataPlatform;
    }
  } // Util
} // Anki

namespace Anki {
namespace Vector {

class AnimEngine
{
public:

  AnimEngine(Util::Data::DataPlatform* dataPlatform);
  ~AnimEngine();

  Result Init();

  // Hook this up to whatever is ticking the game "heartbeat"
  Result Update(BaseStationTime_t currTime_nanosec);

  // Message handlers
  void HandleMessage(const RobotInterface::SetLocale& msg);
  void HandleMessage(const RobotInterface::TextToSpeechPrepare& msg);
  void HandleMessage(const RobotInterface::TextToSpeechDeliver& msg);
  void HandleMessage(const RobotInterface::TextToSpeechPlay& msg);
  void HandleMessage(const RobotInterface::TextToSpeechCancel& msg);

protected:

  bool                                              _isInitialized = false;
  std::unique_ptr<AnimContext>                      _context;
  std::unique_ptr<AnimationStreamer>                _animationStreamer;
  std::unique_ptr<StreamingAnimationModifier>       _streamingAnimationModifier;
  std::unique_ptr<TextToSpeechComponent>            _ttsComponent;
  std::unique_ptr<Audio::MicrophoneAudioClient>     _microphoneAudioClient;
  Audio::CozmoAudioController*                      _audioControllerPtr = nullptr;

}; // class AnimEngine


} // namespace Vector
} // namespace Anki

#endif // ANKI_COZMO_ANIM_ENGINE_H
