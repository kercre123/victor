/**
* File: devLogProcessor
*
* Author: Lee Crippen
* Created: 6/22/2016
*
* Description: Functionality for processing dev logs
*
* Copyright: Anki, inc. 2016
*
*/
#ifndef __Cozmo_Basestation_Debug_DevLogProcessor_H_
#define __Cozmo_Basestation_Debug_DevLogProcessor_H_

#include "util/helpers/noncopyable.h"
#include "anki/cozmo/basestation/debug/devLogReader.h"

#include <string>
#include <cstdint>
#include <functional>
#include <vector>
#include <deque>

namespace Anki {
namespace Cozmo {
  
class DevLogReaderRaw;
class DevLogReaderPrint;

class DevLogProcessor : Util::noncopyable {
public:
  DevLogProcessor(const std::string& directory);
  virtual ~DevLogProcessor();
  
  const std::string& GetDirectoryName() const { return _directoryName; }
  
  // Move forward in time by number of milliseconds specified. Can trigger callbacks if they have been set
  // Returns whether there is more data in the logs to process
  bool AdvanceTime(uint32_t time_ms);
  
  void SetVizMessageCallback(DevLogReader::DataCallback callback);
  void SetPrintCallback(DevLogReader::DataCallback callback);
  
private:
  std::string                         _directoryName;
  std::unique_ptr<DevLogReaderRaw>    _vizMessageReader;
  std::unique_ptr<DevLogReaderPrint>  _printReader;
};

} // end namespace Cozmo
} // end namespace Anki


#endif //__Cozmo_Basestation_Debug_DevLogProcessor_H_
