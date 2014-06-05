/**
 * File: general.h
 *
 * Author: Boris Sofman (boris)
 * Created: 6/11/2008
 * 
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 * 
 * Description: Generally useful funcitons, defines, and macros.
 *
 * Copyright: Anki, Inc. 2011
 *
 **/


#ifndef BASESTATION_GENERAL_GENERAL_H_
#define BASESTATION_GENERAL_GENERAL_H_


//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string>
#include <vector>

//#include <math.h>
//#include <stdio.h>
//#include <iostream>
//#include <sys/types.h>
#include <errno.h>
//#include <iostream>
//#include <limits.h>
//#include <float.h>
//#include <assert.h>

#include "anki/common/constantsAndMacros.h"

//#ifndef FAKE_DAS
//#include <DAS/DAS.h>
//#else
//#include "basestation/DAS.h"
//#endif
//#include "basestation/utils/debug.h"

// for BOUNDED_WHILE exception
//#include "basestation/utils/exceptions.h"

//#define BASESTATION_EXPORT __attribute__((visibility("default")))

// Let's not pollute our namespaces unnecessarily.
// Use this only where needed, if at all.
//// Using std namespace for easier coding
//using namespace std;


namespace Anki {

///////////////////////////////////////////////////////////////////////////////
// Paths to relevant files
///////////////////////////////////////////////////////////////////////////////

// Path to directory containing all road network map files (from
// basestation root path)
#define ROAD_MAP_FILES_PATH "/basestation/config/mapFiles/"

//////////////////////////////////////////////////////////////////////////////
// TYPE DEFINITIONS
//////////////////////////////////////////////////////////////////////////////
typedef unsigned long long int BaseStationTime_t;

typedef enum {
  PR_MODE_NONE, // don't do playback or recording
  PR_MODE_PLAYBACK, // read playback information from file
  PR_MODE_RECORD // write playback information to file
} PLAYBACK_AND_RECORDING_MODE;



///////////////////////////////////////////////////////////////////
// MACROS
//////////////////////////////////////////////////////////////////

// macro hacking stuff
#define CONCAT_IMPL( x, y ) x##y
#define MACRO_CONCAT( x, y ) CONCAT_IMPL( x, y )

// a while loop that executes a limited number of execution (throws exception)
// TODO:(bn) throw exception instead of assert
#define MAKE_NAME MACRO_CONCAT(_bvar_, __LINE__)
#define BOUNDED_WHILE(n, exp) unsigned int MAKE_NAME=0; while(MAKE_NAME++ > (n) ? assert("infinite loop!" == nullptr), false : (exp))

// global error flag so we can check if PRINT_ERROR was called for unit testing
extern bool _errG;
 

///////////////////////////////////////////////////////////////////
// DAS MACROS
//////////////////////////////////////////////////////////////////

//   DEBUG: For general debugging. Does not go to DAS.
//    INFO: What's the diff with DEBUG?
//   EVENT: For general info that does go to DAS.
// WARNING: For things that shouldn't happen, but might and we don't want to stop the app because of it. Goes to DAS.
//   ERROR: For things that definitely shouldn't happen. Goes to DAS.
  
#ifdef USE_REAL_DAS
  
// Print error message to stderr. First flushes stdout to make sure print order
// is correct.
// #define PRINT_ERROR(format, ...) do{ _errG=true; DASErrorAuto(format, ##__VA_ARGS__); }while(0)
// #define PRINT_WARNING(format, ...) do{DASWarnAuto(format, ##__VA_ARGS__);}while(0)
#define PRINT_INFO(format, ...) do{DASInfoAuto(format, ##__VA_ARGS__);}while(0)
#define PRINT_DEBUG(format, ...) do{DASDebugAuto(format, ##__VA_ARGS__);}while(0)
#define PRINT_DAS_BY_TYPE(type, format, ...) do{ switch(type) { case DAS_CATEGORY_DEBUG: PRINT_DEBUG(format,##__VA_ARGS__); break; \
                                                                case DAS_CATEGORY_INFO: PRINT_INFO(format,##__VA_ARGS__); break; \
                                                                case DAS_CATEGORY_WARNING: PRINT_WARNING(format,##__VA_ARGS__); break; \
                                                                case DAS_CATEGORY_ERROR: PRINT_ERROR(format,##__VA_ARGS__); break; \
                                                                default: break; }}while(0)

// Logging with names. This is preferable to the PRINT_* messages above
#define PRINT_NAMED_ERROR(name, format, ...) do{ _errG=true; DASError(name, format, ##__VA_ARGS__); }while(0)
#define PRINT_NAMED_WARNING(name, format, ...) do{DASWarn(name, format, ##__VA_ARGS__);}while(0)
#define PRINT_NAMED_INFO(name, format, ...) do{DASInfo(name, format, ##__VA_ARGS__);}while(0)
#define PRINT_NAMED_DEBUG(name, format, ...) do{DASDebug(name, format, ##__VA_ARGS__);}while(0)

#else 
  
#define PRINT_INFO(format, ...) do{ printf(format, ##__VA_ARGS__); }while(0)
#define PRINT_DEBUG(format, ...) do{ printf(format, ##__VA_ARGS__); }while(0)
#define PRINT_DAS_BY_TYPE(type, format, ...)
#define PRINT_NAMED_ERROR(name, format, ...) do{_errG=true; printf("ERROR: " name " - "); printf(format, ##__VA_ARGS__); }while(0)
#define PRINT_NAMED_WARNING(name, format, ...) do{ printf("WARNING: " name " - "); printf(format, ##__VA_ARGS__); }while(0)
#define PRINT_NAMED_INFO(name, format, ...) do{ printf("INFO: " name " - "); printf(format, ##__VA_ARGS__); }while(0)
#define PRINT_NAMED_DEBUG(name, format, ...) do{ printf("DEBUG: " name " - "); printf(format, ##__VA_ARGS__); }while(0)
  
#endif // USE_REAL_DAS

  
//////////////////////////////////////////////////////////////////
// HASHING HELPERS
//////////////////////////////////////////////////////////////////

// used to create hashes for reproducibility.
#define HASHING_VALUE 19
  
#define ADD_HASH(v, n) AddHash((v), (n), (#n))
  
// Functions for hashing. updates value by hashing in the given newValue
void AddHash(unsigned int& value, const unsigned int newValue, const char* str = "");
void AddHash(unsigned int& value, const int newValue, const char* str = "");
void AddHash(unsigned int& value, const short newValue, const char* str = "");
void AddHash(unsigned int& value, const unsigned short newValue, const char* str = "");
void AddHash(unsigned int& value, const char newValue, const char* str = "");
void AddHash(unsigned int& value, const unsigned char newValue, const char* str = "");

void AddHash(unsigned int& value, const float newValue, const char* str = "");
void AddHash(unsigned int& value, const double newValue, const char* str = "");

  
//////////////////////////////////////////////////////////////////
// FILE HELPERS
//////////////////////////////////////////////////////////////////
  
// Reads formatted line from file (like fscanf), but automatically skips blank
// lines and lines starting with the comment character '#'. Comments can also be
// placed on the same line after non-comment text.
int ReadLine(FILE* stream, const char * format, ...);

// Gets a list of all files in a directory
//
// dir: Directory to get list for
// files: where to store files
bool GetFilesInDir(std::string dir, std::vector<std::string> &files, char *containsStr = NULL);
  
  
//////////////////////////////////////////////////////////////////
// COMPARISON HELPERS
//////////////////////////////////////////////////////////////////
  
// Compare functions for pairs based on first or second element only
template < class X , class Y >
struct CompareFirst  : public std::binary_function<std::pair<X,Y>, std::pair<X,Y>, bool>
{
  bool operator() (const std::pair<X,Y>& a, const std::pair<X,Y>& b) {
    return a.first < b.first;
  }
};

template < class X , class Y >
struct CompareSecond  : public std::binary_function<std::pair<X,Y>, std::pair<X,Y>, bool>
{
  bool operator() (const std::pair<X,Y>& a, const std::pair<X,Y>& b) {
    return a.second < b.second;
  }
};

  
///////////////////////////////////////////////////////////////////
// PRINT HELPERS
///////////////////////////////////////////////////////////////////
void PrintBytesHex(const char* bytes, int num_bytes);

void PrintBytesUInt(const char* bytes, int num_bytes);
  

// Makes a beep (for debugging?)
void SystemBeep();

} // namespace Anki

#endif // BASESTATION_GENERAL_GENERAL_H_

