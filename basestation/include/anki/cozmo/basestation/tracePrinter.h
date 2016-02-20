/** @file Robot Log Trace parsing, formatting, printing etc.
 * @author Daniel Casner <daniel@anki.com>
 */
#ifndef __BASESTATION_TRACE_PRINTER_H_
#define __BASESTATION_TRACE_PRINTER_H_

#include "anki/common/types.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include <map>
#include <string>
#include <utility>

namespace Anki {
  namespace Cozmo {  
  
    typedef std::map<const int, const std::string> IntStringMap;
    typedef std::pair<const std::string, const int> FormatInfo;
    typedef std::map<const int, const FormatInfo> IntFormatMap;
    
  
    class TracePrinter {
    public:
      /// Default constructor, loads the string tables from the default location
      // @param dp A data platform instance to use to load the tables
      TracePrinter(Util::Data::DataPlatform* dp);
      
      /// Sets the log level which will result in printing
      void SetPrintThreshold(const RobotInterface::LogLevel level);
      
      /// Handler for incoming trace messages
      void HandleTrace(const AnkiEvent<RobotInterface::RobotToEngine>& message) const;
      
      /// Handler for robot firmware crash dumps
      void HandleCrashReport(const AnkiEvent<RobotInterface::RobotToEngine>& message) const;
      
      /// Retrieve the name string from a name ID
      const std::string& GetName(const int nameId) const;
      
      /// Return the formatted string from the trace
      std::string GetFormatted(const RobotInterface::PrintTrace& trace) const;
      
    private:
      IntStringMap nameTable;
      IntFormatMap formatTable;
      static const std::string UnknownTraceName;
      static const std::string UnknownTraceFormat;
      static const std::string RobotNamePrefix;
      RobotInterface::LogLevel printThreshold;
    };
    
  }
}

#endif
