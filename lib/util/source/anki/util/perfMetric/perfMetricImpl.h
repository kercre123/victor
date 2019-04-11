/**
 * File: perfMetricImpl
 *
 * Author: Paul Terry
 * Created: 11/03/2017
 *
 * Description: Lightweight performance metric recording: base class implementation
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/string/stringUtils.h"
#include "webServerProcess/src/webService.h"
#include <iomanip>

#define LOG_CHANNEL "PerfMetric"


namespace Anki {
namespace Vector {


const std::string PerfMetric::_logBaseFileName = "perfMetric_";

static const uint32_t kMsPerSecond = 1000;
static const uint32_t kMsPerMinute = 60 * kMsPerSecond;
static const uint32_t kMsPerHour = 60 * kMsPerMinute;
static const float kMsPerDay = kMsPerHour * 24.0f;

static const int kNumLinesInSummary = 4;


#if ANKI_PERF_METRIC_ENABLED

static int PerfMetricWebServerImpl(WebService::WebService::Request* request)
{
  auto* perfMetric = static_cast<PerfMetric*>(request->_cbdata);

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

  auto ws = perfMetric->GetWebService();
  const int returnCode = ws->ProcessRequestExternal(conn, cbdata, PerfMetricWebServerImpl, commands);

  return returnCode;
}

#endif

PerfMetric::PerfMetric()
  : _nextFrameIndex(0)
  , _bufferFilled(false)
  , _isRecording(false)
#if ANKI_PERF_METRIC_ENABLED
  , _autoRecord(true)
#else
  , _autoRecord(false)
#endif
  , _fileDir()
  , _dumpBuffer(nullptr)
  , _queuedCommands()
{
}

PerfMetric::~PerfMetric()
{
#if ANKI_PERF_METRIC_ENABLED
  delete[] _dumpBuffer;
#endif
}

void PerfMetric::OnShutdown()
{
  if (_isRecording)
  {
    Stop();
    if (_autoRecord)
    {
      DumpFiles();
      RemoveOldFiles();
    }
  }
}


void PerfMetric::InitInternal(Util::Data::DataPlatform* dataPlatform, WebService::WebService* webService)
{
#if ANKI_PERF_METRIC_ENABLED
  _dumpBuffer = new char[kSizeDumpBuffer];
  
  _fileDir = dataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, "")
             + "/perfMetricLogs";
  Util::FileUtils::CreateDirectory(_fileDir);
  
  _webService = webService;
  _webService->RegisterRequestHandler("/perfmetric", PerfMetricWebServerHandler, this);
#endif
}


void PerfMetric::UpdateWaitMode()
{
  if (_waitMode)
  {
    if (_waitTimeToExpire != 0.0f)
    {
      if (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _waitTimeToExpire)
      {
        _waitMode = false;
        _waitTimeToExpire = 0.0f;
      }
    }
    else
    {
      if (--_waitTicksRemaining <= 0)
      {
        _waitMode = false;
        _waitTicksRemaining = 0;
      }
    }
  }
}

void PerfMetric::Status(std::string* resultStr) const
{
  *resultStr += _isRecording ? "Recording" : "Stopped";
  const int numFrames = _bufferFilled ? kNumFramesInBuffer : _nextFrameIndex;
  *resultStr += ("," + std::to_string(numFrames) + "," + std::to_string(kNumFramesInBuffer) + "\n");
}

void PerfMetric::Start()
{
  if (_isRecording)
  {
    LOG_INFO("PerfMetric.Start", "Interrupting recording already in progress; re-starting");
  }
  _isRecording = true;

  // Reset the buffer:
  _nextFrameIndex = 0;
  _bufferFilled = false;

  // Save the current system time as 'milliseconds past midnight'
  // Note:  When running on webots pure simulator, note that the log time will
  // drift considerably because we're 'faking' tick time in webots and are not
  // running in real time
  using namespace std::chrono;
  const auto now = system_clock::now();
  const time_t tnow = system_clock::to_time_t(now);
  tm *date = std::localtime(&tnow);
  date->tm_hour = 0;
  date->tm_min = 0;
  date->tm_sec = 0;
  const auto midnight = system_clock::from_time_t(std::mktime(date));
  const auto timeSinceMidnight = now - midnight;
  const auto msSinceMidnight = duration_cast<milliseconds>(timeSinceMidnight);
  _firstFrameTime = msSinceMidnight.count();

  LOG_INFO("PerfMetric.Start", "Recording started");
}

void PerfMetric::Stop()
{
  if (!_isRecording)
  {
    LOG_INFO("Perfmetric.Stop", "Recording was already stopped");
  }
  else
  {
    _isRecording = false;
    LOG_INFO("Perfmetric.Stop", "Recording stopped");
  }
}


void PerfMetric::Dump(const DumpType dumpType, const bool dumpAll,
                      const std::string* fileName, std::string* resultStr)
{
  ANKI_CPU_PROFILE("PerfMetric::Dump");

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

  if (dumpAll)
  {
    if (dumpType == DT_RESPONSE_STRING)
    {
      // When dumping all to HTTP response string, also send back the current frame buffer index:
      // First, return the new frame buffer index (what we are catching up to)
      *resultStr += ("Next frame buffer index:" + std::to_string(_nextFrameIndex) + "\n");
    }

    static const bool kDumpLine2Extra = true;
    DumpHeading(dumpType, kDumpLine2Extra, fd, resultStr);
  }

  int frameBufferIndex = _bufferFilled ? _nextFrameIndex : 0;
  const int numFrames = _bufferFilled ? kNumFramesInBuffer : _nextFrameIndex;
  float timeAtStartOfFrame = _firstFrameTime;
  Util::Stats::StatsAccumulator accTickDuration;
  Util::Stats::StatsAccumulator accTickTotal;
  Util::Stats::StatsAccumulator accSleepIntended;
  Util::Stats::StatsAccumulator accSleepActual;
  Util::Stats::StatsAccumulator accSleepOver;
  InitDumpAccumulators();

  for (int frameIndex = 0; frameIndex < numFrames; frameIndex++)
  {
    // Decode time of start of frame in format HH:MM:SS.MMM
    auto timeSinceMidnight_ms = static_cast<uint32_t>(timeAtStartOfFrame);
    const auto h = timeSinceMidnight_ms / kMsPerHour;
    timeSinceMidnight_ms -= (h * kMsPerHour);
    const auto m = timeSinceMidnight_ms / kMsPerMinute;
    timeSinceMidnight_ms -= (m * kMsPerMinute);
    const auto s = timeSinceMidnight_ms / kMsPerSecond;
    timeSinceMidnight_ms -= (s * kMsPerSecond);
    std::ostringstream stringStream;
    stringStream << std::setfill('0') << std::setw(2) << h
                 << ":" << std::setw(2) << m
                 << ":" << std::setw(2) << s
                 << "." << std::setw(3) << timeSinceMidnight_ms;
    const auto frameTimeString = stringStream.str();

    const FrameMetric& frame = UpdateDumpAccumulators(frameBufferIndex);

    // This stat is calculated rather than stored
    const float tickSleepOver_ms = frame._tickSleepActual_ms - frame._tickSleepIntended_ms;

    accTickDuration    += frame._tickExecution_ms;
    accTickTotal       += frame._tickTotal_ms;
    accSleepIntended   += frame._tickSleepIntended_ms;
    accSleepActual     += frame._tickSleepActual_ms;
    accSleepOver       += tickSleepOver_ms;

    if (dumpAll)
    {
#define LINE_DATA_VARS \
      frameTimeString.c_str(), frameIndex, \
      frame._tickExecution_ms, frame._tickTotal_ms,\
      frame._tickSleepIntended_ms, frame._tickSleepActual_ms, tickSleepOver_ms

      static const char* kFormatLine = "%s %5i %8.3f %8.3f %8.3f %8.3f %8.3f";
      static const char* kFormatLineCSV = "%s,%i,%.3f,%.3f,%.3f,%.3f,%.3f";

      int strSize = snprintf(_dumpBuffer, kSizeDumpBuffer,
                             dumpType == DT_FILE_CSV ? kFormatLineCSV : kFormatLine,
                             LINE_DATA_VARS);

      // Append additional data from derived class
      static const bool kGraphableDataOnly = false;
      strSize += AppendFrameData(dumpType, frameBufferIndex, strSize, kGraphableDataOnly);

      DumpLine(dumpType, strSize, fd, resultStr);
    }

    timeAtStartOfFrame = IncrementFrameTime(timeAtStartOfFrame, frame._tickTotal_ms);

    if (++frameBufferIndex >= kNumFramesInBuffer)
    {
      frameBufferIndex = 0;
    }
  }

  const float totalTime_sec = accTickTotal.GetVal() * 0.001f;
  const int strSize = snprintf(_dumpBuffer, kSizeDumpBuffer,
           "Summary:  (%s build; %s; %i ticks; %.3f seconds total)\n",
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
  DumpLine(dumpType, strSize, fd, resultStr);

  static const bool kDumpLine2Extra = false;
  DumpHeading(dumpType, kDumpLine2Extra, fd, resultStr);

  for (int lineIndex = 0; lineIndex < kNumLinesInSummary; lineIndex++)
  {
    static const char* kSummaryLineFormat = "%18s %8.3f %8.3f %8.3f %8.3f %8.3f";
    static const char* kSummaryLineCSVFormat = ",%s,%8.3f,%8.3f,%8.3f,%8.3f,%8.3f";

#define SUMMARY_LINE_VARS(StatCall)\
    accTickDuration.StatCall(), accTickTotal.StatCall(),\
    accSleepIntended.StatCall(), accSleepActual.StatCall(), accSleepOver.StatCall()

    int strSize = 0;
#define OUTPUT_SUMMARY_LINE(StatDescription, StatCall)\
    strSize += snprintf(&_dumpBuffer[strSize], kSizeDumpBuffer - strSize,\
                        dumpType == DT_FILE_CSV ? kSummaryLineCSVFormat : kSummaryLineFormat,\
                        StatDescription, SUMMARY_LINE_VARS(StatCall));
    switch (lineIndex)
    {
      case 0:   OUTPUT_SUMMARY_LINE("Min:",  GetMin);   break;
      case 1:   OUTPUT_SUMMARY_LINE("Max:",  GetMax);   break;
      case 2:   OUTPUT_SUMMARY_LINE("Mean:", GetMean);  break;
      case 3:   OUTPUT_SUMMARY_LINE("Std:",  GetStd);   break;
    }

    strSize += AppendSummaryData(dumpType, strSize, lineIndex);

    DumpLine(dumpType, strSize, fd, resultStr);
  }

  if (dumpType == DT_FILE_TEXT || dumpType == DT_FILE_CSV)
  {
    fclose(fd);
  }
}


void PerfMetric::DumpFramesSince(const int firstFrameBufferIndex, std::string* resultStr)
{
  ANKI_CPU_PROFILE("PerfMetric::DumpFramesSince");

  int frameBufferIndex = firstFrameBufferIndex;
  int numFrames = 0;
  if (firstFrameBufferIndex < 0)
  {
    frameBufferIndex = _bufferFilled ? _nextFrameIndex : 0;
    numFrames = _bufferFilled ? kNumFramesInBuffer : _nextFrameIndex;
  }
  else
  {
    if (firstFrameBufferIndex < _nextFrameIndex)
    {
      numFrames = _nextFrameIndex - firstFrameBufferIndex;
    }
    else if (firstFrameBufferIndex > _nextFrameIndex)
    {
      DEV_ASSERT(_bufferFilled, "first frame requested is beyond buffer that's been recorded");
      numFrames = kNumFramesInBuffer - (firstFrameBufferIndex - _nextFrameIndex);
    }
  }

  // First, return the new frame buffer index (what we are catching up to), and the number of frames to follow
  *resultStr += (std::to_string(_nextFrameIndex) + "," + std::to_string(numFrames) + "\n");
  
  // Now return data for each frame
  for ( ; numFrames > 0; numFrames--)
  {
    const FrameMetric& frame = GetBaseFrame(frameBufferIndex);
    // This stat is calculated rather than stored
    const float tickSleepOver_ms = frame._tickSleepActual_ms - frame._tickSleepIntended_ms;

#define SINCE_LINE_DATA_VARS \
    frame._tickExecution_ms, frame._tickTotal_ms,\
    frame._tickSleepIntended_ms, frame._tickSleepActual_ms, tickSleepOver_ms

    // Note that we omit the first two fields normally sent (log time and frame buffer index)
    static const char* kFormatLineCSV = ",,%.3f,%.3f,%.3f,%.3f,%.3f";

    int strSize = snprintf(_dumpBuffer, kSizeDumpBuffer, kFormatLineCSV, SINCE_LINE_DATA_VARS);

    // Append additional data from derived class
    static const bool kGraphableDataOnly = true;  // To exclude behavior strings in engine, e.g.
    strSize += AppendFrameData(DT_FILE_CSV, frameBufferIndex, strSize, kGraphableDataOnly);

    *resultStr += _dumpBuffer;

    if (++frameBufferIndex >= kNumFramesInBuffer)
    {
      frameBufferIndex = 0;
    }
  }
}


void PerfMetric::DumpHeading(const DumpType dumpType, const bool dumpLine2Extra,
                             FILE* fd, std::string* resultStr) const
{
  const int head1Len = snprintf(_dumpBuffer, kSizeDumpBuffer, "%s\n",
                                dumpType == DT_FILE_CSV ? _headingLine1CSV : _headingLine1);
  DumpLine(dumpType, head1Len, fd, resultStr);

  const int head2Len = snprintf(_dumpBuffer, kSizeDumpBuffer, "%s%s\n",
                                dumpType == DT_FILE_CSV ? _headingLine2CSV : _headingLine2,
                                dumpLine2Extra ?
                                dumpType == DT_FILE_CSV ? _headingLine2ExtraCSV : _headingLine2Extra : "");
  DumpLine(dumpType, head2Len, fd, resultStr);
}


void PerfMetric::DumpFiles()
{
  if (FrameBufferEmpty())
  {
    LOG_INFO("PerfMetric.DumpFiles", "Nothing to dump; buffer is empty");
    return;
  }

  using namespace std::chrono;
  const auto now = system_clock::now();
  const auto now_time = system_clock::to_time_t(now);
  const auto tm = *std::localtime(&now_time);

  std::ostringstream sstr;
  sstr << _fileDir << "/" << _logBaseFileName << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
#if defined(NDEBUG)
  sstr << "_R_";
#else
  sstr << "_D_";
#endif
  sstr << _fileNameSuffix;
  const std::string logFileNameText = sstr.str() + ".txt";
  const std::string logFileNameCSV  = sstr.str() + ".csv";

  static const bool kDumpAll = true;

  // Write to text file
  Dump(DT_FILE_TEXT, kDumpAll, &logFileNameText);
  LOG_INFO("PerfMetric.DumpFiles", "File written to %s", logFileNameText.c_str());

  // Write to CSV file
  Dump(DT_FILE_CSV, kDumpAll, &logFileNameCSV);
  LOG_INFO("PerfMetric.DumpFiles", "File written to %s", logFileNameCSV.c_str());
}


void PerfMetric::DumpLine(const DumpType dumpType,
                          int dumpBufferOffset,
                          FILE* fd,
                          std::string* resultStr) const
{
  switch (dumpType)
  {
    case DT_LOG:
      {
#ifdef SIMULATOR
        // In webots, the log system adds a newline for us, unlike in VicOS, so we strip it
        // off here rather than have code below to append a newline in the VicOS version
        if (_dumpBuffer[dumpBufferOffset - 1] == '\n')
        {
          _dumpBuffer[--dumpBufferOffset] = 0;
        }
#endif
        LOG_INFO("PerfMetric.Dump", "%s", _dumpBuffer);
      }
      break;
    case DT_RESPONSE_STRING:  // Intentional fall-through
    case DT_FILE_TEXT:        // Intentional fall-through
    case DT_FILE_CSV:
      {
        if (dumpType == DT_RESPONSE_STRING)
        {
          if (resultStr)
          {
            *resultStr += _dumpBuffer;
          }
        }
        else
        {
          fwrite(_dumpBuffer, 1, dumpBufferOffset, fd);
        }
      }
      break;
  }
}


void PerfMetric::RemoveOldFiles() const
{
  static const bool kUseFullPath = true;
  auto fileList = Util::FileUtils::FilesInDirectory(_fileDir, kUseFullPath);

  static const int kMaxNumFilesToKeep = 50;
  const int numFilesToRemove = static_cast<int>(fileList.size()) - kMaxNumFilesToKeep;
  if (numFilesToRemove > 0)
  {
    // Since the filenames are structured with date/time in them, we can
    // sort alphabetically to get a date/time-sorted list
    std::sort(fileList.begin(), fileList.end());
    for (int i = 0; i < numFilesToRemove; i++)
    {
      Util::FileUtils::DeleteFile(fileList[i]);
    }
  }
}


float PerfMetric::IncrementFrameTime(float msSinceMidnight, const float msToAdd) const
{
  msSinceMidnight += msToAdd;
  if (msSinceMidnight >= kMsPerDay)
  {
    msSinceMidnight -= kMsPerDay;
    if (msSinceMidnight < 0.0f) // Just in case
    {
      msSinceMidnight = 0.0f;
    }
  }
  return msSinceMidnight;
}


void PerfMetric::WaitSeconds(const float seconds)
{
  if (_waitMode)
  {
    LOG_INFO("PerfMetric.WaitSeconds", "Wait for seconds requested but already in wait mode");
  }
  _waitMode = true;
  _waitTicksRemaining = 0;
  _waitTimeToExpire = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + seconds;
  LOG_INFO("PerfMetric.WaitSeconds", "Waiting for %f seconds", seconds);
}

void PerfMetric::WaitTicks(const int ticks)
{
  if (_waitMode)
  {
    LOG_INFO("PerfMetric.WaitTicks", "Wait for ticks requested but already in wait mode");
  }
  _waitMode = true;
  _waitTicksRemaining = ticks;
  _waitTimeToExpire = 0.0f;
  LOG_INFO("PerfMetric.WaitSeconds", "Wait for %i ticks", ticks);
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
    
    if (current == "status")
    {
      PerfMetricCommand cmd(STATUS);
      cmds.push_back(cmd);
    }
    else if (current == "start")
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
      PerfMetricCommand cmd(DUMP_RESPONSE_STRING, DT_RESPONSE_STRING, false);
      cmds.push_back(cmd);
    }
    else if (current == "dumpresponseall")
    {
      PerfMetricCommand cmd(DUMP_RESPONSE_STRING, DT_RESPONSE_STRING, true);
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
      static const std::string cmdKeywordDumpResponseSince("dumpresponsesince");

      if (current.substr(0, cmdKeywordWaitSeconds.size()) == cmdKeywordWaitSeconds)
      {
        std::string argumentValue = current.substr(cmdKeywordWaitSeconds.size());
        PerfMetricCommand cmd(WAIT_SECONDS);
        try
        {
          cmd._waitSeconds = std::stof(argumentValue);
        } catch (std::exception)
        {
          LOG_INFO("PerfMetric.ParseCommands", "Error parsing float argument in perfmetric command: %s", current.c_str());
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
          LOG_INFO("PerfMetric.ParseCommands", "Error parsing int argument in perfmetric command: %s", current.c_str());
          return 0;
        }
        cmds.push_back(cmd);
      }
      else if (current.substr(0, cmdKeywordDumpResponseSince.size()) == cmdKeywordDumpResponseSince)
      {
        std::string argumentValue = current.substr(cmdKeywordDumpResponseSince.size());
        PerfMetricCommand cmd(DUMP_RESPONSE_CSV_SINCE);
        try
        {
          cmd._frameIndex = std::stoi(argumentValue);
        } catch (std::exception)
        {
          LOG_INFO("PerfMetric.ParseCommands", "Error parsing int argument in perfmetric command: %s", current.c_str());
          return 0;
        }
        cmds.push_back(cmd);
      }
      else
      {
        LOG_INFO("PerfMetric.ParseCommands", "Error parsing perfmetric command: %s", current.c_str());
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
      case STATUS:
        Status(resultStr);
        break;
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
      case DUMP_RESPONSE_CSV_SINCE:
        DumpFramesSince(cmd._frameIndex, resultStr);
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


} // namespace Vector
} // namespace Anki
