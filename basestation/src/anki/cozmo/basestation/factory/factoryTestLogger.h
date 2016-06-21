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

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"
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
  FactoryTestLogger();
  ~FactoryTestLogger();
  
  // Specify the name of the log (i.e. log folder)
  // Optionally, specify if you want to append the current date and time to the log name
  bool StartLog(const std::string& logName, bool appendDateTime = false, Util::Data::DataPlatform* dataPlatform = nullptr);
  void CloseLog();
  
  std::string GetLogName() { return _logDir; }
  
  // Appends struct as formatted entry to log file
  bool Append(const FactoryTestResultEntry& data);
  bool Append(const CameraCalibration& data);
  bool Append(const ToolCodeInfo& data);
  bool Append(const BirthCertificate& data);
  
  // Prints byte as a bit string
  bool AppendCalibMetaInfo(uint8_t dotsFoundMask);
  
  // Expects poseData[0-2] to represent rotation about x,y,z
  // and poseData[3-5] to represent translation in x,y,z
  bool AppendPoseData(const std::string& poseName, const std::array<float,6>& poseData);
  
  // Adds a file with the given contents to the log folder
  bool AddFile(const std::string& filename, const std::vector<uint8_t>& data);
  
private:
  bool AppendToFile(const std::string& data);
  
  std::string _logDir;
  std::string _logFileName;

  std::ofstream _logFileHandle;
};

} // end namespace Cozmo
} // end namespace Anki


#endif //__Basestation_Factory_FactoryTestLogger_H_
