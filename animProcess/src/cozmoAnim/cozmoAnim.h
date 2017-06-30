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

//#include "util/logging/multiFormattedLoggerProvider.h"
//#include "anki/vision/basestation/image.h"
#include "json/json.h"
#include "util/signals/simpleSignal_fwd.h"

#include "anki/common/types.h"
//#include "anki/types.h"

//#include "clad/types/imageTypes.h"
//#include "clad/types/engineState.h"
//#include "anki/cozmo/basestation/debug/debugConsoleManager.h"
//#include "anki/cozmo/basestation/debug/dasToSdkHandler.h"
//#include "util/global/globalDefinitions.h"

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
class CozmoContext;
  
//template <typename Type>
//class AnkiEvent;


class CozmoAnim
{
public:

  CozmoAnim(Util::Data::DataPlatform* dataPlatform);
  virtual ~CozmoAnim();


  Result Init(const Json::Value& config);

  // Hook this up to whatever is ticking the game "heartbeat"
  Result Update(const BaseStationTime_t currTime_nanosec);

  
  // Handle various message types
//  template<typename T>
//  void HandleMessage(const T& msg);

protected:
  
  //std::vector<::Signal::SmartHandle> _signalHandles;
  
  bool                                                      _isInitialized = false;
  Json::Value                                               _config;

  virtual Result InitInternal();
  
//  void SetEngineState(EngineState newState);
  
//  EngineState _engineState = EngineState::Stopped;
  
}; // class CozmoEngine
  

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ANIM_H
