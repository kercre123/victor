//
//  DAS.h
//  TestLogging
//
//  Created by Mark Pauley on 8/13/12.
//  Copyright (c) 2012 Mark Pauley. All rights reserved.
//

#ifndef __DAS_H__
#define __DAS_H__

#ifdef __cplusplus

#include <functional>
#include <memory>
#include <map>
#include <string>
#include <vector>

namespace DAS {
class IDASPlatform;
}
#endif

#pragma mark DASLogLevel

typedef enum DASLogLevel {
    DASLogLevel_Debug, // Sent to server in development by default
    DASLogLevel_Info, // Sent to server in development by default
    DASLogLevel_Event, // Sent to server in production by default
    DASLogLevel_Warn, // Sent to server in production by default
    DASLogLevel_Error, // Sent to server in production by default
    DASLogLevel_NumLevels
} DASLogLevel;

#pragma mark DASDisableNetworkReason

typedef enum DASDisableNetworkReason {
    DASDisableNetworkReason_Simulator = (1 << 0),
    DASDisableNetworkReason_UserOptOut = (1 << 1),
    DASDisableNetworkReason_Shutdown = (1 << 2),
    DASDisableNetworkReason_LogRollover = (1 << 3),
} DASDisableNetworkReason;

typedef enum DASLocalLoggerMode {
  DASLogMode_Normal,
  DASLogMode_System,
  DASLogMode_Both,
} DASLocalLoggerMode;


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

#define DASSetLevel(eventName, level) do{_DAS_SetLevel(eventName, level);}while(0)

// Streams
#define DASDebug_stream(eventName, args) do{                            \
    if(_DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Debug)) {     \
      std::stringstream ss;                                             \
      ss<<args;                                                         \
      _DAS_Logf(DASLogLevel_Debug, eventName, "%s", __FILE__, __PRETTY_FUNCTION__, __LINE__, ss.str().c_str()); \
    }}while(0)

#define DASInfo_stream(eventName, args) do{                             \
    if(_DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Info)) {      \
      std::stringstream ss;                                             \
      ss<<args;                                                         \
      _DAS_Logf(DASLogLevel_Info, eventName, "%s", __FILE__, __PRETTY_FUNCTION__, __LINE__, ss.str().c_str()); \
    }}while(0)

#define DASWarn_stream(eventName, args) do{                             \
    if(_DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Warn)) {      \
      std::stringstream ss;                                             \
      ss<<args;                                                         \
      _DAS_Logf(DASLogLevel_Warn, eventName, "%s", __FILE__, __PRETTY_FUNCTION__, __LINE__, ss.str().c_str()); \
    }}while(0)

#define DASError_stream(eventName, args) do{                            \
    if(_DAS_IsEventEnabledForLevel(eventName, DASLogLevel_Error)) {     \
      std::stringstream ss;                                             \
      ss<<args;                                                         \
      _DAS_Logf(DASLogLevel_Error, eventName, "%s", __FILE__, __PRETTY_FUNCTION__, __LINE__, ss.str().c_str()); \
    }}while(0)

// Auto streams
#define DASDebugAuto_stream(args) do{ \
    char eventNameBuf[256]; DAS_GENERATE_EVENT_NAME(eventNameBuf, sizeof(eventNameBuf)); \
    DASDebug_stream(eventNameBuf, args); }while(0)


#define DAS_SHORT_FILE ( strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__ )
#define DAS_GENERATE_EVENT_NAME(nameBuf, nameBufLen) { snprintf(nameBuf, nameBufLen, "%s.%s.%d", (DAS_SHORT_FILE), __FUNCTION__, __LINE__); }

// DAS auto-generated events, uses the filename, line and function
#define DASDebugAuto(eventValue_format, ...) do { \
  char eventNameBuf[256]; DAS_GENERATE_EVENT_NAME(eventNameBuf, sizeof(eventNameBuf));\
  DASDebug(eventNameBuf, (eventValue_format), ##__VA_ARGS__); }while(0)

#define DASInfoAuto(eventValue_format, ...) do { \
  char eventNameBuf[256]; DAS_GENERATE_EVENT_NAME(eventNameBuf, sizeof(eventNameBuf));\
  DASInfo(eventNameBuf, (eventValue_format), ##__VA_ARGS__); }while(0)

/* DASEventAuto considered harmful.
 *  Events are tracked by our back end analytics and need to have explicit names.
 */

#define DASWarnAuto(eventValue_format, ...) do { \
  char eventNameBuf[256]; DAS_GENERATE_EVENT_NAME(eventNameBuf, sizeof(eventNameBuf));\
  DASWarn(eventNameBuf, (eventValue_format), ##__VA_ARGS__); }while(0)

#define DASErrorAuto(eventValue_format, ...) do { \
  char eventNameBuf[256]; DAS_GENERATE_EVENT_NAME(eventNameBuf, sizeof(eventNameBuf));\
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

#ifdef __cplusplus
extern "C" {
#endif
extern int DAS_AssertionEnabled;
#ifdef __cplusplus
} // extern "C"
#endif


#if !(defined(UNIT_TEST))
#define DASDEBUG_ABORT __builtin_trap()
#else
#define DASDEBUG_ABORT ((void)0)
#endif

#define DASAssert(exp) do{if((DAS_AssertionEnabled == 1) && !(exp)){ DASErrorAuto("Assertion Failed: " #exp); DASDEBUG_ABORT;} }while(0)

#pragma mark Initialization

#ifdef __cplusplus
extern "C" {
#endif
// Run this as soon as possible, passing in the json config file and the paths for the logs and game lobs
void DASConfigure(const char* configurationJsonFilePath, const char* logDirPath, const char* gameLogDirPath) __attribute__((visibility("default")));
const char* DASGetLogDir() __attribute__((visibility("default")));
void DASClose() __attribute__((visibility("default")));

// Enable network for given reason
void DASEnableNetwork(DASDisableNetworkReason reason) __attribute__((visibility("default")));

// Disable network for given reason
void DASDisableNetwork(DASDisableNetworkReason reason) __attribute__((visibility("default")));

// Return bitmask of reasons network is disabled
int  DASGetNetworkingDisabled() __attribute__((visibility("default")));

void DASForceFlushNow() __attribute__((visibility("default")));

#ifdef __cplusplus
void DASNativeInit(std::unique_ptr<const DAS::IDASPlatform> platform, const char* product) __attribute__((visibility("default")));
const DAS::IDASPlatform* DASGetPlatform() __attribute__((visibility("default")));

using DASFlushCallback = std::function<void(bool,std::string)>; // passes in success/fail
void DASForceFlushWithCallback(const DASFlushCallback& callback) __attribute((visibility("default")));
void DASPauseUploadingToServer(const bool isPaused) __attribute((visibility("default")));


using DASArchiveFunction = std::function<bool(const std::string&)>;
using DASUnarchiveFunction = std::function<std::string(const std::string&)>;
void DASSetArchiveLogConfig(const DASArchiveFunction& archiveFunction,
                            const DASUnarchiveFunction& unarchiveFunction,
                            const std::string& archiveExtension) __attribute((visibility("default")));
#endif

void SetDASLocalLoggerMode(DASLocalLoggerMode logMode) __attribute__((visibility("default")));

#ifdef __cplusplus
} // extern "C"
#endif

#pragma mark Implementation

#define DAS_FORMAT_FUNCTION(F,A) __attribute((format(printf, F, A)))

#ifdef __cplusplus
extern "C" {
#endif
void _DAS_Logf(DASLogLevel level, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...) DAS_FORMAT_FUNCTION(3, 7) __attribute__((visibility("default")));
void _DAS_Log(DASLogLevel level, const char* eventName, const char* eventValue, const char* file, const char* funct, int line, ...) __attribute((sentinel))__attribute__((visibility("default")));
void _DAS_SetGlobal(const char* key, const char* value) __attribute__((visibility("default")));
int _DAS_IsEventEnabledForLevel(const char* eventName, DASLogLevel level) __attribute__((visibility("default")));
void _DAS_SetLevel(const char* eventName, DASLogLevel level) __attribute__((visibility("default")));
DASLogLevel _DAS_GetLevel(const char* eventName, DASLogLevel defaultLevel) __attribute__((visibility("default")));
void _DAS_ClearSetLevels() __attribute__((visibility("default")));

void _DAS_ReportCrashForLastAppRun(const char* apprun) __attribute__((visibility("default")));
size_t _DAS_GetGlobalsForLastRunAsJsonString(char* buffer, size_t len) __attribute__((visibility("default")));

#ifdef __cplusplus
void _DAS_GetGlobalsForLastRun(std::map<std::string, std::string>& dasGlobals) __attribute__((visibility("default")));
void _DAS_GetGlobalsForThisRun(std::map<std::string, std::string>& dasGlobals) __attribute__((visibility("default")));
void _DAS_LogKv(DASLogLevel level, const char* eventName, const char* eventValue,
  const std::vector< std::pair< const char*, const char* > > & keyValues) __attribute__((visibility("default")));
void _DAS_LogKvMap(DASLogLevel level, const char* eventName, const char* eventValue,
  const std::map<std::string, std::string>& keyValues) __attribute__((visibility("default")));
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* defined(__DAS_H__) */
