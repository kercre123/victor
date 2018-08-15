/**
 * File: dasConfiguration.h
 *
 * Author: Lee Crippen
 * Created: 2/8/16
 *
 * Description: Configures DAS to start it running
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Ios_DasConfiguration_H__
#define __Ios_DasConfiguration_H__

#include <string>

// Forward declaration:
namespace Anki {
namespace Util {
namespace Data {
  class DataPlatform;
} // end namspace Data
} // end namespace Vector
} // end namespace Anki

void ConfigureDASForPlatform(Anki::Util::Data::DataPlatform* platform, const std::string& apprun);

#endif // __Ios_DasConfiguration_H__
