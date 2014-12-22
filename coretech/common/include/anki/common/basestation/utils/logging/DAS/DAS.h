//
//  DAS.h
//  TestLogging
//
//  Created by Mark Pauley on 8/13/12.
//  Copyright (c) 2012 Mark Pauley. All rights reserved.
//

#ifndef __DAS_H__
#define __DAS_H__

#include <string.h>
#include <stdio.h>

#pragma mark DASLogLevel
typedef enum DASLogLevel {
  DASLogLevel_Debug, // Sent to server in development by default
  DASLogLevel_Info,  // Sent to server in development by default
  DASLogLevel_Event, // Sent to server in production by default
  DASLogLevel_Warn,  // Sent to server in production by default
  DASLogLevel_Error  // Sent to server in production by default
} DASLogLevel;

#ifdef ANKI_SERVER_BUILD
#define LOG_LEVEL DASLogLevel_Warn
#else
#define LOG_LEVEL DASLogLevel_Debug
#endif
#pragma mark DAS Macros

// Plain old, vanilla logging. eventValue is a printf-style format specifier.  Defaults to INFO level.
// DASEvent("ClassName.eventName", "Value1 = %d, Value2 = %d", value1, value2); or something like that

#define DASDebug(eventName, eventValue_format, ...) do{if(_DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Debug)) {\
_DAS_Logf(DASLogLevel_Debug, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); }}while(0)

#define DASInfo(eventName, eventValue_format, ...)  do{if(_DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Info)) {\
_DAS_Logf(DASLogLevel_Info, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); }}while(0)

#define DASEvent(eventName, eventValue_format, ...) do{if(_DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Event)) {\
_DAS_Logf(DASLogLevel_Event, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); }}while(0)

#define DASWarn(eventName, eventValue_format, ...)  do{if(_DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Warn)) {\
_DAS_Logf(DASLogLevel_Warn, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); }}while(0)

#define DASError(eventName, eventValue_format, ...) do{if(_DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Error)) {\
_DAS_Logf(DASLogLevel_Error, eventName, (eventValue_format), __FILE__, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__); }}while(0)

// Does nothing in the basestation das version
#define DASSetLevel( ... )

// Streams
#define DASDebug_stream(eventName, args) do{                            \
if(1 || _DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Debug)) {     \
std::stringstream ss;                                             \
ss<<args;                                                         \
_DAS_Logf(DASLogLevel_Debug, eventName, ss.str().c_str(),         \
__FILE__, __PRETTY_FUNCTION__, __LINE__);           \
}}while(0)

#define DASInfo_stream(eventName, args) do{                            \
if(1 || _DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Info)) {     \
std::stringstream ss;                                             \
ss<<args;                                                         \
_DAS_Logf(DASLogLevel_Info, eventName, ss.str().c_str(),         \
__FILE__, __PRETTY_FUNCTION__, __LINE__);           \
}}while(0)

#define DASWarn_stream(eventName, args) do{                            \
if(1 || _DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Warn)) {     \
std::stringstream ss;                                             \
ss<<args;                                                         \
_DAS_Logf(DASLogLevel_Warn, eventName, ss.str().c_str(),         \
__FILE__, __PRETTY_FUNCTION__, __LINE__);           \
}}while(0)

#define DASError_stream(eventName, args) do{                            \
if(1 || _DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Error)) {     \
std::stringstream ss;                                             \
ss<<args;                                                         \
_DAS_Logf(DASLogLevel_Error, eventName, ss.str().c_str(),         \
__FILE__, __PRETTY_FUNCTION__, __LINE__);           \
}}while(0)

// Auto streams
#define DASDebugAuto_stream(args)do {                                   \
char eventNameBuf[256]; DAS_GENERATE_EVENT_NAME(eventNameBuf, 256); \
DASDebug_stream(eventNameBuf, args); }while(0)


#define DAS_SHORT_FILE ( strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__ )
#define DAS_GENERATE_EVENT_NAME(nameBuf, nameBufLen) { snprintf(nameBuf, nameBufLen, "%s.%s.%d", (DAS_SHORT_FILE), __FUNCTION__, __LINE__); }

// DAS auto-generated events, uses the filename, line and function
#define DASDebugAuto(eventValue_format, ...) do { \
char eventNameBuf[256]; DAS_GENERATE_EVENT_NAME(eventNameBuf, 256);\
DASDebug(eventNameBuf, (eventValue_format), ##__VA_ARGS__); }while(0)

#define DASInfoAuto(eventValue_format, ...) do { \
char eventNameBuf[256]; DAS_GENERATE_EVENT_NAME(eventNameBuf, 256);\
DASInfo(eventNameBuf, (eventValue_format), ##__VA_ARGS__); }while(0)

/* DASEventAuto considered harmful.
 *  Events are tracked by our back end analytics and need to have explicit names.
 */

#define DASWarnAuto(eventValue_format, ...) do { \
char eventNameBuf[256]; DAS_GENERATE_EVENT_NAME(eventNameBuf, 256);\
DASWarn(eventNameBuf, (eventValue_format), ##__VA_ARGS__); }while(0)

#define DASErrorAuto(eventValue_format, ...) do { \
char eventNameBuf[256]; DAS_GENERATE_EVENT_NAME(eventNameBuf, 256);\
DASError(eventNameBuf, (eventValue_format), ##__VA_ARGS__); }while(0)


// DASp(eventName, eventValue, key1, value1, key2, value2, ..., NULL)
// So, the simplest is DASp(eventName, eventValue, NULL);

#define DASDebugp(eventName, eventValue, ...) do{if(_DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Debug)) {\
_DAS_Log(DASLogLevel_Debug, eventName, (eventValue), __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__, NULL); }}while(0)

#define DASInfop(eventName, eventValue, ...)  do{if(_DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Info)) {\
_DAS_Log(DASLogLevel_Info, eventName, (eventValue), __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__, NULL); }}while(0)

#define DASEventp(eventName, eventValue, ...) do{if(_DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Event)) {\
_DAS_Log(DASLogLevel_Event, eventName, (eventValue), __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__, NULL); }}while(0)

#define DASWarnp(eventName, eventValue, ...)  do{if(_DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Warn)) {\
_DAS_Log(DASLogLevel_Warn, eventName, (eventValue), __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__, NULL); }}while(0)

#define DASErrorp(eventName, eventValue, ...) do{if(_DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Event)) {\
_DAS_Log(DASLogLevel_Error, eventName, (eventValue), __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__, NULL); }}while(0)


#pragma mark DAS_SetGlobal
// Sets a key / value globally on DAS.
// pass NULL for value to un-set a key.
#define DAS_SetGlobal(key, value) do {\
_DAS_SetGlobal(key, value);\
}while(0)\


#pragma mark DASAssert

#if (!defined(NDEBUG)) && !(defined(UNIT_TEST))
#define DASDEBUG_ABORT __builtin_trap()
#else
#define DASDEBUG_ABORT ((void)0)
#endif

#define DASAssert(exp) do{if(!(exp)){ DASErrorAuto("Assertion Failed: " #exp); DASDEBUG_ABORT;} }while(0)

#pragma mark Initialization

#ifdef __cplusplus
extern "C" {
#endif
  // Run this as soon as possible, passing in the log4cxx xml config file.
  void DASConfigure(const char* configurationXMLFilePath);
  void DASClose();
#ifdef __cplusplus
} // extern "C"
#endif


#pragma mark Implementation


#define DAS_FORMAT_FUNCTION(F,A) __attribute((format(printf, F, A)))

#ifdef __cplusplus
extern "C" {
#endif
  void _DAS_Logf(DASLogLevel level, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...);
  void _DAS_Log(DASLogLevel level, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...);
  void _DASSetLogFunction(int (*function)(const char *, ...));
  // void _DAS_SetGlobal(const char* key, const char* value);
  // int  _DAS_IsEventEnabledForLevel(const char* eventName, DASLogLevel level);
#ifdef __cplusplus
} // extern "C"
#endif

// for the basestation main, just use printf (for now)
#define _DAS_IsEventEnabledForLevel(eventName, level) (((DASLogLevel)level) >= LOG_LEVEL )
#define _DAS_SetGlobal(key, value)


#ifdef __OBJC__
// DAS GameLob upload / download / search headers.
// TODO: Put these back in
/*
#import "DASFileDownload.h"
#import "DASFileUpload.h"
#import "DASFileSearch.h"
 */
#endif


#endif /* defined(__DAS_H__) */
