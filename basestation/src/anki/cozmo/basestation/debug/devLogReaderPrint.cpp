/**
* File: devLogReaderPrint
*
* Author: Lee Crippen
* Created: 6/24/2016
*
* Description: Functionality for pulling Print data out of a log file
*
* Copyright: Anki, inc. 2016
*
*/
#include "anki/cozmo/basestation/debug/devLogReaderPrint.h"

namespace Anki {
namespace Cozmo {
  
bool DevLogReaderPrint::FillLogData(std::ifstream& fileHandle, LogData& logData_out) const
{
  // Try to read in the timestamp
  fileHandle >> logData_out._timestamp_ms;
  if (!fileHandle.good())
  {
    return false;
  }
  
  static constexpr auto kMaxLineLength = 1024;
  static char nextLineBuffer[kMaxLineLength];
  
  // get entry which tells us size of line
  std::streamsize lineSize;
  fileHandle >> lineSize;
  
  // reading an extra space and discarding it
  fileHandle.read(nextLineBuffer, 1);
  fileHandle.read(nextLineBuffer, lineSize);
  if (!fileHandle.good())
  {
    return false;
  }
  
  const auto lineLength = fileHandle.gcount();
  
  // We want to include an extra character in the vector for null
  logData_out._data.resize(lineLength + 1);
  
  std::copy(nextLineBuffer, nextLineBuffer + lineLength, reinterpret_cast<char*>(logData_out._data.data()));
  
  // Put in a null character at the end of the line we just copied with the extra space we created above
  logData_out._data[lineLength] = '\0';
  
  return true;
}

} // end namespace Cozmo
} // end namespace Anki
