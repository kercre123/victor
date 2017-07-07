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
#include "util/signals/simpleSignal_fwd.h"

#include "anki/common/types.h"
//#include "anki/types.h"


#include <memory>


namespace Anki {
  
  // Forward declaration:
  namespace Util {
  namespace Data {
    class DataPlatform;
  }
  }
  
namespace Cozmo {
  
// Forward declarations
//class CozmoContext;
class RobotDataLoader;  // TODO: Rename to AssetLoader?  (Anims and sounds)
  
class CozmoAnim
{
public:

  CozmoAnim(Util::Data::DataPlatform* dataPlatform);
  ~CozmoAnim();

  Result Init(const Json::Value& config);

  // Hook this up to whatever is ticking the game "heartbeat"
  Result Update(const BaseStationTime_t currTime_nanosec);

protected:
  
  bool               _isInitialized = false;
  Json::Value        _config;

  virtual Result InitInternal();
  
  std::unique_ptr<RobotDataLoader> _assetLoader;
  
}; // class CozmoAnim
  

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ANIM_H
