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

#include "anki/common/basestation/utils/data/dataScope.h"
#include "json/json-forwards.h"
#include <string>

namespace Anki {
namespace Util {
namespace Data {

class DataPlatform {
public:

  DataPlatform(const std::string &filesPath, const std::string &cachePath,
    const std::string &externalPath,
    const std::string &resourcesPath);

  std::string pathToResource(const Scope& resourceScope, const std::string& resourceName) const;

  // reads resource as json file. returns true if successful.
  bool readAsJson(const Scope& resourceScope, const std::string& resourceName, Json::Value& data) const;

  // reads resource as json file. returns true if successful.
  bool readAsJson(const std::string& resourceName, Json::Value& data) const;

  // write dat to json file. returns true if successful.
  bool writeAsJson(const Scope& resourceScope, const std::string& resourceName, const Json::Value& data) const;

private:
  const std::string _filesPath;
  const std::string _cachePath;
  const std::string _externalPath;
  const std::string _resourcesPath;
};

} // end namespace Data
} // end namespace Util
} // end namespace Anki




#endif //_Anki_Common_Basestation_Utils_Data_DataPlatform_H__
