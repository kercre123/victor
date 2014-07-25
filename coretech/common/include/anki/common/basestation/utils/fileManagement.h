/**
 * File: fileManagement.h
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#ifndef BASESTATION_UTILS_FILEMGMNT_H_
#define BASESTATION_UTILS_FILEMGMNT_H_

//#include <time.h>

#include "anki/common/basestation/general.h"
#include "anki/common/types.h"

//#include <sys/time.h>

namespace Anki {
  
  bool DirExists(const char* dir);
  
} // namespace Anki

#endif // BASESTATION_UTILS_FILEMGMNT_H_
