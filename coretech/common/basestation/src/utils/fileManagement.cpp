/**
 * File: fileManagement.cpp
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "anki/common/basestation/utils/fileManagement.h"

#include <sys/stat.h>

namespace Anki {

  bool DirExists(const char *dir) {
    // Check if directory exists
    struct stat info;
    if( stat( dir, &info ) != 0 ) {
      // Not accessible
      return false;
    }
    if (!S_ISDIR(info.st_mode)) {
      // Not a directory
      return false;
    }
    
    return true;
  }

} // namespace Anki

