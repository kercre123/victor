/**
 * File: fileManagement.cpp
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "anki/common/basestation/utils/fileManagement.h"
#include "util/logging/logging.h"

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>


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

  bool MakeDir(const char* dir) {
    if(mkdir(dir, S_IRWXU) != 0) {
      return false;
    }
    return true;
  }
  
  
  // Reads formatted line from file (like fscanf), but automatically skips blank
  // lines and lines starting with the comment character '#'. Comments can also be
  // placed on the same line after non-comment text.
  int ReadLine(FILE* stream, const char * format, ...) {
    va_list vargs;
    char fbuf[1024];
    char *commentChar;
    char *spaceCheckChar;
    bool commentOnlyLine = true;
    
    memset(fbuf,0, 1024);
    
    while (commentOnlyLine) {
      // Read line including \n
      if ( NULL == fgets(fbuf, sizeof(fbuf), stream) )
      {
        //feof
        return -1;
      }
      
      // Look for comment character
      commentChar = strchr(fbuf, '#');
      
      // If comment character exists, end the string at the comment char.
      if (commentChar) {
        *commentChar = 0;
      }
      
      // Check if the line is empty
      spaceCheckChar = fbuf;
      while (isspace(*spaceCheckChar)) {
        spaceCheckChar++;
      }
      if (*spaceCheckChar != 0) {
        // Line has non-whitespace char so it's valid for formatted read.
        commentOnlyLine = false;
      }
    }
    
    // Read formatted string
    va_start(vargs, format);
    int res = vsscanf(fbuf, format, vargs);
    va_end(vargs);
    
    return res;
  }
  
  
  // Gets a list of all files in a directory
  //
  // dir: Directory to get list for
  // files: Vector to store file names in
  //
  // Returns true on success, false on failure
  bool GetFilesInDir(string dir, vector<string> &files, char *containsStr)
  {
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
      PRINT_NAMED_ERROR("General.GetFilesInDir", "Error %d while opening %s", errno, dir.c_str());
      return false;
    }
    
    while ((dirp = readdir(dp)) != NULL) {
      // If a substring specified, check to make sure that it occurs somewhere inside of the directory
      if(containsStr == NULL || strstr(dirp->d_name, containsStr) != NULL) {
        files.push_back(string(dirp->d_name));
      }
    }
    closedir(dp);
    return true;
  }


  
} // namespace Anki

