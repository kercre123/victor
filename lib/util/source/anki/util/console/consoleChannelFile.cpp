//
//  consoleChannelStdout.cpp
//  BaseStation
//
//  Created by Brian Chapados on 4/27/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#include "consoleChannelFile.h"

#include <stdio.h>

namespace Anki {
namespace Util {

ConsoleChannelFile::ConsoleChannelFile() {

}

ConsoleChannelFile::~ConsoleChannelFile() {

}

bool ConsoleChannelFile::IsOpen() {
  return true;
}

void ConsoleChannelFile::HexDump(FILE* f, int width, uint8_t* buf, int len) {
  int i, n;

  for (i = 0, n = 1; i < len; i++, n++) {
    fprintf(f, "%2.2X ", buf[i]);
    if (n == width) {
      fprintf(f, "\n");
      n = 0;
    }
  }
  if (i && n != 1)
    fprintf(f, "\n");
}

int ConsoleChannelFile::WriteData(uint8_t* buffer, int len) {
  HexDump(stdout, 80, buffer, len);
  return len;
}

int ConsoleChannelFile::WriteLogv(const char* format, va_list args) {
  return vfprintf(stdout, format, args);
}

int ConsoleChannelFile::WriteLog(const char* format, ...) {
  va_list args;
  va_start(args, format);
  int result = WriteLogv(format, args);
  va_end(args);

  return result;
}

}
}