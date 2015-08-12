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


#include "dataPlatform.h"
#include "json/json.h"
#include "util/logging/logging.h"
#include "util/helpers/includeFstream.h"
#include "util/fileUtils/fileUtils.h"

namespace Anki {
namespace Cozmo {
namespace Data {


DataPlatform::DataPlatform(const std::string &filesPath, const std::string &cachePath,
  const std::string &externalPath, const std::string &resourcesPath)
: _filesPath(filesPath)
, _cachePath(cachePath)
, _externalPath(externalPath)
, _resourcesPath(resourcesPath)
{

}

std::string DataPlatform::pathToResource(const Scope& resourceScope, const std::string& resourceName) const
{
  std::string s = "";
  if (Scope::Resources == resourceScope) {
    if (resourceName.empty()) {
      PRINT_NAMED_WARNING("Platform.pathToResource", "Request for top level resource directory");
    }
  }

  if (s.empty()) {
    switch (resourceScope) {
      case Scope::Output:
        s += _filesPath;
        s += "/output";
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
      case Scope::External:
        s += _externalPath;
        break;
    }
    if (!resourceName.empty()) {
      if ('/' != resourceName.front()) {
        s += "/";
      }
      s += resourceName;
    }
  }
  //PRINT_NAMED_DEBUG("DataPlatform", "%s", s.c_str());
  return s;
}

// reads resource as json file. returns true if successful.
bool DataPlatform::readAsJson(const Scope& resourceScope, const std::string& resourceName, Json::Value& data) const
{
  const std::string jsonFilename = pathToResource(resourceScope, resourceName);
  std::ifstream jsonFile(jsonFilename);
  Json::Reader reader;
  bool success = reader.parse(jsonFile, data);
  if(! success) {
    PRINT_NAMED_ERROR("DataPlatform.readAsJson", "Failed to parse Json file %s.", resourceName.c_str());
  }
  jsonFile.close();
  return success;
}

// write dat to json file. returns true if successful.
bool DataPlatform::writeAsJson(const Scope& resourceScope, const std::string& resourceName, const Json::Value& data) const
{
  const std::string jsonFilename = pathToResource(resourceScope, resourceName);
  PRINT_NAMED_INFO("DataPlatform.writeAsJson", "writing to %s", jsonFilename.c_str());
  if (!Util::FileUtils::CreateDirectory(jsonFilename, true, true)) {
    PRINT_NAMED_ERROR("DataPlatform.writeAsJson", "Failed to create folder %s.", jsonFilename.c_str());
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
} // end namespace Cozmo
} // end namespace Anki
