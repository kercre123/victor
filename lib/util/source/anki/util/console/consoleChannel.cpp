//
//  consolePort.c
//  BaseStation
//
//  Created by Brian Chapados on 4/22/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#include "consoleChannel.h"

#include <stdio.h>

bool AnkiConsoleChannelIsOpen(Anki::Util::IConsoleChannel* channel)
{
  return channel->IsOpen();
}

int AnkiConsoleChannelWriteData(Anki::Util::IConsoleChannel* channel, uint8_t *buffer, size_t len)
{
  return channel->WriteData(buffer, (int)len);
}

int AnkiConsoleChannelWriteLog(Anki::Util::IConsoleChannel* channel, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  int result = channel->WriteLogv(format, ap);
  va_end(ap);
  return result;
}

namespace Anki {
namespace Util {

int IConsoleChannel::WriteLog(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int result = WriteLogv(format, ap);
  va_end(ap);
  return result;
}

}
}
