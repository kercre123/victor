/**
 * File: fileManagement.h
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#ifndef BASESTATION_UTILS_FILEMGMNT_H_
#define BASESTATION_UTILS_FILEMGMNT_H_

#include <string>
#include <vector>

using namespace std;

namespace Anki {
  
  // Returns true if directory exists
  bool DirExists(const char* dir);
  
  // Creates directory
  bool MakeDir(const char* dir);
  
  // Reads formatted line from file (like fscanf), but automatically skips blank
  // lines and lines starting with the comment character '#'. Comments can also be
  // placed on the same line after non-comment text.
  int ReadLine(FILE* stream, const char * format, ...);
  
  
  // Gets a list of all files in a directory
  //
  // dir: Directory to get list for
  // files: where to store files
  bool GetFilesInDir(string dir, vector<string> &files, char *containsStr = NULL);
  
} // namespace Anki

#endif // BASESTATION_UTILS_FILEMGMNT_H_
