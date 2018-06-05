//
//  DAS.cpp
//  BaseStation
//
//  Created by damjan stulic on 1/2/13.
//  Copyright (c) 2013 Anki. All rights reserved.
//

#include "DAS.h"
#include "DASPrivate.h"
#include "dasLogMacros.h"
#include "dasGameLogAppender.h"
#include "dasGlobals.h"
#include "dasLocalAppender.h"
#include "dasLocalAppenderFactory.h"
#include "dasLogFileAppender.h"
#include "dasAppender.h"
#include "dasFilter.h"
#include "dasPostToServer.h"
#include "stringUtils.h"
#include "json/json.h"
#include <cassert>
#include <mutex>
#include <atomic>
#include <chrono>
#include <unordered_set>
#include <sstream>
#include <pthread.h>
#include <stdarg.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(NDEBUG)
int DASNetworkingDisabled = 0; // default to networking enabled
static bool sPrintGlobalsAndData = false;
#else
int DASNetworkingDisabled = 0xffff; // disable it for debug
static bool sPrintGlobalsAndData = true;
#endif

#if defined(NDEBUG) || defined(UNIT_TEST)
int DAS_AssertionEnabled = 0;
#else
int DAS_AssertionEnabled = 1;
#endif

static bool DASNeedsInit = true;
static bool sNeedToRotateGlobalsLogs = true;
static std::string sGameLogDir;
static std::string sDasLogDir;

static DASLogLevel sLocalMinLogLevel = DASLogLevel_Info;
static DASLogLevel sRemoteMinLogLevel = DASLogLevel_Event;
static DASLogLevel sGameMinLogLevel = DASLogLevel_Info;
static Anki::Das::DasLocalAppender* sLocalAppender =
  Anki::Das::DasLocalAppenderFactory::CreateAppender(DASLogMode_Both);

static std::unique_ptr<Anki::Das::DasAppender> sRemoteAppender = nullptr;
static std::mutex sRemoteAppenderMutex;
static std::unique_ptr<Anki::Das::DasGameLogAppender> sGameLogAppender = nullptr;
static std::mutex sGameLogAppenderMutex;

using DASGlobals = std::map<std::string, std::string>;

static DASGlobals sDASGlobals;
static std::atomic<uint32_t> sDASGlobalsRevisionNumber = ATOMIC_VAR_INIT(0);
static pthread_key_t sDASGlobalsRevisionNumberKey;
static pthread_key_t sDASGlobalsThreadCopyKey;
static std::mutex sDASGlobalsMutex;

// Local Log Filtering
static Anki::DAS::DASFilter sDASLevelOverride;
static std::mutex sDASLevelOverrideMutex;

// Thread tagging
static std::atomic<Anki::Das::ThreadId_t> sDASThreadMax {0};
static pthread_key_t sDASThreadIdKey;

// Save the DAS globals for crash reporting
static const std::string kLastRunDasGlobalsFileName = "lastRun.dasGlobals";
static const std::string kThisRunDasGlobalsFileName = "thisRun.dasGlobals";
static std::string pathToThisRunDasGlobalsFile;
static std::string pathToLastRunDasGlobalsFile;

static DASArchiveFunction sLogFileArchiveFunc = DASArchiveFunction{};
static DASUnarchiveFunction sLogFileUnarchiveFunc = DASUnarchiveFunction{};
static std::string sLogFileArchiveExtension = "";

void _setThreadLocalUInt32(pthread_key_t key, uint32_t value) {
  uint32_t* p = (uint32_t *) pthread_getspecific(key);
  if (nullptr == p) {
    p = (uint32_t *) malloc(sizeof(*p));
  }
  *p = value;
  (void) pthread_setspecific(key, p);
}

uint32_t _getThreadLocalUInt32(pthread_key_t key, uint32_t defaultValue) {
  uint32_t* p = (uint32_t *) pthread_getspecific(key);
  if (nullptr == p) {
    _setThreadLocalUInt32(key, defaultValue);
    p = (uint32_t *) pthread_getspecific(key);
  }
  return *p;
}


void _freeRevisionNumberKey(void* arg) {
  free(arg);
}

void _setThreadLocalCopyOfDASGlobals(DASGlobals* value) {
  DASGlobals* p = (DASGlobals *) pthread_getspecific(sDASGlobalsThreadCopyKey);
  if (nullptr != p) {
    delete p; p = nullptr;
  }
  p = value;
  (void) pthread_setspecific(sDASGlobalsThreadCopyKey, p);
}

DASGlobals* _getThreadLocalCopyOfDASGlobals() {
  DASGlobals* p = (DASGlobals *) pthread_getspecific(sDASGlobalsThreadCopyKey);
  if (nullptr == p) {
    p = new DASGlobals;
    _setThreadLocalCopyOfDASGlobals(p);
  }
  if (_getThreadLocalUInt32(sDASGlobalsRevisionNumberKey, 0) < sDASGlobalsRevisionNumber) {
    std::lock_guard<std::mutex> lock(sDASGlobalsMutex);

    p->clear();

    for (const auto& kv : sDASGlobals) {
      p->emplace(kv.first, kv.second);
    }
    _setThreadLocalUInt32(sDASGlobalsRevisionNumberKey, sDASGlobalsRevisionNumber);
  }
  return p;
}

void _freeDASGlobals(void* arg) {
  DASGlobals* p = (DASGlobals *) arg;
  delete p;
}

static Anki::Das::ThreadId_t _DASThreadId() {
  Anki::Das::ThreadId_t* p = (Anki::Das::ThreadId_t*)pthread_getspecific(sDASThreadIdKey);
  if(nullptr == p) {
    p = new Anki::Das::ThreadId_t {++sDASThreadMax};
    pthread_setspecific(sDASThreadIdKey, (void*)p);
  }
  assert(nullptr != p);
  return *p;
}

static void _freeDASThreadId(void* arg) {
  Anki::Das::ThreadId_t* p = (Anki::Das::ThreadId_t*) arg;
  delete p;
}

DASLogLevel DASLogLevelNameToEnum(const std::string& logLevelName) {
  DASLogLevel logLevel = DASLogLevel_NumLevels;

  if (AnkiUtil::StringCaseInsensitiveEquals(logLevelName, "debug")) {
    logLevel = DASLogLevel_Debug;
  } else if (AnkiUtil::StringCaseInsensitiveEquals(logLevelName, "info")) {
    logLevel = DASLogLevel_Info;
  } else if (AnkiUtil::StringCaseInsensitiveEquals(logLevelName, "event")) {
    logLevel = DASLogLevel_Event;
  } else if (AnkiUtil::StringCaseInsensitiveEquals(logLevelName, "warn")) {
    logLevel = DASLogLevel_Warn;
  } else if (AnkiUtil::StringCaseInsensitiveEquals(logLevelName, "error")) {
    logLevel = DASLogLevel_Error;
  }

  return logLevel;
}

void getDASLogLevelName(DASLogLevel logLevel, std::string& outLogLevelName) {
  switch(logLevel) {
  case DASLogLevel_Debug:
    outLogLevelName = "debug";
    break;
  case DASLogLevel_Info:
    outLogLevelName = "info";
    break;
  case DASLogLevel_Event:
    outLogLevelName = "event";
    break;
  case DASLogLevel_Warn:
    outLogLevelName = "warn";
    break;
  case DASLogLevel_Error:
    outLogLevelName = "error";
    break;
  default:
    outLogLevelName = "";
    break;
  }
}

void getDASTimeString(std::string& outTimeString) {
  std::chrono::system_clock::time_point now {std::chrono::system_clock::now()};
  std::time_t nowTime{std::chrono::system_clock::to_time_t(now)};
  char timeStr[16];
  std::chrono::milliseconds ms =
    std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
  size_t timeStrLen = std::strftime(timeStr, sizeof(timeStr),
                                    "%H:%M:%S", std::localtime(&nowTime));
  assert(timeStrLen > 0);
  size_t remainingLen = sizeof(timeStr) - timeStrLen;
  int printfResult __attribute((unused)) = snprintf(timeStr + timeStrLen, remainingLen,
                                                    ":%03llu", ms.count() % 1000);
  assert(printfResult <= remainingLen);
  outTimeString = timeStr;
}

void DASConfigure(const char* configurationJsonFilePath,
                  const char* logDirPath,
                  const char* gameLogDirPath) {
  if (nullptr != gameLogDirPath && '\0' != *gameLogDirPath) {
    sGameLogDir = gameLogDirPath;
  }

  if (nullptr != logDirPath && '\0' != *logDirPath) {
    sDasLogDir = logDirPath;
    (void) mkdir(logDirPath, S_IRWXU);
    pathToLastRunDasGlobalsFile = sDasLogDir + "/" + kLastRunDasGlobalsFileName;
    pathToThisRunDasGlobalsFile = sDasLogDir + "/" + kThisRunDasGlobalsFileName;
    if (sNeedToRotateGlobalsLogs) {
      // Rotate the saved DAS globals for crash reporting
      (void) rename(pathToThisRunDasGlobalsFile.c_str(), pathToLastRunDasGlobalsFile.c_str());
      sNeedToRotateGlobalsLogs = false;
    }
  }

  if (!DASNeedsInit) {
    DASClose();
  }

  if (nullptr != configurationJsonFilePath) {
    // load configuration
    std::string config = AnkiUtil::StringFromContentsOfFile(configurationJsonFilePath);
    DASClient::Json::Value root;
    DASClient::Json::Reader reader;
    bool parsingSuccessful = reader.parse(config, root);
    if (parsingSuccessful) {
      DASLogLevel localMinLogLevel = DASLogLevelNameToEnum(root["dasConfig"]
                                                           .get("localMinLogLevel", "info")
                                                           .asString());
      sDASLevelOverride.SetRootLogLevel(localMinLogLevel);
      const DASClient::Json::Value& localOverrides = root["dasConfig"].get("localMinLogLevelOverrides",
                                                                DASClient::Json::objectValue);
      for (const std::string& key : localOverrides.getMemberNames()) {
        DASLogLevel levelForKey = DASLogLevelNameToEnum(localOverrides
                                                        .get(key, DASClient::Json::stringValue).asString());
        sDASLevelOverride.SetMinLogLevel(key, levelForKey);
      }

      sRemoteMinLogLevel = DASLogLevelNameToEnum(root["dasConfig"]
                                                 .get("remoteMinLogLevel", "event").asString());
      sGameMinLogLevel = DASLogLevelNameToEnum(root["dasConfig"]
                                               .get("gameMinLogLevel", "info").asString());
      std::string url = root["dasConfig"].get("url", "").asString();
      {
        std::lock_guard<std::mutex> lock(sRemoteAppenderMutex);
        uint flush_interval = Anki::Das::DasAppender::kDefaultFlushIntervalSeconds;
        if( root["dasConfig"].isMember("flushInterval") )
        {
          flush_interval = root["dasConfig"].get("flushInterval", DASClient::Json::uintValue).asUInt();
        }

        size_t maxLogLength = Anki::Das::kDefaultMaxLogLength;
        if( root["dasConfig"].isMember("maxLogLength") )
        {
          maxLogLength = root["dasConfig"].get("maxLogLength", DASClient::Json::uintValue).asUInt();
        }

        size_t maxLogFiles = Anki::Das::kDasDefaultMaxLogFiles;
        if( root["dasConfig"].isMember("maxLogFiles") )
        {
          maxLogFiles = root["dasConfig"].get("maxLogFiles", DASClient::Json::uintValue).asUInt();
        }

        sRemoteAppender.reset(new Anki::Das::DasAppender(sDasLogDir, url,flush_interval,
                                                         maxLogLength, maxLogFiles,
                                                         sLogFileArchiveFunc,
                                                         sLogFileUnarchiveFunc,
                                                         sLogFileArchiveExtension));
      }
    } else {
      LOGD("Failed to parse configuration: %s", reader.getFormattedErrorMessages().c_str());
    }
  }

  if (DASNeedsInit) {
    (void) pthread_key_create(&sDASGlobalsRevisionNumberKey, _freeRevisionNumberKey);
    (void) pthread_key_create(&sDASGlobalsThreadCopyKey, _freeDASGlobals);
    (void) pthread_key_create(&sDASThreadIdKey, _freeDASThreadId);
  }
  DASNeedsInit = false;
}

const char* DASGetLogDir()
{
  return sDasLogDir.c_str();
}

static void _DAS_DestroyRemoteAppender() {
  std::lock_guard<std::mutex> lock(sRemoteAppenderMutex);
  sRemoteAppender.reset();
}

static void _DAS_DestroyGameLogAppender() {
  std::lock_guard<std::mutex> lock(sGameLogAppenderMutex);
  if (nullptr != sGameLogAppender) {
    sGameLogAppender->flush();
  }
  sGameLogAppender.reset();
}

void SetDASLocalLoggerMode(DASLocalLoggerMode logMode)
{
  Anki::Das::DasLocalAppender* appender;
  appender = Anki::Das::DasLocalAppenderFactory::CreateAppender(logMode);

  Anki::Das::DasLocalAppender* oldAppender = sLocalAppender;
  sLocalAppender = appender;
  if (nullptr != oldAppender) {
    oldAppender->flush();
    delete oldAppender;
  }
}

void DASClose() {
  _DAS_DestroyRemoteAppender();
  _DAS_DestroyGameLogAppender();
  if (nullptr != sLocalAppender) {
    sLocalAppender->flush();
    delete sLocalAppender;
  }
}

void DASForceFlushNow() {
  if (nullptr != sRemoteAppender) {
    sRemoteAppender->ForceFlush();
  }
}

void DASForceFlushWithCallback(const DASFlushCallback& callback) {
  if (nullptr != sRemoteAppender) {
    sRemoteAppender->ForceFlushWithCallback(callback);
  }
  else if (callback) {
    callback(false, "");
  }
}

void DASPauseUploadingToServer(const bool isPaused) {
  if (nullptr != sRemoteAppender) {
    sRemoteAppender->SetIsUploadingPaused(isPaused);
  }
}

void DASSetArchiveLogConfig(const DASArchiveFunction& archiveFunction, const DASUnarchiveFunction& unarchiveFunction, const std::string& archiveExtension)
{
  sLogFileArchiveFunc = archiveFunction;
  sLogFileUnarchiveFunc = unarchiveFunction;
  sLogFileArchiveExtension = archiveExtension;
}

int _DAS_IsEventEnabledForLevel(const char* eventName, DASLogLevel level) {
  if (DASNeedsInit) {
    DASConfigure(nullptr, nullptr, nullptr);
  }
  if (level >= sRemoteMinLogLevel || level >= sGameMinLogLevel
      || level >= _DAS_GetLevel(eventName, sLocalMinLogLevel)) {
    return 1;
  }

  return 0;
}

void _DAS_LogInternal(DASLogLevel level, const char* eventName, const char* eventValue,
  const char* file, const char* funct, int line,
  std::map< std::string, std::string > & data)
{
  // Scream if the client passes a null or empty event name and replace it with "noname"
  // If a "noname" event is seen on the server, open a bug
  if (nullptr == eventName || '\0' == *eventName) {
    LOGD("DAS ERROR: EVENT WITH A NULL OR BLANK NAME PASSED (value=%s file=%s funct=%s line=%d)",
      (eventValue ? eventValue : ""),
      (file ? file : ""),
      (funct ? funct : ""),
      line);
    eventName = "noname";
  }

  std::string logLevel;
  getDASLogLevelName(level, logLevel);
  data[Anki::Das::kMessageLevelGlobalKey] = logLevel;
  Anki::Das::ThreadId_t threadId {_DASThreadId()};

  DASGlobals* globals = _getThreadLocalCopyOfDASGlobals();
  if (globals->end() == globals->find(Anki::Das::kTimeStampGlobalKey)
    && data.end() == data.find(Anki::Das::kTimeStampGlobalKey)) {
    std::chrono::milliseconds milliSecondsSince1970 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    std::string ts = std::to_string(milliSecondsSince1970.count());
    data[Anki::Das::kTimeStampGlobalKey] = ts;
  }

  {
    if (level >= _DAS_GetLevel(eventName, sLocalMinLogLevel)) {
      std::string trimmedEventValue = eventValue;
      AnkiUtil::StringTrimWhitespaceFromEnd(trimmedEventValue);
      std::string globalsAndData;
      if (sPrintGlobalsAndData) {
        getDASGlobalsAndDataString(globals, data, globalsAndData);
      }
      sLocalAppender->append(level, eventName, trimmedEventValue.c_str(),
                            threadId, file, funct, line, globals, data, globalsAndData.c_str());
    }
  }
  if (level >= sRemoteMinLogLevel) {
    std::lock_guard<std::mutex> lock(sRemoteAppenderMutex);
    if (nullptr != sRemoteAppender) {
      sRemoteAppender->append(level, eventName, eventValue, threadId, file, funct, line,
                              globals, data);
    }
  }
  if (level >= sGameMinLogLevel) {
    std::lock_guard<std::mutex> lock(sGameLogAppenderMutex);
    if (nullptr != sGameLogAppender) {
      sGameLogAppender->append(level, eventName, eventValue, threadId, file, funct, line,
                               globals, data);
    }
  }
}

void _DAS_LogKv(DASLogLevel level, const char* eventName, const char* eventValue,
  const std::vector< std::pair< const char*, const char* > > & keyValues)
{
  std::map<std::string,std::string> data;
  for (const auto& keyValuePair : keyValues) {
    data.emplace(std::string(keyValuePair.first), std::string(keyValuePair.second));
  }
  _DAS_LogInternal(level, eventName, eventValue, "", "", 0, data);
}

void _DAS_LogKvMap(DASLogLevel level, const char* eventName, const char* eventValue,
  const std::map<std::string, std::string>& keyValues)
{
  std::map<std::string,std::string> data = keyValues;
  _DAS_LogInternal(level, eventName, eventValue, "", "", 0, data);
}

void _DAS_Log(DASLogLevel level, const char* eventName, const char* eventValue,
              const char* file, const char* funct, int line, ...)
{
  std::map<std::string,std::string> data;
  va_list argList;
  va_start(argList, line);
  const char* key = nullptr;
  while (nullptr != (key = va_arg(argList, const char *))) {
    const char* value;
    value = va_arg(argList, const char *);
    if (nullptr == value) break;
    data.erase(key);
    data.emplace(key, value);
  }
  va_end(argList);

  _DAS_LogInternal(level, eventName, eventValue, file, funct, line, data);
}

void _DAS_SetGlobal(const char* key, const char* value) {
  std::lock_guard<std::mutex> lock(sDASGlobalsMutex);

  // Create and destroy the game log appender with the setting
  // and clearing of the $game global
  if (0 == strcmp(Anki::Das::kGameIdGlobalKey, key)) {
    if (nullptr != value && '\0' != value[0]) {
      std::lock_guard<std::mutex> lg(sGameLogAppenderMutex);
      sGameLogAppender.reset(new Anki::Das::DasGameLogAppender(sGameLogDir, value));
    } else {
      _DAS_DestroyGameLogAppender();
    }
  }

  sDASGlobals.erase(key);
  if (nullptr != value && '\0' != value[0]) {
    sDASGlobals.emplace(key, value);
  }
  sDASGlobalsRevisionNumber++;
  (void) AnkiUtil::StoreStringMapInFile(pathToThisRunDasGlobalsFile, sDASGlobals);
}

void _DAS_GetGlobal(const char* key, std::string& outValue) {
  std::lock_guard<std::mutex> lock(sDASGlobalsMutex);

  auto it = sDASGlobals.find(key);
  if (sDASGlobals.end() != it) {
    outValue = it->second;
  } else {
    outValue = "";
  }
}
void _DAS_GetGlobalsForThisRun(std::map<std::string, std::string>& dasGlobals) {
  std::lock_guard<std::mutex> lock(sDASGlobalsMutex);
  dasGlobals = sDASGlobals;
}

void _DAS_GetGlobalsForLastRun(std::map<std::string, std::string>& dasGlobals) {
  dasGlobals.clear();
  dasGlobals = AnkiUtil::JsonFileToStringMap(pathToLastRunDasGlobalsFile);
}

size_t _DAS_GetGlobalsForLastRunAsJsonString(char* buffer, size_t len) {
  if (buffer && len) {
    buffer[0] = '\0';
  }
  std::string dasGlobalsFromLastRunAsJson =
    AnkiUtil::StringFromContentsOfFile(pathToLastRunDasGlobalsFile);
  size_t requiredLen = dasGlobalsFromLastRunAsJson.size() + 1;
  if (buffer && len >= requiredLen) {
    std::copy(dasGlobalsFromLastRunAsJson.begin(), dasGlobalsFromLastRunAsJson.end(), buffer);
    buffer[dasGlobalsFromLastRunAsJson.size()] = '\0';
  }
  return requiredLen;
}


void _DAS_ReportCrashForLastAppRun(const char* apprun) {
  if (DASNetworkingDisabled) {
    return;
  }
  DASGlobals dasGlobalsFromLastRun;
  _DAS_GetGlobalsForLastRun(dasGlobalsFromLastRun);
  auto search = dasGlobalsFromLastRun.find(Anki::Das::kApplicationRunGlobalKey);
  if (search == dasGlobalsFromLastRun.end()) {
    return;
  }
  if (apprun && *apprun && search->second != std::string(apprun)) {
    return;
  }

  DASLogLevel level = DASLogLevel_Error;
  std::string logLevel;
  getDASLogLevelName(level, logLevel);
  dasGlobalsFromLastRun[Anki::Das::kMessageLevelGlobalKey] = logLevel;

  // Use a sequence number high enough that it will be after all of the DAS events that
  // were reported during the apprun
  dasGlobalsFromLastRun[Anki::Das::kSequenceGlobalKey] = "999999999";

  // We don't know when the crash occurred, so just use the current time
  std::chrono::milliseconds milliSecondsSince1970 =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
  std::string ts = std::to_string(milliSecondsSince1970.count());
  dasGlobalsFromLastRun[Anki::Das::kTimeStampGlobalKey] = ts;
  {
    std::lock_guard<std::mutex> lock(sRemoteAppenderMutex);
    if (!sRemoteAppender) {
      return;
    }
    sRemoteAppender->append(level, "crash", "", dasGlobalsFromLastRun);
  }
  DASEvent("reported_crash", "prior apprun = %s", (search->second).c_str());
}

void _DAS_ClearGlobals() {
  std::lock_guard<std::mutex> lock(sDASGlobalsMutex);

  sDASGlobals.clear();
  _DAS_DestroyGameLogAppender();

  sDASGlobalsRevisionNumber++;
  (void) AnkiUtil::StoreStringMapInFile(pathToThisRunDasGlobalsFile, sDASGlobals);
}

// _DAS_GetGameLogAppender is used by the unit tests. Don't delete!
Anki::Das::DasGameLogAppender* _DAS_GetGameLogAppender() {
  return sGameLogAppender.get();
}

void _DAS_Logf(DASLogLevel level, const char* eventName, const char* eventValue,
               const char* file, const char* funct, int line, ...) {
  char renderedLogString[2048];
  va_list argList;
  va_start(argList, line);
  vsnprintf(renderedLogString, sizeof(renderedLogString), eventValue, argList);
  va_end(argList);
  _DAS_Log(level, eventName, renderedLogString, file, funct, line, NULL);
}

void DASEnableNetwork(DASDisableNetworkReason reason) {
  DASNetworkingDisabled &= ~(reason);
}

void DASDisableNetwork(DASDisableNetworkReason reason) {
  DASNetworkingDisabled |= reason;
}

int DASGetNetworkingDisabled() {
  return DASNetworkingDisabled;
}

void _DAS_SetLevel(const char* eventName, DASLogLevel level) {
  std::lock_guard<std::mutex> lock(sDASLevelOverrideMutex);
  sDASLevelOverride.SetMinLogLevel(eventName, level);
  return;
}

DASLogLevel _DAS_GetLevel(const char* eventName, DASLogLevel defaultLevel) {
  std::lock_guard<std::mutex> lock(sDASLevelOverrideMutex);
  return sDASLevelOverride.GetMinLogLevel(eventName);
}

void _DAS_ClearSetLevels() {
  std::lock_guard<std::mutex> lock(sDASLevelOverrideMutex);
  sDASLevelOverride.ClearLogLevels();
}

void getDASGlobalsAndDataString(const std::map<std::string,std::string>* globals,
                                const std::map<std::string,std::string>& data,
                                std::string& outGlobalsAndData)
{
  outGlobalsAndData = "";
  static const std::unordered_set<std::string> excludedKeys =
    {
      Anki::Das::kApplicationVersionGlobalKey,
      Anki::Das::kApplicationRunGlobalKey,
      Anki::Das::kMessageVersionGlobalKey,
      Anki::Das::kMessageLevelGlobalKey,
      Anki::Das::kPhoneTypeGlobalKey,
      Anki::Das::kPlatformGlobalKey,
      Anki::Das::kProductGlobalKey,
      Anki::Das::kTimeStampGlobalKey,
      Anki::Das::kUnitIdGlobalKey
    };

  std::map<std::string,std::string> dict;
  for (const auto& kv : *globals) {
    if (!excludedKeys.count(kv.first) && strcmp(kv.first.c_str(), Anki::Das::kUserIdGlobalKey)) {
      dict[kv.first] = kv.second;
    }
  }
  for (const auto& kv : data) {
    if (!excludedKeys.count(kv.first)) {
      dict[kv.first] = kv.second;
    }
  }

  std::ostringstream oss;
  bool first = true;
  oss << "{ ";
  for (const auto& kv : dict) {
    if (first) {
      first = false;
    } else {
      oss << ", ";
    }
    oss << kv.first << " = " << kv.second;
  }

  if (!first) {
    oss << " }";
    outGlobalsAndData = oss.str();
  }
}

#if defined(VICOS)
bool PostToServer(const std::string & url, const std::string & body, std::string & response)
{
  return dasPostToServer(url, body, response);
}
#endif

#ifdef __cplusplus
} // extern "C"
#endif
