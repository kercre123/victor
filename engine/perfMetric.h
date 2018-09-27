/**
 * File: perfMetric
 *
 * Author: Paul Terry
 * Created: 11/03/2017
 *
 * Description: Lightweight performance metric recording
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Engine_PerfMetric_H__
#define __Cozmo_Engine_PerfMetric_H__

#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/iBehavior.h"
#include "clad/types/behaviorComponent/activeFeatures.h"

#include <string>
#include <queue>
#include <vector>


namespace Anki {
namespace Vector {


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
  explicit PerfMetric(const CozmoContext* context);
  ~PerfMetric();

  void Init();

  void Update(const float tickDuration_ms,
              const float tickFrequency_ms,
              const float sleepDurationIntended_ms,
              const float sleepDurationActual_ms);

  void Start();
  void Stop();
  void Dump(const DumpType dumpType, const bool dumpAll,
            const std::string* fileName = nullptr, std::string* resultStr = nullptr) const;
  void DumpFiles() const;
  void WaitSeconds(const float seconds);
  void WaitTicks(const int ticks);

  const CozmoContext* GetContext() { return _context; }

  bool GetAutoRecord() const { return _autoRecord; }

  int  ParseCommands(std::string& queryString);
  void ExecuteQueuedCommands(std::string* resultStr = nullptr);

  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);

private:

  void DumpHeading(const DumpType dumpType, const bool showBehaviorHeading,
                   FILE* fd, std::string* resultStr) const;
  bool FrameBufferEmpty() const { return _nextFrameIndex == 0 && !_bufferFilled; }

  struct FrameMetric
  {
    float _tickExecution_ms;
    float _tickTotal_ms;
    float _tickSleepIntended_ms;
    float _tickSleepActual_ms;

    uint32_t _messageCountRtE;
    uint32_t _messageCountEtR;
    uint32_t _messageCountGtE;
    uint32_t _messageCountEtG;
    uint32_t _messageCountViz;
    uint32_t _messageCountGatewayToE;
    uint32_t _messageCountEToGateway;

    float _batteryVoltage;
    uint32_t _cpuFreq_kHz;

    ActiveFeature    _activeFeature;
    static const int kBehaviorStringMaxSize = 32;
    char _behavior[kBehaviorStringMaxSize]; // Some description of what Victor is doing
  };

  static const int kNumFramesInBuffer = 4000;

  FrameMetric*        _frameBuffer = nullptr;
  int                 _nextFrameIndex = 0;
  bool                _bufferFilled = false;
  bool                _isRecording = false;
  bool                _autoRecord;
  bool                _waitMode = false;
  int                 _waitTicksRemaining = 0;
  float               _waitTimeToExpire = 0.0f;

  const CozmoContext* _context;
  std::string         _baseLogDir = "";
  static const std::string _logBaseFileName;
  static const int    kNumCharsInLineBuffer = 256;
  char*               _lineBuffer;

  typedef enum
  {
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


#endif // __Cozmo_Engine_PerfMetric_H__
