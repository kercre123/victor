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


#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/ankiEventUtil.h"
#include "engine/cozmoContext.h"
#include "engine/components/batteryComponent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/perfMetric.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/stats/statsAccumulator.h"
#include "util/transport/connectionStats.h"
#include <iomanip>

#if USE_DAS
#include <DAS/DAS.h>
#include <DAS/DASPlatform.h>
#endif

// To enable PerfMetric in a shipping build, define PERF_METRIC_ENABLED
#if !defined(NDEBUG)
  #define PERF_METRIC_ENABLED
#endif

namespace Anki {
namespace Cozmo {

const char* PerfMetric::kLogChannelName = "PerfMetric";


using namespace std::chrono;
using Time = time_point<system_clock>;

const std::string PerfMetric::_logBaseFileName = "perfMetric_";


PerfMetric::PerfMetric(const CozmoContext* cozmoContext)
: _frameBuffer(nullptr)
, _nextFrameIndex(0)
, _bufferFilled(false)
, _isRecording(false)
#if defined(PERF_METRIC_ENABLED)
, _autoRecord(true)
#else
, _autoRecord(false)
#endif
, _startNextFrame(false)
, _cozmoContext(cozmoContext)
, _baseLogDir()
, _lineBuffer(nullptr)
, _signalHandles()
{
}

PerfMetric::~PerfMetric()
{
  _signalHandles.clear();
  delete _frameBuffer;
}


void PerfMetric::Init()
{
#if defined(PERF_METRIC_ENABLED)
  _frameBuffer = new FrameMetric[kNumFramesInBuffer];
  int sizeKb = (sizeof(FrameMetric) * kNumFramesInBuffer) / 1024;
  PRINT_CH_INFO(kLogChannelName, "PerfMetric.Init",
                "Frame buffer size is %u KB", sizeKb);

  _lineBuffer = new char[kNumCharsInLineBuffer];

  _baseLogDir = _cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Cache, "");

  if (_cozmoContext->GetExternalInterface() != nullptr)
  {
    auto helper = MakeAnkiEventUtil(*_cozmoContext->GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::PerfMetricCommand>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::PerfMetricGetStatus>();
    // helper.SubscribeGameToEngine<MessageGameToEngineTag::ConnectToRobot>();
  }
#endif
}


// This is called whether we are connected to a robot or not
void PerfMetric::Update(const float tickDuration_ms,
                        const float tickFrequency_ms,
                        const float sleepDurationIntended_ms,
                        const float sleepDurationActual_ms)
{
#if defined(PERF_METRIC_ENABLED)
  ANKI_CPU_PROFILE("PerfMetric::Update");

  if (_isRecording)
  {
    FrameMetric& frame = _frameBuffer[_nextFrameIndex];

    frame._tickExecution_ms     = tickDuration_ms;
    frame._tickTotal_ms         = tickFrequency_ms;
    frame._tickSleepIntended_ms = sleepDurationIntended_ms;
    frame._tickSleepActual_ms   = sleepDurationActual_ms;

    const auto MsgHandler = _cozmoContext->GetRobotManager()->GetMsgHandler();
    frame._messageCountRtE = MsgHandler->GetMessageCountRtE();
    frame._messageCountEtR = MsgHandler->GetMessageCountEtR();

    const auto UIMsgHandler = _cozmoContext->GetExternalInterface();
    frame._messageCountGtE = UIMsgHandler->GetMessageCountGtE();
    frame._messageCountEtG = UIMsgHandler->GetMessageCountEtG();

    const auto VizManager = _cozmoContext->GetVizManager();
    frame._messageCountViz = VizManager->GetMessageCountViz();

    frame._wifiLatency_ms = Util::gNetStat2LatencyAvg;

    Robot* robot = _cozmoContext->GetRobotManager()->GetRobot();

    frame._batteryVoltage = robot == nullptr ? 0.0f : robot->GetBatteryComponent().GetBatteryVolts();

    strncpy(frame._state, robot == nullptr ? "" : robot->GetBehaviorDebugString().c_str(), sizeof(frame._state));
    frame._state[FrameMetric::kStateStringMaxSize - 1] = '\0'; // Ensure string is null terminated

    if (++_nextFrameIndex >= kNumFramesInBuffer)
    {
      _nextFrameIndex = 0;
      _bufferFilled = true;
    }
  }

  if (_startNextFrame)
  {
    _startNextFrame = false;
    Start();
  }
#endif
}

void PerfMetric::Start()
{
  if (_isRecording)
  {
    PRINT_CH_INFO(kLogChannelName, "PerfMetric.Start", "Recording already in progress");
  }
  else
  {
    _isRecording = true;
    PRINT_CH_INFO(kLogChannelName, "PerfMetric.Start", "Recording started");
  }

  SendStatusToGame();
}

void PerfMetric::Stop()
{
  if (!_isRecording)
  {
    PRINT_CH_INFO(kLogChannelName, "Perfmetric.Stop", "Recording was already stopped");
  }
  else
  {
    _isRecording = false;
    PRINT_CH_INFO(kLogChannelName, "Perfmetric.Stop", "Recording stopped");
  }

  SendStatusToGame();
}

void PerfMetric::Dump(const DumpType dumpType, const bool dumpAll, const std::string* fileName) const
{
  if (FrameBufferEmpty())
  {
    PRINT_CH_INFO(kLogChannelName, "PerfMetric.Dump", "Nothing to dump; buffer is empty");
    return;
  }

  FILE* fd = nullptr;
  if (dumpType != DT_LOG)
  {
    fd = fopen(fileName->c_str(), "w");
  }

  int frameBufferIndex = _bufferFilled ? _nextFrameIndex : 0;
  const int numFrames  = _bufferFilled ? kNumFramesInBuffer : _nextFrameIndex;
  Util::Stats::StatsAccumulator accTickDuration;
  Util::Stats::StatsAccumulator accTickTotal;
  Util::Stats::StatsAccumulator accSleepIntended;
  Util::Stats::StatsAccumulator accSleepActual;
  Util::Stats::StatsAccumulator accSleepOver;
  Util::Stats::StatsAccumulator accMessageCountRtE;
  Util::Stats::StatsAccumulator accMessageCountEtR;
  Util::Stats::StatsAccumulator accMessageCountGtE;
  Util::Stats::StatsAccumulator accMessageCountEtG;
  Util::Stats::StatsAccumulator accMessageCountViz;
  Util::Stats::StatsAccumulator accWifiLatency;
  Util::Stats::StatsAccumulator accBatteryVoltage;

  if (dumpAll)
  {
    DumpHeading(dumpType, fd);
  }

  for (int frameIndex = 0; frameIndex < numFrames; frameIndex++)
  {
    const FrameMetric& frame = _frameBuffer[frameBufferIndex];

    // This stat is calculated rather than stored
    const float tickSleepOver_ms = frame._tickSleepActual_ms - frame._tickSleepIntended_ms;

    accTickDuration    += frame._tickExecution_ms;
    accTickTotal       += frame._tickTotal_ms;
    accSleepIntended   += frame._tickSleepIntended_ms;
    accSleepActual     += frame._tickSleepActual_ms;
    accSleepOver       += tickSleepOver_ms;
    accMessageCountRtE += frame._messageCountRtE;
    accMessageCountEtR += frame._messageCountEtR;
    accMessageCountGtE += frame._messageCountGtE;
    accMessageCountEtG += frame._messageCountEtG;
    accMessageCountViz += frame._messageCountViz;
    accWifiLatency     += frame._wifiLatency_ms;
    accBatteryVoltage  += frame._batteryVoltage;

    static const std::string kFormatLineText = "%5i %8.3f %8.3f %8.3f %8.3f %8.3f    %5i %5i %5i %5i %5i %8.3f %8.3f  %s";
    static const std::string kFormatLineCSVText = "%5i,%8.3f,%8.3f,%8.3f,%8.3f,%8.3f,%5i,%5i,%5i,%5i,%5i,%8.3f,%8.3f,%s";

#define LINE_DATA_VARS \
    frameIndex, frame._tickExecution_ms, frame._tickTotal_ms,\
    frame._tickSleepIntended_ms, frame._tickSleepActual_ms,\
    tickSleepOver_ms,\
    frame._messageCountRtE, frame._messageCountEtR,\
    frame._messageCountGtE, frame._messageCountEtG,\
    frame._messageCountViz, frame._wifiLatency_ms,\
    frame._batteryVoltage,  frame._state

    if (dumpAll)
    {
      switch (dumpType)
      {
        case DT_LOG:
          {
            PRINT_CH_INFO(kLogChannelName, "PerfMetric.Dump",
                          kFormatLineText.c_str(), LINE_DATA_VARS);
          }
          break;
        case DT_FILE_TEXT:
          {
            int strSize = 0;
            strSize += sprintf(&_lineBuffer[strSize],
                               (kFormatLineText + "\n").c_str(), LINE_DATA_VARS);
            fwrite(_lineBuffer, 1, strSize, fd);
          }
          break;
        case DT_FILE_CSV:
        {
          int strSize = 0;
          strSize += sprintf(&_lineBuffer[strSize],
                             (kFormatLineCSVText + "\n").c_str(), LINE_DATA_VARS);
          fwrite(_lineBuffer, 1, strSize, fd);
        }
          break;
      }
    }

    if (++frameBufferIndex >= kNumFramesInBuffer)
    {
      frameBufferIndex = 0;
    }
  }

  const float totalTime_sec = accTickTotal.GetVal() * 0.001f;
#if USE_DAS
  const DAS::IDASPlatform* platform = DASGetPlatform();
#endif
  sprintf(_lineBuffer, "Summary:  (%s build; %s; %s; %s; %i engine ticks; %.3f seconds total)",
#if defined(NDEBUG)
                "RELEASE"
#else
                "DEBUG"
#endif
                ,
#if defined(ANKI_PLATFORM_IOS)
                "IOS"
#elif defined(ANKI_PLATFORM_ANDROID)
                "ANDROID"
#elif defined(ANKI_PLATFORM_OSX)
                "MAC"
#else
                "UNKNOWN"
#endif
#if USE_DAS
          , platform->GetDeviceModel(), platform->GetOsVersion(),
#else
          , "Mac", "MacOS",
#endif
          numFrames, totalTime_sec);
  switch (dumpType)
  {
    case DT_LOG:
      PRINT_CH_INFO(kLogChannelName, "PerfMetric.Dump", "%s", _lineBuffer);
      break;
    case DT_FILE_TEXT:  // Intentional fall-through
    case DT_FILE_CSV:
      {
        auto index = strlen(_lineBuffer);
        _lineBuffer[index++] = '\n';
        _lineBuffer[index] = '\0';
        fwrite(_lineBuffer, 1, strlen(_lineBuffer), fd);
      }
      break;
  }

  DumpHeading(dumpType, fd);

  static const std::string kSummaryLineFormat = " %8.3f %8.3f %8.3f %8.3f %8.3f    %5.1f %5.1f %5.1f %5.1f %5.1f %8.3f %8.3f\n";
  static const std::string kSummaryLineCSVFormat = ",%8.3f,%8.3f,%8.3f,%8.3f,%8.3f,%5.1f,%5.1f,%5.1f,%5.1f,%5.1f,%8.3f,%8.3f\n";

#define SUMMARY_LINE_VARS(StatCall)\
  accTickDuration.StatCall(), accTickTotal.StatCall(),\
  accSleepIntended.StatCall(), accSleepActual.StatCall(), accSleepOver.StatCall(),\
  accMessageCountRtE.StatCall(), accMessageCountEtR.StatCall(),\
  accMessageCountGtE.StatCall(), accMessageCountEtG.StatCall(), accMessageCountViz.StatCall(),\
  accWifiLatency.StatCall(), accBatteryVoltage.StatCall()

  switch (dumpType)
  {
    case DT_LOG:
      {
        PRINT_CH_INFO(kLogChannelName, "PerfMetric.Dump", (" Min:" + kSummaryLineFormat).c_str(),
                      SUMMARY_LINE_VARS(GetMin));
        PRINT_CH_INFO(kLogChannelName, "PerfMetric.Dump", (" Max:" + kSummaryLineFormat).c_str(),
                      SUMMARY_LINE_VARS(GetMax));
        PRINT_CH_INFO(kLogChannelName, "PerfMetric.Dump", ("Mean:" + kSummaryLineFormat).c_str(),
                      SUMMARY_LINE_VARS(GetMean));
        PRINT_CH_INFO(kLogChannelName, "PerfMetric.Dump", (" Std:" + kSummaryLineFormat).c_str(),
                      SUMMARY_LINE_VARS(GetStd));
      }
      break;
    case DT_FILE_TEXT:
      {
        int strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], " Min:");
        strSize += sprintf(&_lineBuffer[strSize], (kSummaryLineFormat.c_str()),
                           SUMMARY_LINE_VARS(GetMin));
        fwrite(_lineBuffer, 1, strSize, fd);
        strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], " Max:");
        strSize += sprintf(&_lineBuffer[strSize], (kSummaryLineFormat.c_str()),
                           SUMMARY_LINE_VARS(GetMax));
        fwrite(_lineBuffer, 1, strSize, fd);
        strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], "Mean:");
        strSize += sprintf(&_lineBuffer[strSize], (kSummaryLineFormat.c_str()),
                           SUMMARY_LINE_VARS(GetMean));
        fwrite(_lineBuffer, 1, strSize, fd);
        strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], " Std:");
        strSize += sprintf(&_lineBuffer[strSize], (kSummaryLineFormat.c_str()),
                           SUMMARY_LINE_VARS(GetStd));
        fwrite(_lineBuffer, 1, strSize, fd);
      }
      break;
    case DT_FILE_CSV:
      {
        int strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], " Min:");
        strSize += sprintf(&_lineBuffer[strSize], (kSummaryLineCSVFormat.c_str()),
                           SUMMARY_LINE_VARS(GetMin));
        fwrite(_lineBuffer, 1, strSize, fd);
        strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], " Max:");
        strSize += sprintf(&_lineBuffer[strSize], (kSummaryLineCSVFormat.c_str()),
                           SUMMARY_LINE_VARS(GetMax));
        fwrite(_lineBuffer, 1, strSize, fd);
        strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], "Mean:");
        strSize += sprintf(&_lineBuffer[strSize], (kSummaryLineCSVFormat.c_str()),
                           SUMMARY_LINE_VARS(GetMean));
        fwrite(_lineBuffer, 1, strSize, fd);
        strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], " Std:");
        strSize += sprintf(&_lineBuffer[strSize], (kSummaryLineCSVFormat.c_str()),
                           SUMMARY_LINE_VARS(GetStd));
        fwrite(_lineBuffer, 1, strSize, fd);
      }
      break;
  }

  if (dumpType != DT_LOG)
  {
    fclose(fd);
  }
}

void PerfMetric::DumpHeading(const DumpType dumpType, FILE* fd) const
{
  static const char* kHeading1 = "        Engine   Engine    Sleep    Sleep     Over      RtE   EtR   GtE   EtG   Viz     WiFi  Battery";
  static const char* kHeading2 = "      Duration     Freq Intended   Actual    Sleep    Count Count Count Count Count  Latency  Voltage";
  static const char* kHeadingCSV1 = ",Engine,Engine,Sleep,Sleep,Over,RtE,EtR,GtE,EtG,Viz,WiFi,Battery";
  static const char* kHeadingCSV2 = ",Duration,Freq,Intended,Actual,Sleep,Count,Count,Count,Count,Count,Latency,Voltage";

  switch (dumpType)
  {
    case DT_LOG:
      {
        PRINT_CH_INFO(kLogChannelName, "PerfMetric.Dump", "%s", kHeading1);
        PRINT_CH_INFO(kLogChannelName, "PerfMetric.Dump", "%s", kHeading2);
      }
      break;
    case DT_FILE_TEXT:
      {
        int strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], "%s\n%s\n", kHeading1, kHeading2);
        fwrite(_lineBuffer, 1, strSize, fd);
      }
      break;
    case DT_FILE_CSV:
      {
        int strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], "%s\n%s\n", kHeadingCSV1, kHeadingCSV2);
        fwrite(_lineBuffer, 1, strSize, fd);
      }
      break;
  }
}

void PerfMetric::DumpFiles() const
{
  if (FrameBufferEmpty())
  {
    PRINT_CH_INFO(kLogChannelName, "PerfMetric.DumpFiles", "Nothing to dump; buffer is empty");
    return;
  }

  PRINT_CH_INFO(kLogChannelName, "PerfMetric.DumpFiles", "Dumping to files");

  const auto now = std::chrono::system_clock::now();
  const auto now_time = std::chrono::system_clock::to_time_t(now);
  const auto tm = *std::localtime(&now_time);

  std::string logDir = _baseLogDir + "/perfMetricLogs";
  Util::FileUtils::CreateDirectory(logDir);
  std::ostringstream sstr;
  sstr << logDir << "/" << _logBaseFileName << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
#if defined(NDEBUG)
  sstr << "_R";
#else
  sstr << "_D";
#endif
  const std::string logFileNameText = sstr.str() + ".txt";
  const std::string logFileNameCSV = sstr.str() + ".csv";

  static const bool kDumpAll = true;

  // Write to text file
  Dump(DT_FILE_TEXT, kDumpAll, &logFileNameText);

  // Write to CSV file
  Dump(DT_FILE_CSV, kDumpAll, &logFileNameCSV);

}

void PerfMetric::Reset()
{
  if (_isRecording)
  {
    PRINT_CH_INFO(kLogChannelName, "PerfMetric.Reset", "Recording aborted by Reset command");
  }
  PRINT_CH_INFO(kLogChannelName, "PerfMetric.Reset", "Resetting recording buffer");
  _isRecording = false;
  _nextFrameIndex = 0;
  _bufferFilled = false;

  SendStatusToGame();
}

void PerfMetric::OnRobotDisconnected()
{
  if (_autoRecord)
  {
    Stop();
  }

  DumpFiles();
}


template<>
void PerfMetric::HandleMessage(const ExternalInterface::PerfMetricCommand& msg)
{
  switch (msg.command)
  {
    case ExternalInterface::PerfMetricCommandType::Reset:     Reset();             break;
    case ExternalInterface::PerfMetricCommandType::Start:     Start();             break;
    case ExternalInterface::PerfMetricCommandType::Stop:      Stop();              break;
    case ExternalInterface::PerfMetricCommandType::Dump:      Dump(DT_LOG, false); break;
    case ExternalInterface::PerfMetricCommandType::DumpAll:   Dump(DT_LOG, true);  break;
    case ExternalInterface::PerfMetricCommandType::DumpFiles: DumpFiles();         break;
  }
}

template<>
void PerfMetric::HandleMessage(const ExternalInterface::PerfMetricGetStatus& msg)
{
  SendStatusToGame();
}

// template<>
// void PerfMetric::HandleMessage(const ExternalInterface::ConnectToRobot& msg)
// {
//   if (_autoRecord)
//   {
//     // Since we've just connected, and that frame typically takes a long time,
//     // let's not actually include it in this recording.  Instead, start the
//     // recording on the next frame.
//     _startNextFrame = true;
//   }
// }

void PerfMetric::SendStatusToGame() const
{
  const int numTicksRecorded = _bufferFilled ? kNumFramesInBuffer : _nextFrameIndex;
  const std::string statusStr = _isRecording ?
                                "RECORDING" :
                                "Number of ticks in buffer: " + std::to_string(numTicksRecorded);

  ExternalInterface::PerfMetricStatus msg(_isRecording, statusStr.c_str());
  const auto& extInt = _cozmoContext->GetExternalInterface();
  extInt->Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
}


} // namespace Cozmo
} // namespace Anki

