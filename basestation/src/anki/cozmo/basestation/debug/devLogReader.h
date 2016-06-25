/**
* File: devLogReader
*
* Author: Lee Crippen
* Created: 6/22/2016
*
* Description: Functionality for pulling data out of a log file
*
* Copyright: Anki, inc. 2016
*
*/
#ifndef __Cozmo_Basestation_Debug_DevLogReader_H_
#define __Cozmo_Basestation_Debug_DevLogReader_H_

#include "util/helpers/noncopyable.h"

#include <string>
#include <cstdint>
#include <functional>
#include <vector>
#include <deque>
#include <fstream>

namespace Anki {
namespace Cozmo {

class DevLogReader: Util::noncopyable {
public:
  DevLogReader(const std::string& directory);
  virtual ~DevLogReader() { }
  
  const std::string& GetDirectoryName() const { return _directory; }
  
  // Move forward in time by number of milliseconds specified. Can trigger callbacks if they have been set
  // Returns whether there is more data in the logs to process
  bool AdvanceTime(uint32_t timestep_ms);
  
  struct LogData
  {
    uint32_t              _timestamp_ms = 0;
    std::vector<uint8_t>  _data;
    
    bool IsValid() const { return !_data.empty(); }
  };
  
  using DataCallback = std::function<void(const LogData&)>;
  void SetDataCallback(DataCallback callback) { _dataCallback = callback; }
  
protected:
  virtual bool FillLogData(std::ifstream& fileHandle, LogData& logData_out) const = 0;
  
private:
  std::string             _directory;
  DataCallback            _dataCallback;
  std::deque<std::string> _files;
  uint32_t                _totalTime = 0;
  std::ifstream           _currentLogFileHandle;
  LogData                 _currentLogData;
  
  void DiscoverLogFiles();
  bool UpdateForCurrentTime(uint32_t time_ms);
  bool ExtractAndCallback(uint32_t time_ms);
  bool CheckTimeAndCallback(uint32_t time_ms);
};

} // end namespace Cozmo
} // end namespace Anki


#endif //__Cozmo_Basestation_Debug_DevLogReader_H_
