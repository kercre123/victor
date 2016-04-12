//
//  dataPlatformCreator.h
//  cozmoGame
//
//  Created by damjan stulic on 8/8/15.
//
//

#ifndef __Ios_DataPlatformCreator_H__
#define __Ios_DataPlatformCreator_H__

// Forward declaration:
namespace Anki {
namespace Util {
namespace Data {
  class DataPlatform;
} // end namspace Data
} // end namespace Cozmo
} // end namespace Anki

Anki::Util::Data::DataPlatform* CreateDataPlatform();

#endif // end __Ios_DataPlatformCreator_H__
