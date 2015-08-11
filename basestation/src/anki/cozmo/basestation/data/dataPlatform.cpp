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
#include "util/logging/logging.h"
#include <unistd.h>

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

std::string DataPlatform::pathToResource(Scope resourceScope, const std::string& resourceName) const
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
  PRINT_NAMED_DEBUG("DataPlatform", "%s", s.c_str());
  return s;
}

// returns true if the given resource exists, false otherwise
bool DataPlatform::exists(const std::string& resource) const
{
  return access(resource.c_str(), R_OK) == 0;
}


} // end namespace Data
} // end namespace Cozmo
} // end namespace Anki
