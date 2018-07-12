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
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/activeFeatureComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
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
#include "util/string/stringUtils.h"
#include "webServerProcess/src/webService.h"
#include <iomanip>

#if USE_DAS
#include <DAS/DAS.h>
#include <DAS/DASPlatform.h>
#endif

// To enable PerfMetric in a build, define ANKI_PERF_METRIC_ENABLED as 1
#if !defined(ANKI_PERF_METRIC_ENABLED)
  #if ANKI_DEV_CHEATS
    #define ANKI_PERF_METRIC_ENABLED 1
  #else
    #define ANKI_PERF_METRIC_ENABLED 0
  #endif
#endif

namespace Anki {
namespace Cozmo {

const char* PerfMetric::kLogChannelName = "PerfMetric";


using namespace std::chrono;
using Time = time_point<system_clock>;

const std::string PerfMetric::_logBaseFileName = "perfMetric_";


#if ANKI_PERF_METRIC_ENABLED

static int PerfMetricWebServerImpl(WebService::WebService::Request* request)
{
  auto* perfMetric = static_cast<PerfMetric*>(request->_cbdata);
  PRINT_CH_INFO(PerfMetric::kLogChannelName, "PerfMetric.PerfMetricWebServerImpl",
                "Query string: %s", request->_param1.c_str());
  
  int returnCode = perfMetric->ParseCommands(request->_param1);
  if (returnCode != 0)
  {
    // If there were no errors, attempt to execute the commands
    // (may be blocked by wait mode), output string messages/results
    // so that they can be returned in the web request
    perfMetric->ExecuteQueuedCommands(&request->_result);
  }

  return returnCode;
}

// Note that this can be called at any arbitrary time, from a webservice thread
static int PerfMetricWebServerHandler(struct mg_connection *conn, void *cbdata)
{
  const mg_request_info* info = mg_get_request_info(conn);
  auto* perfMetric = static_cast<PerfMetric*>(cbdata);

  std::string commands;
  if (info->content_length > 0)
  {
    char buf[info->content_length + 1];
    mg_read(conn, buf, sizeof(buf));
    buf[info->content_length] = 0;
    commands = buf;
  }
  else if (info->query_string)
  {
    commands = info->query_string;
  }
  
  auto ws = perfMetric->GetContext()->GetWebService();
  const int returnCode = ws->ProcessRequestExternal(conn, cbdata, PerfMetricWebServerImpl, commands);

  return returnCode;
}

#endif

PerfMetric::PerfMetric(const CozmoContext* context)
: _frameBuffer(nullptr)
, _nextFrameIndex(0)
, _bufferFilled(false)
, _isRecording(false)
#if ANKI_PERF_METRIC_ENABLED
, _autoRecord(true)
#else
, _autoRecord(false)
#endif
, _startNextFrame(false)
, _context(context)
, _baseLogDir()
, _lineBuffer(nullptr)
, _signalHandles()
, _queuedCommands()
{
}

PerfMetric::~PerfMetric()
{
  _signalHandles.clear();
  delete _frameBuffer;
}


void PerfMetric::Init()
{
#if ANKI_PERF_METRIC_ENABLED
  _frameBuffer = new FrameMetric[kNumFramesInBuffer];
  int sizeKb = (sizeof(FrameMetric) * kNumFramesInBuffer) / 1024;
  PRINT_CH_INFO(kLogChannelName, "PerfMetric.Init",
                "Frame buffer size is %u KB", sizeKb);

  _lineBuffer = new char[kNumCharsInLineBuffer];

  _baseLogDir = _context->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Cache, "");

  if (_context->GetExternalInterface() != nullptr)
  {
    auto helper = MakeAnkiEventUtil(*_context->GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::PerfMetricCommand>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::PerfMetricGetStatus>();
    // helper.SubscribeGameToEngine<MessageGameToEngineTag::ConnectToRobot>();
  }
  
  const auto& webService = _context->GetWebService();
  webService->RegisterRequestHandler("/perfmetric", PerfMetricWebServerHandler, this);
#endif
}


// This is called at the end of the tick
void PerfMetric::Update(const float tickDuration_ms,
                        const float tickFrequency_ms,
                        const float sleepDurationIntended_ms,
                        const float sleepDurationActual_ms)
{
#if ANKI_PERF_METRIC_ENABLED
  ANKI_CPU_PROFILE("PerfMetric::Update");

  ExecuteQueuedCommands();

  if (_isRecording)
  {
    FrameMetric& frame = _frameBuffer[_nextFrameIndex];

    frame._tickExecution_ms     = tickDuration_ms;
    frame._tickTotal_ms         = tickFrequency_ms;
    frame._tickSleepIntended_ms = sleepDurationIntended_ms;
    frame._tickSleepActual_ms   = sleepDurationActual_ms;

    const auto MsgHandler = _context->GetRobotManager()->GetMsgHandler();
    frame._messageCountRtE = MsgHandler->GetMessageCountRtE();
    frame._messageCountEtR = MsgHandler->GetMessageCountEtR();

    const auto UIMsgHandler = _context->GetExternalInterface();
    frame._messageCountGtE = UIMsgHandler->GetMessageCountGtE();
    frame._messageCountEtG = UIMsgHandler->GetMessageCountEtG();

    const auto VizManager = _context->GetVizManager();
    frame._messageCountViz = VizManager->GetMessageCountViz();

    Robot* robot = _context->GetRobotManager()->GetRobot();

    frame._batteryVoltage = robot == nullptr ? 0.0f : robot->GetBatteryComponent().GetBatteryVolts();
    
    if (robot != nullptr)
    {
      const auto& bc = robot->GetAIComponent().GetComponent<BehaviorComponent>();
      const auto& afc = bc.GetComponent<ActiveFeatureComponent>();
      frame._activeFeature = afc.GetActiveFeature();
      const auto& bsm = bc.GetComponent<BehaviorSystemManager>();
      strncpy(frame._behavior, bsm.GetTopBehaviorDebugLabel().c_str(), sizeof(frame._behavior));
      frame._behavior[FrameMetric::kBehaviorStringMaxSize - 1] = '\0'; // Ensure string is null terminated
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

  if (_waitMode)
  {
    if (_waitTimeToExpire != 0.0f)
    {
      if (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _waitTimeToExpire)
      {
        _waitMode = false;
        _waitTimeToExpire = 0.0f;
        // todo call into 'execute queued commands' (but for now this is fine because we'll do that at top of this function)
      }
    }
    else
    {
      if (--_waitTicksRemaining <= 0)
      {
        _waitMode = false;
        _waitTicksRemaining = 0;
        // todo call into 'execute queued commands' (but for now this is fine because we'll do that at top of this function)
      }
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
    PRINT_CH_INFO(kLogChannelName, "PerfMetric.Start",
                  "Interrupting recording already in progress; re-starting");
  }
  _isRecording = true;

  // Reset the buffer:
  _nextFrameIndex = 0;
  _bufferFilled = false;

  PRINT_CH_INFO(kLogChannelName, "PerfMetric.Start", "Recording started");

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

void PerfMetric::Dump(const DumpType dumpType, const bool dumpAll,
                      const std::string* fileName, std::string* resultStr) const
{
  if (FrameBufferEmpty())
  {
    PRINT_CH_INFO(kLogChannelName, "PerfMetric.Dump", "Nothing to dump; buffer is empty");
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
  Util::Stats::StatsAccumulator accMessageCountViz;
  Util::Stats::StatsAccumulator accBatteryVoltage;

  if (dumpAll)
  {
    DumpHeading(dumpType, dumpAll, fd, resultStr);
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
    accBatteryVoltage  += frame._batteryVoltage;

    static const std::string kFormatLineText = "%5i %8.3f %8.3f %8.3f %8.3f %8.3f    %5i %5i %5i %5i %5i %8.3f  %s  %s";
    static const std::string kFormatLineCSVText = "%5i,%8.3f,%8.3f,%8.3f,%8.3f,%8.3f,%5i,%5i,%5i,%5i,%5i,%8.3f,%s,%s";

#define LINE_DATA_VARS \
    frameIndex, frame._tickExecution_ms, frame._tickTotal_ms,\
    frame._tickSleepIntended_ms, frame._tickSleepActual_ms,\
    tickSleepOver_ms,\
    frame._messageCountRtE, frame._messageCountEtR,\
    frame._messageCountGtE, frame._messageCountEtG,\
    frame._messageCountViz,\
    frame._batteryVoltage, EnumToString(frame._activeFeature),\
    frame._behavior

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
#if defined(ANKI_PLATFORM_IOS)
                "IOS"
#elif defined(ANKI_PLATFORM_ANDROID)
                "ANDROID"
#elif defined(ANKI_PLATFORM_OSX)
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
      PRINT_CH_INFO(kLogChannelName, "PerfMetric.Dump", "%s", _lineBuffer);
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

  static const std::string kSummaryLineFormat = " %8.3f %8.3f %8.3f %8.3f %8.3f    %5.1f %5.1f %5.1f %5.1f %5.1f %8.3f\n";
  static const std::string kSummaryLineCSVFormat = ",%8.3f,%8.3f,%8.3f,%8.3f,%8.3f,%5.1f,%5.1f,%5.1f,%5.1f,%5.1f,%8.3f\n";

#define SUMMARY_LINE_VARS(StatCall)\
  accTickDuration.StatCall(), accTickTotal.StatCall(),\
  accSleepIntended.StatCall(), accSleepActual.StatCall(), accSleepOver.StatCall(),\
  accMessageCountRtE.StatCall(), accMessageCountEtR.StatCall(),\
  accMessageCountGtE.StatCall(), accMessageCountEtG.StatCall(), accMessageCountViz.StatCall(),\
  accBatteryVoltage.StatCall()

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

void PerfMetric::DumpHeading(const DumpType dumpType, const bool showBehaviorHeading,
                             FILE* fd, std::string* resultStr) const
{
  static const char* kHeading1 = "        Engine   Engine    Sleep    Sleep     Over      RtE   EtR   GtE   EtG   Viz  Battery";
  static const char* kHeading2 = "      Duration     Freq Intended   Actual    Sleep    Count Count Count Count Count  Voltage";
  static const char* kHeading3 = "  Active Feature/Behavior";
  static const char* kHeadingCSV1 = ",Engine,Engine,Sleep,Sleep,Over,RtE,EtR,GtE,EtG,Viz,Battery";
  static const char* kHeadingCSV2 = ",Duration,Freq,Intended,Actual,Sleep,Count,Count,Count,Count,Count,Voltage";
  static const char* kHeadingCSV3 = ",Active Feature,Behavior";

  switch (dumpType)
  {
    case DT_LOG:
      {
        PRINT_CH_INFO(kLogChannelName, "PerfMetric.Dump", "%s", kHeading1);
        PRINT_CH_INFO(kLogChannelName, "PerfMetric.Dump", "%s%s", kHeading2,
                      showBehaviorHeading ? kHeading3 : "");
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
  PRINT_CH_INFO(kLogChannelName, "PerfMetric.DumpFiles", "File written to %s",
                logFileNameText.c_str());

  // Write to CSV file
  Dump(DT_FILE_CSV, kDumpAll, &logFileNameCSV);
  PRINT_CH_INFO(kLogChannelName, "PerfMetric.DumpFiles", "File written to %s",
                logFileNameCSV.c_str());
}

void PerfMetric::WaitSeconds(const float seconds)
{
  if (_waitMode)
  {
    PRINT_CH_INFO(kLogChannelName, "PerfMetric.WaitSeconds", "Wait for seconds requested but already in wait mode");
  }
  _waitMode = true;
  _waitTicksRemaining = 0;
  _waitTimeToExpire = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + seconds;
  PRINT_CH_INFO(kLogChannelName, "PerfMetric.WaitSeconds", "Waiting for %f seconds", seconds);
}

void PerfMetric::WaitTicks(const int ticks)
{
  if (_waitMode)
  {
    PRINT_CH_INFO(kLogChannelName, "PerfMetric.WaitTicks", "Wait for ticks requested but already in wait mode");
  }
  _waitMode = true;
  _waitTicksRemaining = ticks;
  _waitTimeToExpire = 0.0f;
  PRINT_CH_INFO(kLogChannelName, "PerfMetric.WaitSeconds", "Wait for %i ticks", ticks);
}


void PerfMetric::OnRobotDisconnected()
{
  if (_autoRecord)
  {
    Stop();
  }

  DumpFiles();
}


// Parse commands out of the query string, and only if there are no errors,
// add them to the queue
int PerfMetric::ParseCommands(std::string& queryString)
{
  queryString = Util::StringToLower(queryString);
  
  std::vector<PerfMetricCommand> cmds;
  std::string current;
  
  while (!queryString.empty())
  {
    size_t amp = queryString.find('&');
    if (amp == std::string::npos)
    {
      current = queryString;
      queryString = "";
    }
    else
    {
      current = queryString.substr(0, amp);
      queryString = queryString.substr(amp + 1);
    }
  
    if (current == "start")
    {
      PerfMetricCommand cmd(START);
      cmds.push_back(cmd);
    }
    else if (current == "stop")
    {
      PerfMetricCommand cmd(STOP);
      cmds.push_back(cmd);
    }
    else if (current == "dumplog")
    {
      PerfMetricCommand cmd(DUMP_LOG, DT_LOG, false);
      cmds.push_back(cmd);
    }
    else if (current == "dumplogall")
    {
      PerfMetricCommand cmd(DUMP_LOG, DT_LOG, true);
      cmds.push_back(cmd);
    }
    else if (current == "dumpresponse")
    {
      PerfMetricCommand cmd(DUMP_RESPONSE_STRING, DT_LOG, false);
      cmds.push_back(cmd);
    }
    else if (current == "dumpresponseall")
    {
      PerfMetricCommand cmd(DUMP_RESPONSE_STRING, DT_LOG, true);
      cmds.push_back(cmd);
    }
    else if (current == "dumpfiles")
    {
      PerfMetricCommand cmd(DUMP_FILES);
      cmds.push_back(cmd);
    }
    else
    {
      // Commands that have arguments:
      static const std::string cmdKeywordWaitSeconds("waitseconds");
      static const std::string cmdKeywordWaitTicks("waitticks");

      if (current.substr(0, cmdKeywordWaitSeconds.size()) == cmdKeywordWaitSeconds)
      {
        std::string argumentValue = current.substr(cmdKeywordWaitSeconds.size());
        PerfMetricCommand cmd(WAIT_SECONDS);
        try
        {
          cmd._waitSeconds = std::stof(argumentValue);
        } catch (std::exception)
        {
          PRINT_CH_INFO(PerfMetric::kLogChannelName, "PerfMetric.ParseCommands", "Error parsing float argument in perfmetric command: %s", current.c_str());
          return 0;
        }
        cmds.push_back(cmd);
      }
      else if (current.substr(0, cmdKeywordWaitTicks.size()) == cmdKeywordWaitTicks)
      {
        std::string argumentValue = current.substr(cmdKeywordWaitTicks.size());
        PerfMetricCommand cmd(WAIT_TICKS);
        try
        {
          cmd._waitTicks = std::stoi(argumentValue);
        } catch (std::exception)
        {
          PRINT_CH_INFO(PerfMetric::kLogChannelName, "PerfMetric.ParseCommands", "Error parsing int argument in perfmetric command: %s", current.c_str());
          return 0;
        }
        cmds.push_back(cmd);
      }
      else
      {
        PRINT_CH_INFO(PerfMetric::kLogChannelName, "PerfMetric.ParseCommands", "Error parsing perfmetric command: %s", current.c_str());
        return 0;
      }
    }
  }
  
  // Now that there are no errors, add all parse commands to queue
  for (auto& cmd : cmds)
  {
    _queuedCommands.push(cmd);
  }
  return 1;
}
  

void PerfMetric::ExecuteQueuedCommands(std::string* resultStr)
{
  // Execute queued commands (unless/until we're in wait mode)
  while (!_waitMode && !_queuedCommands.empty())
  {
    const PerfMetricCommand cmd = _queuedCommands.front();
    _queuedCommands.pop();
    switch (cmd._command)
    {
      case START:
        Start();
        break;
      case STOP:
        Stop();
        break;
      case DUMP_LOG:
        Dump(DT_LOG, cmd._dumpAll, nullptr);
        break;
      case DUMP_RESPONSE_STRING:
        Dump(DT_RESPONSE_STRING, cmd._dumpAll, nullptr, resultStr);
        break;
      case DUMP_FILES:
        DumpFiles();
        break;
      case WAIT_SECONDS:
        WaitSeconds(cmd._waitSeconds);
        break;
      case WAIT_TICKS:
        WaitTicks(cmd._waitTicks);
        break;
    }
  }
}


template<>
void PerfMetric::HandleMessage(const ExternalInterface::PerfMetricCommand& msg)
{
  switch (msg.command)
  {
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
  const auto& extInt = _context->GetExternalInterface();
  extInt->Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
}


} // namespace Cozmo
} // namespace Anki

