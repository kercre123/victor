/**
 * File: fileManagement.h
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#ifndef BASESTATION_UTILS_FILEMGMNT_H_
#define BASESTATION_UTILS_FILEMGMNT_H_


namespace Anki {
  
  bool DirExists(const char* dir);
  bool MakeDir(const char* dir);
  
} // namespace Anki

#endif // BASESTATION_UTILS_FILEMGMNT_H_
