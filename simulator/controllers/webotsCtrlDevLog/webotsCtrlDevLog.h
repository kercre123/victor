/**
* File: webotsCtrlDevLog
*
* Author: Lee Crippen
* Created: 6/21/2016
*
* Description: Webots controller for loading and displaying Cozmo dev logs
*
* Copyright: Anki, inc. 2016
*
*/
#ifndef __Simulator_Controllers_WebotsCtrlDevLog_H_
#define __Simulator_Controllers_WebotsCtrlDevLog_H_

#include "anki/cozmo/basestation/debug/devLogReader.h"

#include <cstdint>
#include <memory>
#include <vector>
#include <set>

namespace webots {
  class Supervisor;
  class Node;
}

class UdpClient;

namespace Anki {
namespace Cozmo {
  
class DevLogProcessor;

class WebotsDevLogController
{
public:
  WebotsDevLogController(int32_t stepTime_ms);
  virtual ~WebotsDevLogController();
  
  std::string GetDirectoryPath() const;
  void InitDevLogProcessor(const std::string& directoryPath);
  
  // Set save images state
  void EnableSaveImages(bool enable);
  
  int32_t Update();
  
private:
  int32_t _stepTime_ms;
  std::unique_ptr<webots::Supervisor> _supervisor;
  std::unique_ptr<DevLogProcessor>    _devLogProcessor;
  std::unique_ptr<UdpClient>          _vizConnection;
  std::set<int>                       _lastKeysPressed;
  webots::Node*                       _selfNode;
  bool                                _savingImages;
  
  void HandleVizData(const DevLogReader::LogData& logData);
  void HandlePrintLines(const DevLogReader::LogData& logData);
  void UpdateKeyboard();
  bool UpdatePressedKeys();
  void EnableSaveImagesIfChecked();
  
}; // class WebotsDevLogController
} // namespace Cozmo
} // namespace Anki

#endif  // __Simulator_Controllers_WebotsCtrlDevLog_H_
