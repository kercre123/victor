/**
* File: dataPlatform
*
* Author: damjan stulic
* Created: 8/5/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/

#ifndef _Anki_Common_Basestation_Utils_Data_DataPlatform_H__
#define _Anki_Common_Basestation_Utils_Data_DataPlatform_H__

#include "coretech/common/engine/utils/data/dataScope.h"
#include "json/json-forwards.h"
#include <string>

namespace Anki {
namespace Util {
namespace Data {

class DataPlatform {
public:

  DataPlatform(const std::string &persistentPath, const std::string &cachePath, const std::string &resourcesPath);

  std::string pathToResource(const Scope& resourceScope, const std::string& resourceName) const;
  
  // returns a key for the current platform/OS (eg: osx, android, ios)
  static std::string GetOSPlatformString();

  // reads resource as json file. returns true if successful.
  bool readAsJson(const Scope& resourceScope, const std::string& resourceName, Json::Value& data) const;

  // reads resource as json file. returns true if successful.
  static bool readAsJson(const std::string& resourceName, Json::Value& data);

  // write dat to json file. returns true if successful.
  bool writeAsJson(const Scope& resourceScope, const std::string& resourceName, const Json::Value& data) const;

private:
  const std::string _persistentPath;
  const std::string _cachePath;
  const std::string _resourcesPath;
};

} // end namespace Data
} // end namespace Util
} // end namespace Anki




#endif //_Anki_Common_Basestation_Utils_Data_DataPlatform_H__
