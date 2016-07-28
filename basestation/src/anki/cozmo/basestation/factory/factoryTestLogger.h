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
#ifndef __Basestation_Factory_FactoryTestLogger_H_
#define __Basestation_Factory_FactoryTestLogger_H_

#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/pose.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "json/json.h"
#include <string>
#include <vector>
#include <fstream>


namespace Anki {

// Forward declaration
namespace Util {
  namespace Data {
    class DataPlatform;
  }
}
  
namespace Cozmo {

class FactoryTestLogger {
public:
  FactoryTestLogger(bool exportJson = true);
  ~FactoryTestLogger();
  
  // Specify the name of the log (i.e. log folder)
  // Optionally, specify if you want to append the current date and time to the log name
  bool StartLog(const std::string& logName, bool appendDateTime = false, Util::Data::DataPlatform* dataPlatform = nullptr);
  void CloseLog();
  
  bool IsOpen();
  
  std::string GetLogName() { return _logDir; }
  
  // Appends struct as formatted entry to log file
  bool Append(const FactoryTestResultEntry& data);
  bool Append(const CameraCalibration& data);
  bool Append(const ToolCodeInfo& data);
  bool Append(const BirthCertificate& data);
  bool Append(const IMUInfo& data);
  bool Append(const CalibMetaInfo& data);
  bool AppendCliffValueOnDrop(const CliffSensorValue& data);
  bool AppendCliffValueOnGround(const CliffSensorValue& data);
  bool AppendCalibPose(const PoseData& data);
  bool AppendObservedCubePose(const PoseData& data);
  bool Append(const ExternalInterface::RobotCompletedFactoryDotTest& msg);
  
  // Adds a file with the given contents to the log folder
  bool AddFile(const std::string& filename, const std::vector<uint8_t>& data);

  
private:
  
  bool AppendCliffSensorValue(const std::string& readingName, const CliffSensorValue& data);
  bool AppendPoseData(const std::string& poseName, const PoseData& data);
  bool AppendToFile(const std::string& data);
  
  std::string _logDir;
  std::string _logFileName;
  std::ofstream _logFileHandle;
  
  Json::Value _json;
  bool _exportJson;
};

} // end namespace Cozmo
} // end namespace Anki


#endif //__Basestation_Factory_FactoryTestLogger_H_
