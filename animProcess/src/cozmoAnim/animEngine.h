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
  namespace Cozmo {
    class AnimContext;
    class AnimationStreamer;
    class TextToSpeechComponent;
  }
}
namespace Anki {
  namespace Cozmo {
    namespace RobotInterface {
      struct TextToSpeechPrepare;
      struct TextToSpeechDeliver;
      struct TextToSpeechCancel;
    }
  }
}
namespace Anki {
  namespace Util {
    namespace Data {
      class DataPlatform;
    }
  }
}

namespace Anki {
namespace Cozmo {

class AnimEngine
{
public:

  AnimEngine(Util::Data::DataPlatform* dataPlatform);
  ~AnimEngine();

  Result Init();

  // Hook this up to whatever is ticking the game "heartbeat"
  Result Update(BaseStationTime_t currTime_nanosec);

  // Message handlers
  void HandleMessage(const RobotInterface::TextToSpeechPrepare& msg);
  void HandleMessage(const RobotInterface::TextToSpeechDeliver& msg);
  void HandleMessage(const RobotInterface::TextToSpeechCancel& msg);

protected:

  bool               _isInitialized = false;

  std::unique_ptr<AnimContext>      _context;
  std::unique_ptr<AnimationStreamer>     _animationStreamer;
  std::unique_ptr<TextToSpeechComponent> _ttsComponent;

}; // class AnimEngine


} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ANIM_ENGINE_H
