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

#include <string>


namespace Anki {
namespace Cozmo {


typedef enum
{
  DT_LOG,
  DT_FILE_TEXT,
  DT_FILE_CSV,
} DumpType;


class PerfMetric
{
public:
  explicit PerfMetric(const CozmoContext* cozmoContext);
  ~PerfMetric();

  void Init();

  void Update(const float tickDuration_ms,
              const float tickFrequency_ms,
              const float sleepDurationIntended_ms,
              const float sleepDurationActual_ms);

  void OnRobotDisconnected();

  static const char* kLogChannelName;

  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);

private:

  void Start();
  void Stop();
  void Dump(const DumpType dumpType, const bool dumpAll, const std::string* fileName = nullptr) const;
  void DumpFiles() const;
  void Reset();

  void DumpHeading(const DumpType dumpType, FILE* fd) const;
  void SendStatusToGame() const;
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

    float _wifiLatency_ms;

    float _batteryVoltage;

    static const int kStateStringMaxSize = 64;
    char _state[kStateStringMaxSize]; // Some description of what Cozmo is doing
  };

  static const int    kNumFramesInBuffer = 5000;
  FrameMetric*        _frameBuffer = nullptr;
  int                 _nextFrameIndex = 0;
  bool                _bufferFilled = false;
  bool                _isRecording = false;
  bool                _autoRecord; // Auto-records while connected to robot; see constructor for init
  bool                _startNextFrame = false;

  const CozmoContext* _cozmoContext;
  std::string         _baseLogDir = "";
  static const std::string _logBaseFileName;
  static const int    kNumCharsInLineBuffer = 256;
  char*               _lineBuffer;

  std::vector<Signal::SmartHandle> _signalHandles;
};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Engine_PerfMetric_H__

