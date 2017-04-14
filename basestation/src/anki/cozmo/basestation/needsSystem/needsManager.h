/**
 * File: needsManager
 *
 * Author: Paul Terry
 * Created: 04/12/2017
 *
 * Description: Manages the Needs for a Cozmo Robot
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_NeedsSystem_NeedsManager_H__
#define __Cozmo_Basestation_NeedsSystem_NeedsManager_H__

#include "anki/common/types.h"
#include "anki/cozmo/basestation/needsSystem/needsConfig.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "util/graphEvaluator/graphEvaluator2d.h" // For json; todo find a better way to get it
#include "util/signals/simpleSignal_fwd.h"
#include <assert.h>

namespace Anki {
namespace Cozmo {


namespace ExternalInterface {
  class MessageGameToEngine;
}
  
class Robot;


class NeedsManager
{
public:
  explicit NeedsManager(Robot* inRobot = nullptr);
  
  void Init(const Json::Value& inJson);
  
  void Update(float currentTime_s);
  
  void SetPaused(bool paused);
  
  bool GetPaused() const { return _isPaused; };
  
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);
  
private:
  
  Robot*      _robot;
  float       _lastUpdateTime;
  
  NeedsState  _needsState;
  NeedsConfig _needsConfig;
  
  bool        _isPaused;
  float       _timeLastPaused;  //??
  
  std::vector<Signal::SmartHandle> _signalHandles;
};
  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_NeedsSystem_NeedsManager_H__

