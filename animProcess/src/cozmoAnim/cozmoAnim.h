/*
 * File:          cozmoAnim.h
 * Date:          6/26/2017
 * Author:        Kevin Yoon
 *
 * Description:   A platform-independent container for spinning up all the pieces
 *                required to run Cozmo Animation Process.
 *
 */

#ifndef ANKI_COZMO_ANIM_H
#define ANKI_COZMO_ANIM_H

#include "json/json.h"

#include "anki/common/types.h"

namespace Anki {
  
// Forward declaration:
namespace Util {
namespace Data {
  class DataPlatform;
}
}
  
namespace Cozmo {
  
// Forward declarations
class CozmoAnimContext;
class AnimationStreamer;
  
class CozmoAnimEngine
{
public:

  CozmoAnimEngine(Util::Data::DataPlatform* dataPlatform);
  ~CozmoAnimEngine();

  Result Init();

  // Hook this up to whatever is ticking the game "heartbeat"
  Result Update(const BaseStationTime_t currTime_nanosec);

protected:
  
  bool               _isInitialized = false;
  
  std::unique_ptr<CozmoAnimContext>       _context;
  std::unique_ptr<AnimationStreamer>  _animationStreamer;
  
}; // class CozmoAnimEngine
  

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ANIM_H
