//
//  consoleChannelStdout.h
//  BaseStation
//
//  Created by Brian Chapados on 4/27/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#ifndef ANKIUTIL_CONSOLE_CHANNEL_STDOUT_H
#define ANKIUTIL_CONSOLE_CHANNEL_STDOUT_H

#include "stdio.h"
#include "util/console/consoleChannel.h"

namespace Anki {
namespace Util {

class ConsoleChannelFile : public IConsoleChannel {
public:
  // Default constructor
  ConsoleChannelFile();

  // The destructor will automatically cleans up
  virtual ~ConsoleChannelFile();

  // Returns true if the channel can receive data
  bool IsOpen();

  // Write data
  int WriteData(uint8_t* buffer, int len);

  // Write formatted log string
  int WriteLogv(const char* format, va_list args);

  int WriteLog(const char* format, ...);

  // Flush buffered data
  bool Flush() {
    return false;
  }

private:
  void HexDump(FILE* f, int width, uint8_t* buf, int len);
};

}
}

#endif /* defined(__BaseStation__consoleChannelStdout__) */
