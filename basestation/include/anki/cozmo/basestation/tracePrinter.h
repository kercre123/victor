/** @file Robot Log Trace parsing, formatting, printing etc.
 * @author Daniel Casner <daniel@anki.com>
 */
#ifndef __BASESTATION_TRACE_PRINTER_H_
#define __BASESTATION_TRACE_PRINTER_H_

#include "anki/common/types.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "util/signals/signalHolder.h"
#include <map>
#include <string>
#include <utility>

namespace Anki {
  namespace Cozmo {  
    
    class Robot;
  
    typedef std::map<const int, const std::string> IntStringMap;
    typedef std::pair<const std::string, const int> FormatInfo;
    typedef std::map<const int, const FormatInfo> IntFormatMap;
    
  
    class TracePrinter : private Util::SignalHolder {
    public:
      /// Default constructor, loads the string tables from the default location
      // @param dp A data platform instance to use to load the tables
      TracePrinter(Robot* robot);
      
      /// Sets the log level which will result in printing
      void SetPrintThreshold(const RobotInterface::LogLevel level);
      
      /// Handler for incoming trace messages
      void HandleTrace(const AnkiEvent<RobotInterface::RobotToEngine>& message);
      
      /// Handler for robot firmware crash dumps
      // @TODO this should be moved to a different module
      void HandleCrashReport(const AnkiEvent<RobotInterface::RobotToEngine>& message);
      
      /// Handle wifi flash ID reports
      // @TODO this should be moved to a different module
      void HandleWiFiFlashID(const AnkiEvent<RobotInterface::RobotToEngine>& message);
      
      template<typename T>
      void HandleMessage(const T& msg);
      
      /// Retrieve the name string from a name ID
      const std::string& GetName(const int nameId) const;
      
      /// Return the formatted string from the trace
      std::string GetFormatted(const RobotInterface::PrintTrace& trace) const;
      
    private:
      IntStringMap _nameTable;
      IntFormatMap _formatTable;
      static const std::string _kUnknownTraceName;
      static const std::string _kUnknownTraceFormat;
      static const std::string _kRobotNamePrefix;
      RobotInterface::LogLevel _printThreshold;
      int _lastLogRequested;
      Robot* _robot;
    };
    
  }
}

#endif
