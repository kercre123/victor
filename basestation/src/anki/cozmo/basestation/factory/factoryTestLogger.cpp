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
#include "anki/cozmo/basestation/util/file/archiveUtil.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "util/logging/logging.h"
#include "util/fileUtils/fileUtils.h"

#include <sstream>
#include <chrono>

namespace Anki {
namespace Cozmo {

  static const std::string _kLogTextFileName = "mfgData";
  static const std::string _kLogRootDirName = "factory_test_logs";
  static const std::string _kArchiveRootDirName = "factory_test_log_archives";
  static const Util::Data::Scope _kLogScope = Util::Data::Scope::Output;
  
  FactoryTestLogger::FactoryTestLogger(bool exportJson)
  : _logDir("")
  , _logFileName("")
  , _exportJson(exportJson)
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
      newLogDir = Util::FileUtils::FullFilePath({dataPlatform->pathToResource(_kLogScope, _kLogRootDirName), logName});
    } else {
      newLogDir = Util::FileUtils::FullFilePath({_kLogRootDirName, logName});
    }

    // Append date time to log name
    if (appendDateTime) {
      newLogDir += "_-_" + GetCurrDateTime() + "/";
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
    Util::FileUtils::CreateDirectory(_logDir);
    _logFileName = Util::FileUtils::FullFilePath({_logDir, _kLogTextFileName + (_exportJson ? ".json" : ".txt")});
    
    if (_logFileHandle.is_open()) {
      PRINT_NAMED_WARNING("FactoryTestLogger.FileUnexpectedlyOpen", "");
      _logFileHandle.close();
    }
    
    PRINT_NAMED_INFO("FactoryTestLogger.StartLog.CreatingLogFile", "%s", _logFileName.c_str());
    _logFileHandle.open(_logFileName, std::ofstream::out | std::ofstream::app);
    
    _json.clear();
    
    return true;
  }
  
  
  void FactoryTestLogger::CloseLog()
  {
    if (_logFileHandle.is_open()) {
      PRINT_NAMED_INFO("FactoryTestLogger.CloseLog.Closing","");
      
      // If exporting json, write it to file here
      if (_exportJson) {
        _logFileHandle << _json;
      }
      _logFileHandle.close();
    }
    
    _logDir = "";
    _logFileName = "";
  }
  
  bool FactoryTestLogger::IsOpen() {
    return _logFileHandle.is_open();
  }
  
  bool FactoryTestLogger::Append(const FactoryTestResultEntry& data)
  {
    std::stringstream ss;
    if (_exportJson) {
      Json::Value& node = _json["PlayPenTest"];
      node["Result"]    = EnumToString(data.result);
      node["Time"]      = data.utcTime;
      std::stringstream shaStr;
      shaStr << std::hex << data.engineSHA1 << std::dec;
      node["SHA-1"]     = shaStr.str();
      node["StationID"] = data.stationID;
      for (auto i=0; i<data.timestamps.size(); ++i) {
        node["Timestamps"][i] = data.timestamps[i];
      }
      ss << "[PlayPenTest]\n" << node;
    } else {
      ss << "\n[PlayPenTest]"
         << "\nResult: "    << EnumToString(data.result)
         << "\nTime: "      << data.utcTime
         << "\nSHA-1: 0x"   << std::hex << data.engineSHA1 << std::dec
         << "\nStationID: " << data.stationID
         << "\nTimestamps: ";
      for(auto timestamp : data.timestamps) {
        ss << timestamp << " ";
      }
    }

    PRINT_NAMED_INFO("FactoryTestLogger.Append.FactoryTestResultEntry", "%s", ss.str().c_str());
    return AppendToFile(ss.str());
  }
  
  
  bool FactoryTestLogger::Append(const CameraCalibration& data)
  {
    std::stringstream ss;
    if (_exportJson) {
      Json::Value& node = _json["CameraCalibration"];
      node["fx"] = data.focalLength_x;
      node["fy"] = data.focalLength_y;
      node["cx"] = data.center_x;
      node["cy"] = data.center_y;
      node["skew"] = data.skew;
      node["nrows"] = data.nrows;
      node["ncols"] = data.ncols;
      for (auto i=0; i < data.distCoeffs.size(); ++i) {
        node["distortionCoeffs"][i] = data.distCoeffs[i];
      }
      ss << "[CameraCalibration]\n" << node;
    } else {
      ss << "\n[CameraCalibration]" << std::fixed
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
    }
    PRINT_NAMED_INFO("FactoryTestLogger.Append.CameraCalibration", "%s", ss.str().c_str());
    return AppendToFile(ss.str());
  }
  
  bool FactoryTestLogger::Append(const ToolCodeInfo& data)
  {
    std::stringstream ss;
    if (_exportJson) {
      Json::Value& node = _json["ToolCode"];
      node["Code"] = EnumToString(data.code);
      node["Expected_L"][0] = data.expectedCalibDotLeft_x;
      node["Expected_L"][1] = data.expectedCalibDotLeft_y;
      node["Expected_R"][0] = data.expectedCalibDotRight_x;
      node["Expected_R"][1] = data.expectedCalibDotRight_y;
      node["Observed_L"][0] = data.observedCalibDotLeft_x;
      node["Observed_L"][1] = data.observedCalibDotLeft_y;
      node["Observed_R"][0] = data.observedCalibDotRight_x;
      node["Observed_R"][1] = data.observedCalibDotRight_y;
      ss << "[ToolCode]\n" << node;
    } else {
      ss << "\n[ToolCode]"
      << "\nCode: " << EnumToString(data.code)
      << "\nExpected_L: " << data.expectedCalibDotLeft_x  << ", " << data.expectedCalibDotLeft_y
      << "\nExpected_R: " << data.expectedCalibDotRight_x << ", " << data.expectedCalibDotRight_y
      << "\nObserved_L: " << data.observedCalibDotLeft_x  << ", " << data.observedCalibDotLeft_y
      << "\nObserved_R: " << data.observedCalibDotRight_x << ", " << data.observedCalibDotRight_y;
    }
    
    PRINT_NAMED_INFO("FactoryTestLogger.Append.ToolCodeInfo", "%s", ss.str().c_str());
    return AppendToFile(ss.str());
  }

  bool FactoryTestLogger::Append(const BirthCertificate& data)
  {
    std::stringstream ss;
    if (_exportJson) {
      Json::Value& node = _json["BirthCertificate"];
      node["AtFactory"] = static_cast<int>(data.atFactory);
      node["Factory"]   = static_cast<int>(data.whichFactory);
      node["Line"]      = static_cast<int>(data.whichLine);
      node["Model"]     = static_cast<int>(data.model);
      node["Year"]      = static_cast<int>(data.year);
      node["Month"]     = static_cast<int>(data.month);
      node["Day"]       = static_cast<int>(data.day);
      node["Hour"]      = static_cast<int>(data.hour);
      node["Minute"]    = static_cast<int>(data.minute);
      node["Second"]    = static_cast<int>(data.second);
      ss << "[BirthCertificate]\n" << node;
    } else {
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
         << "\nSecond: "    << static_cast<int>(data.second);
    }
    PRINT_NAMED_INFO("FactoryTestLogger.Append.BirthCertificate", "%s", ss.str().c_str());
    return AppendToFile(ss.str());
  }

  bool FactoryTestLogger::Append(const CalibMetaInfo& data)
  {
    std::bitset<8> b(data.dotsFoundMask);
    
    std::stringstream ss;
    if (_exportJson) {
      Json::Value& node = _json["CalibMetaInfo"];
      node["ImagesUsed"] = b.to_string();
      ss << "[CalibMetaInfo]\n" << node;
    } else {
      ss << "\n[CalibMetaInfo]"
      << "\nImagesUsed: " << b.to_string();
    }
    PRINT_NAMED_INFO("FactoryTestLogger.Append.CalibMetaInfo", "%s", ss.str().c_str());
    return AppendToFile(ss.str());
  }
  
  bool FactoryTestLogger::Append(const IMUInfo& data)
  {
    std::stringstream ss;
    if (_exportJson) {
      Json::Value& node = _json["IMUInfo"];
      node["DriftRate_degPerSec"] = data.driftRate_degPerSec;
      ss << "[IMUInfo]\n" << node;
    } else {
      ss << "\n[IMUInfo]" << std::fixed
      << "\nDriftRate_degPerSec: " << data.driftRate_degPerSec;
    }
    PRINT_NAMED_INFO("FactoryTestLogger.Append.IMUInfo", "%s", ss.str().c_str());
    return AppendToFile(ss.str());
  }
  
  
  bool FactoryTestLogger::AppendCliffValueOnDrop(const CliffSensorValue& data) {
    return AppendCliffSensorValue("CliffOnDrop", data);
  }
  
  bool FactoryTestLogger::AppendCliffValueOnGround(const CliffSensorValue& data) {
    return AppendCliffSensorValue("CliffOnGround", data);
  }
  
  bool FactoryTestLogger::AppendCliffSensorValue(const std::string& readingName, const CliffSensorValue& data)
  {
    std::stringstream ss;
    if (_exportJson) {
      Json::Value& node = _json[readingName];
      node["val"] = data.val;
      ss << "[" << readingName<< "]\n" << node;
    } else {
      ss << "\n[" << readingName << "]"
      << "\nval: " << data.val;
    }
    PRINT_NAMED_INFO("FactoryTestLogger.Append.CliffSensorValue", "%s", ss.str().c_str());
    return AppendToFile(ss.str());
  }
      
  bool FactoryTestLogger::AppendCalibPose(const PoseData& data) {
    return AppendPoseData("CalibPose", data);
  }

  bool FactoryTestLogger::AppendObservedCubePose(const PoseData& data) {
    return AppendPoseData("ObservedCubePose", data);
  }
    
  bool FactoryTestLogger::AppendPoseData(const std::string& poseName, const PoseData& data)
  {
    std::stringstream ss;
    if (_exportJson) {
      Json::Value& node = _json[poseName];
      node["Rot_deg"][0] = RAD_TO_DEG_F32(data.angleX_rad);
      node["Rot_deg"][1] = RAD_TO_DEG_F32(data.angleY_rad);
      node["Rot_deg"][2] = RAD_TO_DEG_F32(data.angleZ_rad);
      node["Trans_mm"][0] = data.transX_mm;
      node["Trans_mm"][1] = data.transY_mm;
      node["Trans_mm"][2] = data.transZ_mm;
      ss << "[" << poseName << "]\n" << node;
    } else {
      ss << "\n[" << poseName << "]" << std::fixed
         << "\nRot_deg: "  << RAD_TO_DEG_F32(data.angleX_rad) << " " << RAD_TO_DEG_F32(data.angleY_rad) << " " << RAD_TO_DEG_F32(data.angleZ_rad)
         << "\nTrans_mm: " << data.transX_mm << " " << data.transY_mm << " " << data.transZ_mm;
    }
    PRINT_NAMED_INFO("FactoryTestLogger.Append.PoseData", "%s", ss.str().c_str());
    return AppendToFile(ss.str());
  }
  
  bool FactoryTestLogger::Append(const ExternalInterface::RobotCompletedFactoryDotTest& msg)
  {
    std::stringstream ss;
    if (_exportJson) {
      if(msg.success)
      {
        Json::Value& node = _json["CentroidInfo"];
        node["HeadAngle_deg"] = RAD_TO_DEG(msg.headAngle);
        node["UpperLeft"][0] = msg.dotCenX_pix[0];
        node["UpperLeft"][1] = msg.dotCenY_pix[0];
        node["LowerLeft"][0] = msg.dotCenX_pix[1];
        node["LowerLeft"][1] = msg.dotCenY_pix[1];
        node["UpperRight"][0] = msg.dotCenX_pix[2];
        node["UpperRight"][1] = msg.dotCenY_pix[2];
        node["LowerRight"][0] = msg.dotCenX_pix[3];
        node["LowerRight"][1] = msg.dotCenY_pix[3];
        ss << "[CentroidInfo]\n" << node;
        
        if(msg.didComputePose)
        {
          PoseData pd(msg.camPoseRoll_rad, msg.camPosePitch_rad, msg.camPoseYaw_rad,
                      msg.camPoseX_mm, msg.camPoseY_mm, msg.camPoseZ_mm);
          AppendPoseData("CamPose", pd);
        }
      }
    } else {
      if(msg.success)
      {
        ss << "\n[CentroidInfo]" << std::fixed
        << "\nHeadAngle_deg: " << RAD_TO_DEG(msg.headAngle)
        << "\nUpperLeft: "  << msg.dotCenX_pix[0] << " " << msg.dotCenY_pix[0]
        << "\nLowerLeft: "  << msg.dotCenX_pix[1] << " " << msg.dotCenY_pix[1]
        << "\nUpperRight: " << msg.dotCenX_pix[2] << " " << msg.dotCenY_pix[2]
        << "\nLowerRight: " << msg.dotCenX_pix[3] << " " << msg.dotCenY_pix[3];
        
        if(msg.didComputePose)
        {
          PoseData pd(msg.camPoseRoll_rad, msg.camPosePitch_rad, msg.camPoseYaw_rad,
                      msg.camPoseX_mm, msg.camPoseY_mm, msg.camPoseZ_mm);
          AppendPoseData("CamPose", pd);
        }
      }
    }
    
    PRINT_NAMED_INFO("FactoryTestLogger.Append.CentroidInfo", "%s", ss.str().c_str());
    return AppendToFile(ss.str());
  }
  
  bool FactoryTestLogger::AppendToFile(const std::string& data) {
    
    // If log name was not actually defined yet, do nothing.
    if (!_logFileHandle.is_open()) {
      PRINT_NAMED_INFO("FactoryTestLogger.Append.LogNotStarted", "Ignoring because log not started");
      _json.clear();
      return false;
    }
    if (!_exportJson) {
      _logFileHandle << data << std::endl;
    }
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
  
  
  uint32_t FactoryTestLogger::GetNumLogs(Util::Data::DataPlatform* dataPlatform)
  {
    // Get base directory of log directories
    std::string baseDirectory = _kLogRootDirName;
    if (dataPlatform) {
      baseDirectory = dataPlatform->pathToResource(_kLogScope, _kLogRootDirName);
    }
    
    // Get all log directories
    std::vector<std::string> directoryList;
    Util::FileUtils::ListAllDirectories(baseDirectory, directoryList);
    
    return static_cast<uint32_t>(directoryList.size());
  }

  uint32_t FactoryTestLogger::GetNumArchives(Util::Data::DataPlatform* dataPlatform)
  {
    // Get base directory of log directories
    std::string baseDirectory = _kArchiveRootDirName;
    if (dataPlatform) {
      baseDirectory = dataPlatform->pathToResource(_kLogScope, _kArchiveRootDirName);
    }
    
    // Get all log directories
    std::vector<std::string> directoryList;
    Util::FileUtils::ListAllDirectories(baseDirectory, directoryList);
    
    return static_cast<uint32_t>(directoryList.size());
  }
  
  bool FactoryTestLogger::ArchiveLogs(Util::Data::DataPlatform* dataPlatform)
  {
    // Get base directory of log directories
    std::string logBaseDirectory = _kLogRootDirName;
    std::string archiveBaseDirectory = _kArchiveRootDirName;
    if (dataPlatform) {
      logBaseDirectory = dataPlatform->pathToResource(_kLogScope, _kLogRootDirName);
      archiveBaseDirectory = dataPlatform->pathToResource(_kLogScope, _kArchiveRootDirName);
    }
    
    // Make sure output directory exists
    Util::FileUtils::CreateDirectory(archiveBaseDirectory, false, true);
    
    // Generate name of new archive based on current date-time
    std::string archiveName = GetCurrDateTime() + ".tar.gz";
    
    // Create archive
    auto filePaths = Util::FileUtils::FilesInDirectory(logBaseDirectory, true, nullptr, true);
    if (ArchiveUtil::CreateArchiveFromFiles(archiveBaseDirectory + "/" + archiveName, logBaseDirectory, filePaths)) {
    
      // Delete original logs
      Util::FileUtils::RemoveDirectory(logBaseDirectory);
      
      return true;
    }
    
    return false;
  }
  
  std::string FactoryTestLogger::GetCurrDateTime() const
  {
    auto time_point = std::chrono::system_clock::now();
    time_t nowTime = std::chrono::system_clock::to_time_t(time_point);
    auto nowLocalTime = localtime(&nowTime);
    char buf[50];
    strftime(buf, sizeof(buf), "%F_%H-%M-%S", nowLocalTime);
    return std::string(buf);
  }
  
} // end namespace Cozmo
} // end namespace Anki
