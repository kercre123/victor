/**
 * File: iPerfMetric.h
 *
 * Author: Paul Terry
 * Created: 11/03/2017
 *
 * Description: Lightweight performance metric recording: interface class
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Vector_iPerfMetric_H__
#define __Vector_iPerfMetric_H__

#include "util/global/globalDefinitions.h"
#include <string>
#include <queue>


// To enable PerfMetric in a build, define ANKI_PERF_METRIC_ENABLED as 1
#if !defined(ANKI_PERF_METRIC_ENABLED)
  #if ANKI_DEV_CHEATS
    #define ANKI_PERF_METRIC_ENABLED 1
  #else
    #define ANKI_PERF_METRIC_ENABLED 0
  #endif
#endif


namespace Anki {
namespace Vector {

namespace WebService {
  class WebService;
}

typedef enum
{
  DT_LOG,
  DT_RESPONSE_STRING,
  DT_FILE_TEXT,
  DT_FILE_CSV,
} DumpType;


class PerfMetric
{
public:
  explicit PerfMetric();
  virtual ~PerfMetric();

  virtual void Init(Util::Data::DataPlatform* dataPlatform, WebService::WebService* webService) = 0;
  virtual void Update(const float tickDuration_ms,
                      const float tickFrequency_ms,
                      const float sleepDurationIntended_ms,
                      const float sleepDurationActual_ms) = 0;
  void Start();

  WebService::WebService* GetWebService() { return _webService; }

  bool GetAutoRecord() const { return _autoRecord; }

  int  ParseCommands(std::string& queryString);
  void ExecuteQueuedCommands(std::string* resultStr = nullptr);

protected:
  void Status(std::string* resultStr) const;
  void Stop();
  virtual void Dump(const DumpType dumpType, const bool dumpAll,
                    const std::string* fileName = nullptr, std::string* resultStr = nullptr) const = 0;
  void DumpFiles() const;
  void WaitSeconds(const float seconds);
  void WaitTicks(const int ticks);
  void OnShutdown();
  void InitInternal(Util::Data::DataPlatform* dataPlatform, WebService::WebService* webService);
  void UpdateWaitMode();
  void RemoveOldFiles() const;
  bool FrameBufferEmpty() const { return _nextFrameIndex == 0 && !_bufferFilled; }

  struct FrameMetric
  {
    float _tickExecution_ms;
    float _tickTotal_ms;
    float _tickSleepIntended_ms;
    float _tickSleepActual_ms;
  };

  static const int kNumFramesInBuffer = 4000;

  int                 _nextFrameIndex = 0;
  bool                _bufferFilled = false;
  bool                _isRecording = false;
  bool                _autoRecord;
  bool                _waitMode = false;
  int                 _waitTicksRemaining = 0;
  float               _waitTimeToExpire = 0.0f;

  WebService::WebService* _webService;
  std::string         _fileDir = "";
  static const std::string _logBaseFileName;
  std::string         _fileNameSuffix = "";
  static const int    kNumCharsInLineBuffer = 256;
  char*               _lineBuffer;

  typedef enum
  {
    STATUS,
    START,
    STOP,
    DUMP_LOG,
    DUMP_RESPONSE_STRING,
    DUMP_FILES,
    WAIT_SECONDS,
    WAIT_TICKS,
  } CommandType;

  struct PerfMetricCommand
  {
    CommandType _command;
    DumpType    _dumpType;
    bool        _dumpAll;
    float       _waitSeconds;
    int         _waitTicks;

    PerfMetricCommand(CommandType cmd)
    {
      _command = cmd; _dumpType = DT_LOG; _dumpAll = true;
    }
    PerfMetricCommand(CommandType cmd, DumpType dumpType, bool dumpAll)
    {
      _command = cmd; _dumpType = dumpType, _dumpAll = dumpAll;
    }
  };
  
  std::queue<PerfMetricCommand> _queuedCommands;
};


} // namespace Vector
} // namespace Anki


#endif // __Vector_iPerfMetric_H__
