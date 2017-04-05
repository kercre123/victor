/// Implementation for printTrace functionality

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/tracePrinter.h"
#include "debug/devLoggingSystem.h"
#include "util/UUID/UUID.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/base64.h"
#include "util/logging/logging.h"
#include <fstream>
#include <stdarg.h>
#include <stdlib.h>

#if USE_DAS
#include <DAS/DAS.h>
#include <DAS/DASPlatform.h>
#endif


namespace Anki {
namespace Cozmo {

using KVV = std::vector<std::pair<const char*, const char*>>;
  
const std::string TracePrinter::_kUnknownTraceName   = "UnknownTraceName";
const std::string TracePrinter::_kUnknownTraceFormat = "Unknown trace format [%d] with %d parameters";
const std::string TracePrinter::_kRobotNamePrefix    = "RobotFirmware.";
  
// constant calculated from ram size in robot/espressif/app/include/driver/crash.h, which is not in app search path.
static const int kMaxCrashLogs = 4;

TracePrinter::TracePrinter(Robot* robot)
  : _printThreshold(RobotInterface::LogLevel::ANKI_LOG_LEVEL_DEBUG)
  , _lastLogRequested(kMaxCrashLogs)
  , _robot(robot)
{
  // Listen to some messages from the robot
  RobotInterface::MessageHandler* messageHandler = _robot->GetContext()->GetRobotManager()->GetMsgHandler();
  if (messageHandler)
  {
    using localHandlerType= void(TracePrinter::*)(const AnkiEvent<RobotInterface::RobotToEngine>&);
    auto robotId = _robot->GetID();
    auto& signalHandles = GetSignalHandles();
    auto doRobotSubscribe = [this, &signalHandles, robotId, messageHandler] (RobotInterface::RobotToEngineTag tagType, localHandlerType handler)
    {
      signalHandles.push_back(messageHandler->Subscribe(robotId, tagType, std::bind(handler, this, std::placeholders::_1)));
    };
    
    doRobotSubscribe(RobotInterface::RobotToEngineTag::trace,         &TracePrinter::HandleTrace);
    doRobotSubscribe(RobotInterface::RobotToEngineTag::crashReport,   &TracePrinter::HandleCrashReport);
    doRobotSubscribe(RobotInterface::RobotToEngineTag::wifiFlashID,   &TracePrinter::HandleWiFiFlashID);
  }
  
  // Listen to some messages from the engine
  IExternalInterface* externalInterface = _robot->GetContext()->GetExternalInterface();
  if (externalInterface)
  {
    using namespace ExternalInterface;
    
    auto helper = MakeAnkiEventUtil(*externalInterface, *this, GetSignalHandles());
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotConnectionResponse>();
  }
  
  Util::Data::DataPlatform* dp = _robot->GetContextDataPlatform();
  if (dp) {
    Json::Value jsonDict;
    const std::string jsonFilename = "config/basestation/AnkiLogStringTables.json";
    bool success = dp->readAsJson(Util::Data::Scope::Resources, jsonFilename, jsonDict);
    if (!success)
    {
      PRINT_NAMED_ERROR("Robot.AnkiLogStringTablesNotFound",
                        "Robot PrintTrace string table Json config file %s not found.",
                        jsonFilename.c_str());
    }
    
    const Json::Value jsonNameTable   = jsonDict["nameTable"];
    for (Json::ValueConstIterator itr = jsonNameTable.begin(); itr != jsonNameTable.end(); itr++) {
      _nameTable.insert(std::pair<const int, const std::string>(atoi(itr.key().asString().c_str()), itr->asString()));
    }
    
    const Json::Value jsonFormatTable = jsonDict["formatTable"];
    for (Json::ValueConstIterator itr = jsonFormatTable.begin(); itr != jsonFormatTable.end(); itr++) {
      const int key = atoi(itr.key().asString().c_str());
      const std::string fmt = (*itr)[0].asString();
      const int nargs = (*itr)[1].asInt();
      _formatTable.insert(std::pair<const int, const FormatInfo>(key, FormatInfo(fmt, nargs)));
    }
  }
}

void TracePrinter::HandleTrace(const AnkiEvent<RobotInterface::RobotToEngine>& message) {
  ANKI_CPU_PROFILE("TracePrinter::HandleTrace");
  const RobotInterface::PrintTrace& trace = message.GetData().Get_trace();
  if (trace.level >= _printThreshold) {
    std::string tmpdata;
    KVV keyValuePairs;
    if (trace.value.size() == 1)
    {
      tmpdata = std::to_string(trace.value[0]);
      KVV::value_type kvp(DDATA, tmpdata.c_str());
      keyValuePairs.push_back(kvp);
    }
    const std::string name = _kRobotNamePrefix + GetName(trace.name);
    const std::string mesg = GetFormatted(trace);
    switch (trace.level)
    {
      case RobotInterface::LogLevel::ANKI_LOG_LEVEL_DEBUG:
      case RobotInterface::LogLevel::ANKI_LOG_LEVEL_PRINT:
      {
        Util::sChanneledDebugF(DEFAULT_CHANNEL_NAME, name.c_str(), keyValuePairs, "%s", mesg.c_str()); //< Necessary because format must be a string literal
        break;
      }
      case RobotInterface::LogLevel::ANKI_LOG_LEVEL_INFO:
      {
        Util::sChanneledInfoF(DEFAULT_CHANNEL_NAME, name.c_str(), keyValuePairs, "%s", mesg.c_str());
        break;
      }
      case RobotInterface::LogLevel::ANKI_LOG_LEVEL_EVENT:
      {
        Util::sEventF(name.c_str(), keyValuePairs, "%s", mesg.c_str());
        break;
      }
      case RobotInterface::LogLevel::ANKI_LOG_LEVEL_WARN:
      {
        Util::sWarningF(name.c_str(), keyValuePairs, "%s", mesg.c_str());
        break;
      }
      case RobotInterface::LogLevel::ANKI_LOG_LEVEL_ASSERT:
      case RobotInterface::LogLevel::ANKI_LOG_LEVEL_ERROR:
      {
        Util::sErrorF(name.c_str(), keyValuePairs, "%s", mesg.c_str());
        break;
      }
    }
  }
}

void TracePrinter::HandleCrashReport(const AnkiEvent<RobotInterface::RobotToEngine>& message) {
  ANKI_CPU_PROFILE("TracePrinter::HandleCrashReport");
  const RobotInterface::CrashReport& report = message.GetData().Get_crashReport();
  if (report.errorCode || report.dump.size()) {
    LOG_EVENT("RobotFirmware.CrashReport", "Firmware crash report received: %s, %x", CrashSourceToString(report.which), report.errorCode);
    if (DevLoggingSystem::GetInstance() != NULL) {
      
      const std::string fileExt = ".log"; // Only .log files are archived and transmitted
      const std::string dumpFileNameDir = DevLoggingSystem::GetInstance()->GetDevLoggingBaseDirectory() + "/robotFirmware/";
      
      std::ostringstream dumpFilepathStream;
      dumpFilepathStream << "crash_";
      dumpFilepathStream << std::to_string(Util::EnumToUnderlying(report.which)) << "_";
      dumpFilepathStream << std::hex << report.errorCode << "_";
      dumpFilepathStream << std::dec << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
      const std::string dumpFileNameBase = dumpFilepathStream.str();
      
      // Find a filepath for the crash dump that doesn't already exist and set up directories if needed
      std::string dumpFileName = dumpFileNameBase + fileExt;
      std::string dumpFilepath = dumpFileNameDir + dumpFileName;
      uint32_t crashCounter = 0;
      while (Util::FileUtils::FileExists(dumpFilepath)) {
        dumpFileName = dumpFileNameBase + "-" + std::to_string(++crashCounter) + fileExt;
        dumpFilepath = dumpFileNameDir + dumpFileName;
      }
      Util::FileUtils::CreateDirectory(dumpFilepath, true, true);
      
      // Copy the crash data to a local vector and convert from 4-byte words to chars
      const size_t dumpSize = report.dump.size() * sizeof(report.dump[0]);
      std::vector<uint8_t> crashData = std::vector<uint8_t>(dumpSize);
      const uint8_t* reportDataBegin = reinterpret_cast<const uint8_t*>(report.dump.data());
      std::copy(reportDataBegin, reportDataBegin + dumpSize, crashData.data());
      
      if(Util::FileUtils::WriteFile(dumpFilepath, crashData)) {
        LOG_EVENT("RobotFirmware.CrashReport.Written", "Firmware crash report written to \"%s\"", dumpFileName.c_str());
      }
      else {
        PRINT_NAMED_ERROR("RobotFirmware.CrashReport.FailedToWrite", "Couldn't write report to file \"%s\"", dumpFileName.c_str());
      }
    }
    if (report.errorCode == 0 && report.dump.size() > 0) {
      const unsigned char* const dump_bytes = reinterpret_cast<const unsigned char* const>(report.dump.data());
      const size_t dumpSize = report.dump.size() * sizeof(report.dump[0]);
      std::string base64Crash = Util::base64_encode(dump_bytes, dumpSize);
      Util::sErrorF("RobotFirmware.CrashReport.Data", {{DDATA, base64Crash.c_str()}},
                    "Crash Dump Type %s", CrashSourceToString(report.which));
    }
    else
    {
      Util::sErrorF("RobotFirmware.CrashReport.code",
                    {{DDATA, std::to_string(report.errorCode).c_str()}},
                    "errorCode = %d", report.errorCode);
    }
  }
  if (_lastLogRequested < kMaxCrashLogs)
  {
      _robot->SendRobotMessage<RobotInterface::RequestCrashReports>(_lastLogRequested++);
  }
}

void TracePrinter::HandleWiFiFlashID(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  ANKI_CPU_PROFILE("TracePrinter::HandleCrashReport");
  const RobotInterface::WiFiFlashID& fidMsg = message.GetData().Get_wifiFlashID();
  const std::string idAsString = std::to_string(fidMsg.chip_id);
  Util::sEventF("hardware.wifi.flash_id", {{DDATA, idAsString.c_str()}}, "chip_id=%d", fidMsg.chip_id);
}

const std::string& TracePrinter::GetName(const int nameId) const {
  const IntStringMap::const_iterator it = _nameTable.find(nameId);
  if (it == _nameTable.end()) return _kUnknownTraceName;
  else return it->second;
}

std::string TracePrinter::GetFormatted(const RobotInterface::PrintTrace& trace) const {
  char pbuf[512];
  char fbuf[64];
  const IntFormatMap::const_iterator it = _formatTable.find(trace.stringId);
  if (it == _formatTable.end()) {
    snprintf(pbuf, sizeof(pbuf), _kUnknownTraceFormat.c_str(), trace.stringId, trace.value.size());
    return pbuf;
  }
  else {
    const FormatInfo& fi(it->second);
    const int nargs = fi.second;
    if (nargs != trace.value.size()) {
      snprintf(pbuf, sizeof(pbuf), "Trace nargs mismatch. Expected %d values but got %d for format string (%d) \"%s\"",
               nargs, (int)trace.value.size(), trace.stringId, fi.first.c_str());
      return pbuf;
    }
    else if (nargs == 0)
    {
      return fi.first;
    }
    else {
      int index = 0;
      int argInd = 0;
      const char* fmtPtr = fi.first.c_str();
      int subFmtInd = -1;
      while ((*fmtPtr != 0) && (index < ((int)sizeof(pbuf)-1)) && (subFmtInd < ((int)sizeof(fbuf)-1)))
      {
        if (subFmtInd >= 0)
        {
          switch(fmtPtr[subFmtInd])
          {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '.':
            {
              fbuf[subFmtInd] = fmtPtr[subFmtInd];
              subFmtInd++;
              continue;
            }
            case '%':
            {
              pbuf[index++] = '%';
              fmtPtr += subFmtInd + 1;
              subFmtInd = -1;
              continue;
            }
            case 'd':
            case 'i':
            case 'x':
            case 'f':
            {
              fbuf[subFmtInd] = fmtPtr[subFmtInd];
              fbuf[subFmtInd+1] = '\0';
              if (fmtPtr[subFmtInd] == 'f')
              {
                const float fltArg = *(reinterpret_cast<const float*>(&trace.value[argInd]));
                index += snprintf(pbuf + index, sizeof(pbuf)-index, fbuf, fltArg);
              }
              else
              {
                index += snprintf(pbuf + index, sizeof(pbuf)-index, fbuf, trace.value[argInd]);
              }
              fmtPtr += subFmtInd + 1;
              subFmtInd = -1;
              argInd++;
              continue;
            }
            default: // So copy it over and return to normal operation
            {
              pbuf[index++] = *(fmtPtr++);
              subFmtInd = -1;
              continue;
            }
          }
        }
        else if (*fmtPtr == '%')
        {
          fbuf[0] = '%';
          subFmtInd = 1;
          continue;
        }
        else
        {
          pbuf[index++] = *(fmtPtr++);
        }
      }
      pbuf[index] = '\0';
      return pbuf;
    }
  }
}

template<>
void TracePrinter::HandleMessage(const ExternalInterface::RobotConnectionResponse& msg)
{
  if (msg.result == RobotConnectionResult::Success) {
    UUIDBytes appRunBytes;
    memset(&appRunBytes, 0xff, sizeof(UUIDBytes));
    #if USE_DAS
    {
      const DAS::IDASPlatform* platform = DASGetPlatform();
      if (nullptr != platform)
      {
        UUIDBytesFromString(&appRunBytes, platform->GetAppRunId());
      }
    }
    #endif
    _robot->SendRobotMessage<RobotInterface::SetAppRunID>(appRunBytes.bytes, 16);
    
    // Request the first crash report
    if (_lastLogRequested >= kMaxCrashLogs) {
      _lastLogRequested = 0;
      _robot->SendRobotMessage<RobotInterface::RequestCrashReports>(_lastLogRequested++);
    }
  }
}

} // Namespace Cozmo
} // Namespace Anki
