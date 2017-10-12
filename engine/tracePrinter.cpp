/// Implementation for printTrace functionality

#include "anki/common/basestation/jsonTools.h"
#include "engine/ankiEventUtil.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"
#include "engine/tracePrinter.h"
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

TracePrinter::TracePrinter(Robot* robot)
  : _printThreshold(RobotInterface::LogLevel::ANKI_LOG_LEVEL_DEBUG)
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
  }
  
  Util::Data::DataPlatform* dp = _robot->GetContextDataPlatform();
  if (dp) {
    Json::Value jsonDict;
    const std::string jsonFilename = "config/engine/AnkiLogStringTables.json";
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


} // Namespace Cozmo
} // Namespace Anki
