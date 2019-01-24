/**
 * File: perfMetricAnim
 *
 * Author: Paul Terry
 * Created: 11/28/2018
 *
 * Description: Lightweight performance metric recording: for vic-anim
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "cozmoAnim/animContext.h"
#include "cozmoAnim/animProcessMessages.h"
#include "cozmoAnim/perfMetricAnim.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/perfMetric/perfMetricImpl.h"
#include "util/stats/statsAccumulator.h"

#include <iomanip>


#define LOG_CHANNEL "PerfMetric"

namespace Anki {
namespace Vector {


PerfMetricAnim::PerfMetricAnim(const AnimContext* context)
  : _frameBuffer(nullptr)
  //, _context(context)
{
}

PerfMetricAnim::~PerfMetricAnim()
{
#if ANKI_PERF_METRIC_ENABLED
  OnShutdown();

  delete[] _frameBuffer;
#endif
}

void PerfMetricAnim::Init(Util::Data::DataPlatform* dataPlatform, WebService::WebService* webService)
{
#if ANKI_PERF_METRIC_ENABLED
  _frameBuffer = new FrameMetricAnim[kNumFramesInBuffer];

  _fileNameSuffix = "Anim";
  
  InitInternal(dataPlatform, webService);
#endif
}


// This is called at the end of the tick
void PerfMetricAnim::Update(const float tickDuration_ms,
                            const float tickFrequency_ms,
                            const float sleepDurationIntended_ms,
                            const float sleepDurationActual_ms)
{
#if ANKI_PERF_METRIC_ENABLED
  ANKI_CPU_PROFILE("PerfMetricAnim::Update");

  ExecuteQueuedCommands();

  if (_isRecording)
  {
    FrameMetricAnim& frame = _frameBuffer[_nextFrameIndex];

    frame._tickExecution_ms     = tickDuration_ms;
    frame._tickTotal_ms         = tickFrequency_ms;
    frame._tickSleepIntended_ms = sleepDurationIntended_ms;
    frame._tickSleepActual_ms   = sleepDurationActual_ms;

    frame._messageCountAnimToRobot  = AnimProcessMessages::GetMessageCountAtR();
    frame._messageCountAnimToEngine = AnimProcessMessages::GetMessageCountAtE();
    frame._messageCountRobotToAnim  = AnimProcessMessages::GetMessageCountRtA();
    frame._messageCountEngineToAnim = AnimProcessMessages::GetMessageCountEtA();

    if (++_nextFrameIndex >= kNumFramesInBuffer)
    {
      _nextFrameIndex = 0;
      _bufferFilled = true;
    }
  }

  UpdateWaitMode();
#endif
}

void PerfMetricAnim::Dump(const DumpType dumpType, const bool dumpAll,
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
  Util::Stats::StatsAccumulator accMessageCountRtA;
  Util::Stats::StatsAccumulator accMessageCountAtR;
  Util::Stats::StatsAccumulator accMessageCountEtA;
  Util::Stats::StatsAccumulator accMessageCountAtE;

  if (dumpAll)
  {
    DumpHeading(dumpType, dumpAll, fd, resultStr);
  }

  for (int frameIndex = 0; frameIndex < numFrames; frameIndex++)
  {
    const FrameMetricAnim& frame = _frameBuffer[frameBufferIndex];

    // This stat is calculated rather than stored
    const float tickSleepOver_ms = frame._tickSleepActual_ms - frame._tickSleepIntended_ms;

    accTickDuration    += frame._tickExecution_ms;
    accTickTotal       += frame._tickTotal_ms;
    accSleepIntended   += frame._tickSleepIntended_ms;
    accSleepActual     += frame._tickSleepActual_ms;
    accSleepOver       += tickSleepOver_ms;
    accMessageCountRtA += frame._messageCountRobotToAnim;
    accMessageCountAtR += frame._messageCountAnimToRobot;
    accMessageCountEtA += frame._messageCountEngineToAnim;
    accMessageCountAtE += frame._messageCountAnimToEngine;

    static const std::string kFormatLineText = "%5i %8.3f %8.3f %8.3f %8.3f %8.3f    %5i %5i %5i %5i";
    static const std::string kFormatLineCSVText = "%5i,%8.3f,%8.3f,%8.3f,%8.3f,%8.3f,%5i,%5i,%5i,%5i";

#define LINE_DATA_VARS \
    frameIndex, frame._tickExecution_ms, frame._tickTotal_ms,\
    frame._tickSleepIntended_ms, frame._tickSleepActual_ms,\
    tickSleepOver_ms,\
    frame._messageCountRobotToAnim, frame._messageCountAnimToRobot,\
    frame._messageCountEngineToAnim, frame._messageCountAnimToEngine

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
  sprintf(_lineBuffer, "Summary:  (%s build; %s; %i anim ticks; %.3f seconds total)",
#if defined(NDEBUG)
                "RELEASE"
#else
                "DEBUG"
#endif
                ,
#if defined(SIMULATOR)  // ANKI_PLATFORM_* is not defined in vic-anim for some reason
                "MAC"
#else
                "VICOS"
#endif
          , numFrames, totalTime_sec);
  switch (dumpType)
  {
    case DT_LOG:
      LOG_INFO("PerfMetric.Dump", "%s", _lineBuffer);
      break;
    case DT_RESPONSE_STRING:  // Intentional fall-through
    case DT_FILE_TEXT:        // Intentional fall-through
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

  static const std::string kSummaryLineFormat = " %8.3f %8.3f %8.3f %8.3f %8.3f    %5.1f %5.1f %5.1f %5.1f\n";
  static const std::string kSummaryLineCSVFormat = ",%8.3f,%8.3f,%8.3f,%8.3f,%8.3f,%5.1f,%5.1f,%5.1f,%5.1f\n";

#define SUMMARY_LINE_VARS(StatCall)\
  accTickDuration.StatCall(), accTickTotal.StatCall(),\
  accSleepIntended.StatCall(), accSleepActual.StatCall(), accSleepOver.StatCall(),\
  accMessageCountRtA.StatCall(), accMessageCountAtR.StatCall(),\
  accMessageCountEtA.StatCall(), accMessageCountAtE.StatCall()

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


void PerfMetricAnim::DumpHeading(const DumpType dumpType, const bool showBehaviorHeading,
                                 FILE* fd, std::string* resultStr) const
{
  static const char* kHeading1 = "          Anim     Anim    Sleep    Sleep     Over      RtA   AtR   EtA   AtE";
  static const char* kHeading2 = "      Duration     Freq Intended   Actual    Sleep    Count Count Count Count";
  static const char* kHeadingCSV1 = ",Engine,Engine,Sleep,Sleep,Over,RtA,AtR,EtA,AtE";
  static const char* kHeadingCSV2 = ",Duration,Freq,Intended,Actual,Sleep,Count,Count,Count,Count,Count";

  switch (dumpType)
  {
    case DT_LOG:
      {
        LOG_INFO("PerfMetric.Dump", "%s", kHeading1);
        LOG_INFO("PerfMetric.Dump", "%s", kHeading2);
      }
      break;
    case DT_RESPONSE_STRING:  // Intentional fall-through
    case DT_FILE_TEXT:
      {
        int strSize = 0;
        strSize += sprintf(&_lineBuffer[strSize], "%s\n%s\n", kHeading1, kHeading2);
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
        strSize += sprintf(&_lineBuffer[strSize], "%s\n%s\n", kHeadingCSV1, kHeadingCSV2);
        fwrite(_lineBuffer, 1, strSize, fd);
      }
      break;
  }
}

} // namespace Vector
} // namespace Anki
