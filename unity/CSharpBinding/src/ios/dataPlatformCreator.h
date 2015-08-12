//
//  dataPlatformCreator.h
//  cozmoGame
//
//  Created by damjan stulic on 8/8/15.
//
//

#ifndef __Ios_DataPlatformCreator_H__
#define __Ios_DataPlatformCreator_H__

namespace Anki {
namespace Cozmo {
namespace Data {
class DataPlatform;
} // end namspace Data
} // end namespace Cozmo
} // end namespace Anki

Anki::Cozmo::Data::DataPlatform* CreateDataPlatform();

#endif // end __Ios_DataPlatformCreator_H__
