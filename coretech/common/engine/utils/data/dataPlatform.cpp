/**
* File: dataPlatform.cpp
*
* Author: damjan stulic
* Created: 8/5/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/


#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "json/json.h"
#include "util/logging/logging.h"
#include "util/helpers/includeFstream.h"
#include "util/helpers/ankiDefines.h"
#include "util/fileUtils/fileUtils.h"

#define LOG_CHANNEL "DataPlatform"

namespace Anki {
namespace Util {
namespace Data {


DataPlatform::DataPlatform(const std::string &persistentPath, const std::string &cachePath, const std::string &resourcesPath)
: _persistentPath(persistentPath)
, _cachePath(cachePath)
, _resourcesPath(resourcesPath)
{

}

std::string DataPlatform::pathToResource(const Scope& resourceScope, const std::string& resourceName) const
{
  std::string s = "";
  switch (resourceScope) {
    case Scope::Persistent:
      s += _persistentPath;
      break;
    case Scope::Resources:
      s += _resourcesPath;
      break;
    case Scope::Cache:
      s += _cachePath;
      break;
    case Scope::CurrentGameLog:
      s += _cachePath;
      s += "/gameLogs";
      break;
  }
  if (!resourceName.empty()) {
    if ('/' != resourceName.front()) {
      s += "/";
    }
    s += resourceName;
  }

  //LOG_DEBUG("DataPlatform", "%s", s.c_str());
  return s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string DataPlatform::GetOSPlatformString()
{
  #if defined(ANKI_PLATFORM_IOS)
    return "ios";
  #elif defined(ANKI_PLATFORM_ANDROID)
    return "android";
  #elif defined(ANKI_PLATFORM_OSX)
    return "osx";
  #else
    return "undefined";
  #endif
}

// reads resource as json file. returns true if successful.
bool DataPlatform::readAsJson(const Scope& resourceScope, const std::string& resourceName, Json::Value& data) const
{
  const std::string& jsonFilename = pathToResource(resourceScope, resourceName);
  return readAsJson(jsonFilename, data);
}

// reads resource as json file. returns true if successful.
bool DataPlatform::readAsJson(const std::string& resourceName, Json::Value& data)
{
  std::ifstream jsonFile(resourceName);
  Json::Reader reader;
  bool success = reader.parse(jsonFile, data);
  if (!success) {
    LOG_ERROR("DataPlatform.readAsJson", "Failed to read [%s]", resourceName.c_str());
    const std::string& errors = reader.getFormattedErrorMessages();
    if (!errors.empty()) {
      LOG_DEBUG("DataPlatform.readAsJson", "Json reader errors [%s]", errors.c_str());
    }
  }
  jsonFile.close();
  return success;
}

// write dat to json file. returns true if successful.
bool DataPlatform::writeAsJson(const Scope& resourceScope, const std::string& resourceName, const Json::Value& data) const
{
  const std::string jsonFilename = pathToResource(resourceScope, resourceName);
  LOG_INFO("DataPlatform.writeAsJson", "writing to %s", jsonFilename.c_str());
  if (!Util::FileUtils::CreateDirectory(jsonFilename, true, true)) {
    LOG_ERROR("DataPlatform.writeAsJson", "Failed to create folder %s", jsonFilename.c_str());
    return false;
  }
  Json::StyledStreamWriter writer;
  std::fstream fs;
  fs.open(jsonFilename, std::ios::out);
  if (!fs.is_open()) {
    return false;
  }
  writer.write(fs, data);
  fs.close();
  return true;
}


} // end namespace Data
} // end namespace Util
} // end namespace Anki
