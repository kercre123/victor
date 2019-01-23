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


#if ANKI_PERF_METRIC_ENABLED

static int PerfMetricWebServerImpl(WebService::WebService::Request* request)
{
  auto* perfMetric = static_cast<PerfMetric*>(request->_cbdata);
  //LOG_INFO("PerfMetric.PerfMetricWebServerImpl", "Query string: %s", request->_param1.c_str());
  
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
  , _lineBuffer(nullptr)
  , _queuedCommands()
{
}

PerfMetric::~PerfMetric()
{
#if ANKI_PERF_METRIC_ENABLED
  delete[] _lineBuffer;
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
  _lineBuffer = new char[kNumCharsInLineBuffer];
  
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
  const int numFrames  = _bufferFilled ? kNumFramesInBuffer : _nextFrameIndex;
  *resultStr += ",";
  *resultStr += std::to_string(numFrames);
  *resultStr += "\n";
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

void PerfMetric::DumpFiles() const
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
  LOG_INFO("PerfMetric.DumpFiles", "File written to %s",
           logFileNameText.c_str());

  // Write to CSV file
  Dump(DT_FILE_CSV, kDumpAll, &logFileNameCSV);
  LOG_INFO("PerfMetric.DumpFiles", "File written to %s", logFileNameCSV.c_str());
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
