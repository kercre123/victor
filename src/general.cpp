/**
 * File: general.c
 *
 * Author: Boris Sofman (boris)
 * Created: 6/29/2008
 * 
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 * 
 * Description: Generally useful funcitons, defines, and macros.
 *
 * Copyright: Anki, Inc. 2008-2011
 *
 **/

#include "anki/general.h"
#include <stdarg.h>
#include <string.h>

namespace Anki {

bool _errG = false;

bool _DEBUG_REPRO_HAS_ENABLED;
/*
bool _checkReproHash()
{
  if(_DEBUG_REPRO_HAS_ENABLED)
    return true;
  return (_DEBUG_REPRO_HAS_ENABLED =
              BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() >= DEBUG_REPRO_HASH_ENABLE_TIME_S);
}

#define CHECK_REPRO_HASH (DEBUG_REPRO_HASH && _checkReproHash())
*/
#define CHECK_REPRO_HASH 0
  
  
void _AddHash(unsigned int& value, const unsigned int newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    static unsigned long long int numHashes;
    printf("hv: %08X [hash#%8llu] item \"%s\"\n", newValue, numHashes, str);
    numHashes++;
  }

  value = value * HASHING_VALUE + newValue;
}


void AddHash(unsigned int& value, const unsigned int newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("iv: %35u ", newValue);
  }
  _AddHash(value, newValue, str);
}

void AddHash(unsigned int& value, const int newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("iv: %+35d ", newValue);
  }
  _AddHash(value, newValue, str);
}

void AddHash(unsigned int& value, const short newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("iv: %+35d ", newValue);
  }
  _AddHash(value, newValue, str);
}

void AddHash(unsigned int& value, const unsigned short newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("iv: %35d ", newValue);
  }
  _AddHash(value, newValue, str);
}

void AddHash(unsigned int& value, const char newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("iv: %+35d ", newValue);
  }
  _AddHash(value, newValue, str);
}

void AddHash(unsigned int& value, const unsigned char newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("iv: %35u ", newValue);
  }
  _AddHash(value, newValue, str);
}


void AddHash(unsigned int& value, const float newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("fv: %+35.26e ", newValue);
  }

  union {
    float floatValue;
    unsigned int uintValue;
  } floatConverter;

  floatConverter.floatValue = newValue;
  _AddHash(value, floatConverter.uintValue, str);
}

void AddHash(unsigned int& value, const double newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("dv: %+35.26e ", newValue);
  }

  union {
    double doubleValue;
    unsigned int uintValue;
  } doubleConverter;

  doubleConverter.doubleValue = newValue;
  _AddHash(value, doubleConverter.uintValue, str);
}


int ReadLine(FILE* stream, const char * format, ...) {
  va_list vargs;
  char fbuf[1024];
  char *commentChar;
  char *spaceCheckChar;
  bool commentOnlyLine = true;
     
  while (commentOnlyLine) {
    // Read line including \n
    fgets(fbuf, sizeof(fbuf), stream);

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

void SystemBeep()
{
  printf("\a");
}

} // namespace BaseStation


