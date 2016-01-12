/// Implementation for printTrace functionality

#include "anki/cozmo/basestation/tracePrinter.h"
#include "anki/common/basestation/jsonTools.h"
#include "util/logging/logging.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

namespace Anki {
namespace Cozmo {

const std::string TracePrinter::UnknownTraceName   = "Unknown trace name";
const std::string TracePrinter::UnknownTraceFormat = "Unknown trace format with %d parameters";

TracePrinter::TracePrinter(Util::Data::DataPlatform* dp):
  printThreshold(RobotInterface::LogLevel::ANKI_LOG_LEVEL_DEBUG) {
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
  for (Json::ValueIterator itr = jsonNameTable.begin(); itr != jsonNameTable.end(); itr++) {
    nameTable.insert(std::pair<const int, const std::string>(atoi(itr.key().asString().c_str()), itr->asString()));
  }
  
  const Json::Value jsonFormatTable = jsonDict["formatTable"];
  for (Json::ValueIterator itr = jsonFormatTable.begin(); itr != jsonFormatTable.end(); itr++) {
    const int key = atoi(itr.key().asString().c_str());
    const std::string fmt = (*itr)[0].asString();
    const int nargs = (*itr)[1].asInt();
    formatTable.insert(std::pair<const int, const FormatInfo>(key, FormatInfo(fmt, nargs)));
  }
}

void TracePrinter::HandleTrace(const AnkiEvent<RobotInterface::RobotToEngine>& message) const {
  const RobotInterface::PrintTrace& trace = message.GetData().Get_trace();
  if (trace.level >= printThreshold) {
    const std::string name = GetName(trace.name);
    const std::string mesg = GetFormatted(trace);
    printf("ROBOT-%s %s: %s\n", RobotInterface::EnumToString(trace.level), name.c_str(), mesg.c_str());
  }
}

const std::string& TracePrinter::GetName(const int nameId) const {
  const IntStringMap::const_iterator it = nameTable.find(nameId);
  if (it == nameTable.end()) return UnknownTraceName;
  else return it->second;
}

std::string TracePrinter::GetFormatted(const RobotInterface::PrintTrace& trace) const {
  char pbuf[512];
  const IntFormatMap::const_iterator it = formatTable.find(trace.stringId);
  if (it == formatTable.end()) {
    snprintf(pbuf, sizeof(pbuf), UnknownTraceFormat.c_str(), trace.value.size());
    return pbuf;
  }
  else {
    const FormatInfo& fi(it->second);
    const int nargs = fi.second;
    if (nargs != trace.value.size()) {
      snprintf(pbuf, sizeof(pbuf), "Trace nargs missmatch. Expected %d values but got %d for format string \"%s\"",
               nargs, (int)trace.value.size(), fi.first.c_str());
      return pbuf;
    }
    else {
      // Switch on nargs because no safe way to convert vector back to va_list for sprintf, the max is 12 so it's not too bad
      switch (nargs) {
        case 0:
          return fi.first;
        case 1:
          snprintf(pbuf, sizeof(pbuf), fi.first.c_str(), trace.value[0]);
          return pbuf;
        case 2:
          snprintf(pbuf, sizeof(pbuf), fi.first.c_str(), trace.value[0], trace.value[1]);
          return pbuf;
        case 3:
          snprintf(pbuf, sizeof(pbuf), fi.first.c_str(), trace.value[0], trace.value[1], trace.value[3]);
          return pbuf;
        case 4:
          snprintf(pbuf, sizeof(pbuf), fi.first.c_str(), trace.value[0], trace.value[1], trace.value[2], trace.value[3]);
          return pbuf;
        case 5:
          snprintf(pbuf, sizeof(pbuf), fi.first.c_str(), trace.value[0], trace.value[1], trace.value[2], trace.value[3], trace.value[4]);
          return pbuf;
        case 6:
          snprintf(pbuf, sizeof(pbuf), fi.first.c_str(), trace.value[0], trace.value[1], trace.value[2], trace.value[3], trace.value[4], trace.value[5]);
          return pbuf;
        case 7:
          snprintf(pbuf, sizeof(pbuf), fi.first.c_str(), trace.value[0], trace.value[1], trace.value[2], trace.value[3], trace.value[4], trace.value[5], trace.value[6]);
          return pbuf;
        case 8:
          snprintf(pbuf, sizeof(pbuf), fi.first.c_str(), trace.value[0], trace.value[1], trace.value[2], trace.value[3], trace.value[4], trace.value[5], trace.value[6], trace.value[7]);
          return pbuf;
        case 9:
          snprintf(pbuf, sizeof(pbuf), fi.first.c_str(), trace.value[0], trace.value[1], trace.value[2], trace.value[3], trace.value[4], trace.value[5], trace.value[6], trace.value[7], trace.value[8]);
          return pbuf;
        case 10:
          snprintf(pbuf, sizeof(pbuf), fi.first.c_str(), trace.value[0], trace.value[1], trace.value[2], trace.value[3], trace.value[4], trace.value[5], trace.value[6], trace.value[7], trace.value[9]);
          return pbuf;
        case 11:
          snprintf(pbuf, sizeof(pbuf), fi.first.c_str(), trace.value[0], trace.value[1], trace.value[2], trace.value[3], trace.value[4], trace.value[5], trace.value[6], trace.value[7], trace.value[9], trace.value[10]);
          return pbuf;
        case 12:
          snprintf(pbuf, sizeof(pbuf), fi.first.c_str(), trace.value[0], trace.value[1], trace.value[2], trace.value[3], trace.value[4], trace.value[5], trace.value[6], trace.value[7], trace.value[9], trace.value[10], trace.value[11]);
          return pbuf;
        default:
          PRINT_NAMED_ERROR("Robot.TracePrinterBadArgs", "Invalid number of trace arguments %d", nargs);
          snprintf(pbuf, sizeof(pbuf), "Invalid number of trace arguments: %d", nargs);
          return pbuf;
      }
    }
  }
}


} // Namespace Cozmo
} // Namespace Anki
