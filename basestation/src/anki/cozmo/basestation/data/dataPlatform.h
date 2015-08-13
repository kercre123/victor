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

#ifndef _Anki_Cozmo_Basestation_Data_DataPlatform_H__
#define _Anki_Cozmo_Basestation_Data_DataPlatform_H__

#include "dataScope.h"
#include <string>

namespace Anki {
namespace Cozmo {
namespace Data {


class DataPlatform {
public:

  DataPlatform(const std::string &filesPath, const std::string &cachePath,
    const std::string &externalPath,
    const std::string &resourcesPath);

  std::string pathToResource(Scope resourceScope, const std::string& resourceName) const;

  // returns true if the given resource exists, false otherwise
  bool exists(const std::string& resource) const;

private:
  const std::string _filesPath;
  const std::string _cachePath;
  const std::string _externalPath;
  const std::string _resourcesPath;
};

} // end namespace Data
} // end namespace Cozmo
} // end namespace Anki




#endif //_Anki_Cozmo_Basestation_Data_DataPlatform_H__
