/**
 * File: factoryTestLogger
 *
 * Author: Kevin Yoon
 * Created: 6/14/2016
 *
 * Description: Exports structs to factory test (i.e. Playpen test) formatted log file
 *
 * Copyright: Anki, inc. 2016
 *
 */
#include "anki/cozmo/basestation/factory/factoryTestLogger.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "util/logging/logging.h"
#include "util/fileUtils/fileUtils.h"

#include <sstream>
#include <chrono>

namespace Anki {
namespace Cozmo {

  static const char* _kLogTextFileName = "mfgData.txt";
  static const char* _kLogRootDirName = "factory_test_logs";
  
  FactoryTestLogger::FactoryTestLogger()
  : _logDir("")
  , _logFileName("")
  {
    
  }
  
  FactoryTestLogger::~FactoryTestLogger()
  {
    CloseLog();
  }
  

  bool FactoryTestLogger::StartLog(const std::string& logName, bool appendDateTime, Util::Data::DataPlatform* dataPlatform)
  {
    // Generate new log dir name
    std::string newLogDir = "";
    if (dataPlatform) {
      newLogDir = Util::FileUtils::FullFilePath({dataPlatform->pathToResource(Util::Data::Scope::Cache, _kLogRootDirName), logName});
    } else {
      newLogDir = Util::FileUtils::FullFilePath({_kLogRootDirName, logName});
    }
    
    // Check if it already exists
    if (Util::FileUtils::DirectoryExists(newLogDir)) {
      if (_logDir == newLogDir) {
        PRINT_NAMED_WARNING("FactoryTestLogger.StartLog.DirIsCurrentLog",
                            "Aborting current log %s because why are you trying to start it again?", newLogDir.c_str());
        CloseLog();
      } else {
        PRINT_NAMED_WARNING("FactoryTestLogger.StartLog.DirExists",
                            "Ignoring log %s because it already exists", newLogDir.c_str());
      }
      return false;
    } else {
      CloseLog();
    }

    _logDir = newLogDir;
    PRINT_NAMED_INFO("FactoryTestLogger.StartLog.CreatingLogDir", "%s", _logDir.c_str());
    
    if (appendDateTime) {
      auto time_point = std::chrono::system_clock::now();
      time_t nowTime = std::chrono::system_clock::to_time_t(time_point);
      auto nowLocalTime = localtime(&nowTime);
      char buf[80];
      strftime(buf, sizeof(buf), "_-_%F_%H-%M-%S/", nowLocalTime);
      _logDir += buf;
    }
    
    Util::FileUtils::CreateDirectory(_logDir);
    _logFileName = Util::FileUtils::FullFilePath({_logDir, _kLogTextFileName});
    
    if (_logFileHandle.is_open()) {
      PRINT_NAMED_WARNING("FactoryTestLogger.FileUnexpectedlyOpen", "");
      _logFileHandle.close();
    }
    
    PRINT_NAMED_INFO("FactoryTestLogger.StartLog.CreatingLogFile", "%s", _logFileName.c_str());
    _logFileHandle.open(_logFileName, std::ofstream::out | std::ofstream::app);
    
    return true;
  }
  
  
  void FactoryTestLogger::CloseLog()
  {
    if (_logFileHandle.is_open()) {
      _logFileHandle.close();
    }
    
    _logDir = "";
    _logFileName = "";
  }
  
  bool FactoryTestLogger::Append(const FactoryTestResultEntry& data)
  {
    std::stringstream ss;
    ss << "\n[PlayPenTest]"
       << "\nResult: "    << EnumToString(data.result)
       << "\nTime: "      << data.utcTime
       << "\nSHA-1: 0x"   << std::hex << data.engineSHA1 << std::dec
       << "\nStationID: " << data.stationID
       << "\nTimestamps: ";
    
    for(auto timestamp : data.timestamps) {
      ss << timestamp << " ";
    }
    ss << std::endl;

    PRINT_NAMED_INFO("FactoryTestLogger.Append.FactoryTestResultEntry", "%s", ss.str().c_str());
    return AppendToFile(ss.str());
  }
  
  
  bool FactoryTestLogger::Append(const CameraCalibration& data)
  {
    std::stringstream ss;
    ss << "\n[CameraCalibration]"
       << "\nfx: " << data.focalLength_x
       << "\nfy: " << data.focalLength_y
       << "\ncx: " << data.center_x
       << "\ncy: " << data.center_y
       << "\nskew: " << data.skew
       << "\nrows: " << data.nrows
       << "\ncols: " << data.ncols
       << "\ndistortionCoeffs: ";
    
    for(auto coeff : data.distCoeffs) {
      ss << coeff << " ";
    }
    ss << std::endl;
    
    PRINT_NAMED_INFO("FactoryTestLogger.Append.CameraCalibration", "%s", ss.str().c_str());
    return AppendToFile(ss.str());
  }
  
  bool FactoryTestLogger::Append(const ToolCodeInfo& data)
  {
    std::stringstream ss;
    ss << "\n[ToolCode]"
       << "\nCode: " << EnumToString(data.code)
       << "\nExpected_L: " << data.expectedCalibDotLeft_x  << ", " << data.expectedCalibDotLeft_y
       << "\nExpected_R: " << data.expectedCalibDotRight_x << ", " << data.expectedCalibDotRight_y
       << "\nObserved_L: " << data.observedCalibDotLeft_x  << ", " << data.observedCalibDotLeft_y
       << "\nObserved_R: " << data.observedCalibDotRight_x << ", " << data.observedCalibDotRight_y << std::endl;
    
    PRINT_NAMED_INFO("FactoryTestLogger.Append.ToolCodeInfo", "%s", ss.str().c_str());
    return AppendToFile(ss.str());
  }

  bool FactoryTestLogger::Append(const BirthCertificate& data)
  {
    std::stringstream ss;
    ss << "\n[BirthCertificate]"
       << "\nAtFactory: " << static_cast<int>(data.atFactory)
       << "\nFactory: "   << static_cast<int>(data.whichFactory)
       << "\nLine: "      << static_cast<int>(data.whichLine)
       << "\nModel: "     << static_cast<int>(data.model)
       << "\nYear: "      << static_cast<int>(data.year)
       << "\nMonth: "     << static_cast<int>(data.month)
       << "\nDay: "       << static_cast<int>(data.day)
       << "\nHour: "      << static_cast<int>(data.hour)
       << "\nMinute: "    << static_cast<int>(data.minute)
       << "\nSecond: "    << static_cast<int>(data.second) << std::endl;
    
    PRINT_NAMED_INFO("FactoryTestLogger.Append.BirthCertificate", "%s", ss.str().c_str());
    return AppendToFile(ss.str());
  }

  bool FactoryTestLogger::AppendPoseData(const std::string& poseName, const std::array<float,6>& poseData)
  {
    std::stringstream ss;
    ss << "\n[" << poseName << "]"
       << "\nRot: "   << poseData[0] << " " << poseData[1] << " " << poseData[2]
       << "\nTrans: " << poseData[3] << " " << poseData[4] << " " << poseData[5] << std::endl;
    
    PRINT_NAMED_INFO("FactoryTestLogger.Append.PoseData", "%s", ss.str().c_str());
    return AppendToFile(ss.str());
  }
  
  bool FactoryTestLogger::AppendToFile(const std::string& data) {
    
    // If log name was not actually defined yet, do nothing.
    if (!_logFileHandle.is_open()) {
      PRINT_NAMED_INFO("FactoryTestLogger.Append.LogNotStarted", "Ignoring because log not started");
      return false;
    }
    _logFileHandle << data;
    return true;
  }

  bool FactoryTestLogger::AddFile(const std::string& filename, const std::vector<uint8_t>& data)
  {
    if (_logDir.empty()) {
      PRINT_NAMED_INFO("FactoryTestLogger.AddFile.LogNotStarted", "Ignoring because log not started");
      return false;
    }
    
    if (filename.empty()) {
      PRINT_NAMED_WARNING("FactoryTestLogger.AddFile.EmptyFilename", "");
      return false;
    }
    
    std::string outFile = Util::FileUtils::FullFilePath({_logDir, filename});
    
    if (Util::FileUtils::FileExists(outFile)) {
      PRINT_NAMED_WARNING("FactoryTestLogger.AddFile.AlreadyExists",
                          "Ignoring because %s already exists", outFile.c_str() );
      return false;
    }
    
    PRINT_NAMED_INFO("FactoryTestLogger.AddFile",
                     "File: %s, size: %zu bytes",
                     outFile.c_str(), data.size());
    
    return Util::FileUtils::WriteFile(outFile, data);
  }
  
  
} // end namespace Cozmo
} // end namespace Anki
