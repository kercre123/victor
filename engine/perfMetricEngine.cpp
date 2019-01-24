/**
 * File: perfMetricEngine
 *
 * Author: Paul Terry
 * Created: 11/16/2018
 *
 * Description: Lightweight performance metric recording: for vic-engine
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/activeFeatureComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/cozmoContext.h"
#include "engine/components/battery/batteryComponent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/perfMetricEngine.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"
#include "osState/osState.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/perfMetric/perfMetricImpl.h"
#include "util/stats/statsAccumulator.h"

#include <iomanip>


#define LOG_CHANNEL "PerfMetric"

namespace Anki {
namespace Vector {


PerfMetricEngine::PerfMetricEngine(const CozmoContext* context)
  : _frameBuffer(nullptr)
#if ANKI_PERF_METRIC_ENABLED
  , _context(context)
#endif
{
}

PerfMetricEngine::~PerfMetricEngine()
{
#if ANKI_PERF_METRIC_ENABLED
  OnShutdown();

  delete[] _frameBuffer;
#endif
}

void PerfMetricEngine::Init(Util::Data::DataPlatform* dataPlatform, WebService::WebService* webService)
{
#if ANKI_PERF_METRIC_ENABLED
  _frameBuffer = new FrameMetricEngine[kNumFramesInBuffer];

  _fileNameSuffix = "Eng";

  InitInternal(dataPlatform, webService);
#endif
}


// This is called at the end of the tick
void PerfMetricEngine::Update(const float tickDuration_ms,
                              const float tickFrequency_ms,
                              const float sleepDurationIntended_ms,
                              const float sleepDurationActual_ms)
{
#if ANKI_PERF_METRIC_ENABLED
  ANKI_CPU_PROFILE("PerfMetricEngine::Update");

  ExecuteQueuedCommands();

  if (_isRecording)
  {
    FrameMetricEngine& frame = _frameBuffer[_nextFrameIndex];

    frame._tickExecution_ms     = tickDuration_ms;
    frame._tickTotal_ms         = tickFrequency_ms;
    frame._tickSleepIntended_ms = sleepDurationIntended_ms;
    frame._tickSleepActual_ms   = sleepDurationActual_ms;

    const auto msgHandler = _context->GetRobotManager()->GetMsgHandler();
    frame._messageCountRobotToEngine = msgHandler->GetMessageCountRtE();
    frame._messageCountEngineToRobot = msgHandler->GetMessageCountEtR();

    const auto UIMsgHandler = _context->GetExternalInterface();
    frame._messageCountGatewayToEngine = UIMsgHandler->GetMessageCountGtE();
    frame._messageCountEngineToGame = UIMsgHandler->GetMessageCountEtG();

    const auto vizManager = _context->GetVizManager();
    frame._messageCountViz = vizManager->GetMessageCountViz();

    const auto gateway = _context->GetGatewayInterface();
    frame._messageCountGatewayToEngine = gateway->GetMessageCountIncoming();
    frame._messageCountEngineToGateway = gateway->GetMessageCountOutgoing();

    Robot* robot = _context->GetRobotManager()->GetRobot();

    frame._batteryVoltage = robot == nullptr ? 0.0f : robot->GetBatteryComponent().GetBatteryVolts();

    const auto& osState = OSState::getInstance();
    frame._cpuFreq_kHz = osState->GetCPUFreq_kHz();

    if (robot != nullptr)
    {
      const auto& bc = robot->GetAIComponent().GetComponent<BehaviorComponent>();
      const auto& afc = bc.GetComponent<ActiveFeatureComponent>();
      frame._activeFeature = afc.GetActiveFeature();
      const auto& bsm = bc.GetComponent<BehaviorSystemManager>();
      strncpy(frame._behavior, bsm.GetTopBehaviorDebugLabel().c_str(), sizeof(frame._behavior));
      frame._behavior[FrameMetricEngine::kBehaviorStringMaxSize - 1] = '\0'; // Ensure string is null terminated
    }
    else
    {
      frame._activeFeature = ActiveFeature::NoFeature;
      frame._behavior[0] = '\0';
    }

    if (++_nextFrameIndex >= kNumFramesInBuffer)
    {
      _nextFrameIndex = 0;
      _bufferFilled = true;
    }
  }

  UpdateWaitMode();
#endif
}

void PerfMetricEngine::Dump(const DumpType dumpType, const bool dumpAll,
                            const std::string* fileName, std::string* resultStr) const
{
  if (FrameBufferEmpty())
  {
    LOG_INFO("PerfMetric.Dump", "Nothing to dump; buffer is empty");
    return;
  }

  FILE* fd = nullptr;
  if (dumpType == DT_FILE_TEXT || dumpType == DT_FILE_CSV)
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
  Util::Stats::StatsAccumulator accMessageCountGatewayToE;
  Util::Stats::StatsAccumulator accMessageCountEToGateway;
  Util::Stats::StatsAccumulator accMessageCountViz;
  Util::Stats::StatsAccumulator accBatteryVoltage;
  Util::Stats::StatsAccumulator accCPUFreq;

  if (dumpAll)
  {
    DumpHeading(dumpType, dumpAll, fd, resultStr);
  }

  for (int frameIndex = 0; frameIndex < numFrames; frameIndex++)
  {
    const FrameMetricEngine& frame = _frameBuffer[frameBufferIndex];

    // This stat is calculated rather than stored
    const float tickSleepOver_ms = frame._tickSleepActual_ms - frame._tickSleepIntended_ms;

    accTickDuration    += frame._tickExecution_ms;
    accTickTotal       += frame._tickTotal_ms;
    accSleepIntended   += frame._tickSleepIntended_ms;
    accSleepActual     += frame._tickSleepActual_ms;
    accSleepOver       += tickSleepOver_ms;
    accMessageCountRtE += frame._messageCountRobotToEngine;
    accMessageCountEtR += frame._messageCountEngineToRobot;
    accMessageCountGtE += frame._messageCountGameToEngine;
    accMessageCountEtG += frame._messageCountEngineToGame;
    accMessageCountGatewayToE += frame._messageCountGatewayToEngine;
    accMessageCountEToGateway += frame._messageCountEngineToGateway;
    accMessageCountViz += frame._messageCountViz;
    accBatteryVoltage  += frame._batteryVoltage;
    accCPUFreq         += frame._cpuFreq_kHz;

    static const std::string kFormatLineText = "%5i %8.3f %8.3f %8.3f %8.3f %8.3f    %5i %5i %5i %5i %5i %5i %5i %8.3f %6i  %s  %s";
    static const std::string kFormatLineCSVText = "%5i,%8.3f,%8.3f,%8.3f,%8.3f,%8.3f,%5i,%5i,%5i,%5i,%5i,%5i,%5i,%8.3f,%6i,%s,%s";

#define LINE_DATA_VARS \
    frameIndex, frame._tickExecution_ms, frame._tickTotal_ms,\
    frame._tickSleepIntended_ms, frame._tickSleepActual_ms,\
    tickSleepOver_ms,\
    frame._messageCountRobotToEngine, frame._messageCountEngineToRobot,\
    frame._messageCountGameToEngine, frame._messageCountEngineToGame,\
    frame._messageCountGatewayToEngine, frame._messageCountEngineToGateway,\
    frame._messageCountViz,\
    frame._batteryVoltage, frame._cpuFreq_kHz, EnumToString(frame._activeFeature),\
    frame._behavior

    if (dumpAll)
    {
      switch (dumpType)
      {
        case DT_LOG:
          {
            LOG_INFO("PerfMetric.Dump", kFormatLineText.c_str(), LINE_DATA_VARS);
          }
          break;
        case DT_RESPONSE_STRING:  // Intentional fall-through
        case DT_FILE_TEXT:
          {
            int strSize = 0;
            strSize += sprintf(&_lineBuffer[strSize],
                               (kFormatLineText + "\n").c_str(), LINE_DATA_VARS);
            if (dumpType == DT_FILE_TEXT)
            {
              fwrite(_lineBuffer, 1, strSize, fd);
            }
            else if (resultStr)
            {
              *resultStr += _lineBuffer;
            }
          }
          break;
        case DT_FILE_CSV:
          {
            int strSize = 0;
            strSize += sprintf(&_lineBuffer[strSize],
                               (kFormatLineCSVText + "\n").c_str(), LINE_DATA_VARS);
            fwrite(_lineBuffer, 1, strSize, fd);
            if (resultStr)
            {
              *resultStr += _lineBuffer;
            }
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
  sprintf(_lineBuffer, "Summary:  (%s build; %s; %i engine ticks; %.3f seconds total)",
#if defined(NDEBUG)
                "RELEASE"
#else
                "DEBUG"
#endif
                ,
#if defined(ANKI_PLATFORM_OSX)
                "MAC"
#elif defined(ANKI_PLATFORM_VICOS)
                "VICOS"
#else
                "UNKNOWN"
#endif
          , numFrames, totalTime_sec);
  switch (dumpType)
  {
    case DT_LOG:
      LOG_INFO("PerfMetric.Dump", "%s", _lineBuffer);
      break;
    case DT_RESPONSE_STRING:  // Intentional fall-through
    case DT_FILE_TEXT:  // Intentional fall-through
    case DT_FILE_CSV:
      {
        auto index = strlen(_lineBuffer);
        _lineBuffer[index++] = '\n';
        _lineBuffer[index] = '\0';
        if (dumpType != DT_RESPONSE_STRING)
        {
          fwrite(_lineBuffer, 1, strlen(_lineBuffer), fd);
        }
        else if (resultStr)
        {
          *resultStr += _lineBuffer;
        }
      }
      break;
  }

  static const bool kShowBehaviorHeading = false;
  DumpHeading(dumpType, kShowBehaviorHeading, fd, resultStr);

  static const std::string kSummaryLineFormat = " %8.3f %8.3f %8.3f %8.3f %8.3f    %5.1f %5.1f %5.1f %5.1f %5.1f %5.1f %5.1f %8.3f %6.0f\n";
  static const std::string kSummaryLineCSVFormat = ",%8.3f,%8.3f,%8.3f,%8.3f,%8.3f,%5.1f,%5.1f,%5.1f,%5.1f,%5.1f,%5.1f,%5.1f,%8.3f,%6.0f\n";

#define SUMMARY_LINE_VARS(StatCall)\
  accTickDuration.StatCall(), accTickTotal.StatCall(),\
  accSleepIntended.StatCall(), accSleepActual.StatCall(), accSleepOver.StatCall(),\
  accMessageCountRtE.StatCall(), accMessageCountEtR.StatCall(),\
  accMessageCountGtE.StatCall(), accMessageCountEtG.StatCall(),\
  accMessageCountGatewayToE.StatCall(), accMessageCountEToGateway.StatCall(),\
  accMessageCountViz.StatCall(),\
  accBatteryVoltage.StatCall(), accCPUFreq.StatCall()

  switch (dumpType)
  {
    case DT_LOG:
      {
        LOG_INFO("PerfMetric.Dump", (" Min:" + kSummaryLineFormat).c_str(), SUMMARY_LINE_VARS(GetMin));
        LOG_INFO("PerfMetric.Dump", (" Max:" + kSummaryLineFormat).c_str(), SUMMARY_LINE_VARS(GetMax));
        LOG_INFO("PerfMetric.Dump", ("Mean:" + kSummaryLineFormat).c_str(), SUMMARY_LINE_VARS(GetMean));
        LOG_INFO("PerfMetric.Dump", (" Std:" + kSummaryLineFormat).c_str(), SUMMARY_LINE_VARS(GetStd));
      }
      break;
    case DT_RESPONSE_STRING:  // Intentional fall-through
    case DT_FILE_TEXT:
      {
        int strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], " Min:");
        strSize += sprintf(&_lineBuffer[strSize], (kSummaryLineFormat.c_str()),
                           SUMMARY_LINE_VARS(GetMin));
        if (dumpType == DT_FILE_TEXT)
        {
          fwrite(_lineBuffer, 1, strSize, fd);
        }
        else if (resultStr)
        {
          *resultStr += _lineBuffer;
        }
        strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], " Max:");
        strSize += sprintf(&_lineBuffer[strSize], (kSummaryLineFormat.c_str()),
                           SUMMARY_LINE_VARS(GetMax));
        if (dumpType == DT_FILE_TEXT)
        {
          fwrite(_lineBuffer, 1, strSize, fd);
        }
        else if (resultStr)
        {
          *resultStr += _lineBuffer;
        }
        strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], "Mean:");
        strSize += sprintf(&_lineBuffer[strSize], (kSummaryLineFormat.c_str()),
                           SUMMARY_LINE_VARS(GetMean));
        if (dumpType == DT_FILE_TEXT)
        {
          fwrite(_lineBuffer, 1, strSize, fd);
        }
        else if (resultStr)
        {
          *resultStr += _lineBuffer;
        }
        strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], " Std:");
        strSize += sprintf(&_lineBuffer[strSize], (kSummaryLineFormat.c_str()),
                           SUMMARY_LINE_VARS(GetStd));
        if (dumpType == DT_FILE_TEXT)
        {
          fwrite(_lineBuffer, 1, strSize, fd);
        }
        else if (resultStr)
        {
          *resultStr += _lineBuffer;
        }
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

  if (dumpType == DT_FILE_TEXT || dumpType == DT_FILE_CSV)
  {
    fclose(fd);
  }
}

void PerfMetricEngine::DumpHeading(const DumpType dumpType, const bool showBehaviorHeading,
                                   FILE* fd, std::string* resultStr) const
{
  static const char* kHeading1 = "        Engine   Engine    Sleep    Sleep     Over      RtE   EtR   GtE   EtG  GWtE  EtGW   Viz  Battery    CPU";
  static const char* kHeading2 = "      Duration     Freq Intended   Actual    Sleep    Count Count Count Count Count Count Count  Voltage   Freq";
  static const char* kHeading3 = "  Active Feature/Behavior";
  static const char* kHeadingCSV1 = ",Engine,Engine,Sleep,Sleep,Over,RtE,EtR,GtE,EtG,GWtE,EtGW,Viz,Battery,CPU";
  static const char* kHeadingCSV2 = ",Duration,Freq,Intended,Actual,Sleep,Count,Count,Count,Count,Count,Count,Count,Voltage,Freq";
  static const char* kHeadingCSV3 = ",Active Feature,Behavior";

  switch (dumpType)
  {
    case DT_LOG:
      {
        LOG_INFO("PerfMetric.Dump", "%s", kHeading1);
        LOG_INFO("PerfMetric.Dump", "%s%s", kHeading2, showBehaviorHeading ? kHeading3 : "");
      }
      break;
    case DT_RESPONSE_STRING:  // Intentional fall-through
    case DT_FILE_TEXT:
      {
        int strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], "%s\n%s%s\n", kHeading1, kHeading2,
                           showBehaviorHeading ? kHeading3 : "");
        if (dumpType == DT_FILE_TEXT)
        {
          fwrite(_lineBuffer, 1, strSize, fd);
        }
        else if (resultStr)
        {
          *resultStr += _lineBuffer;
        }
      }
      break;
    case DT_FILE_CSV:
      {
        int strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], "%s\n%s%s\n", kHeadingCSV1, kHeadingCSV2,
                           showBehaviorHeading ? kHeadingCSV3 : "");
        fwrite(_lineBuffer, 1, strSize, fd);
      }
      break;
  }
}

} // namespace Vector
} // namespace Anki
