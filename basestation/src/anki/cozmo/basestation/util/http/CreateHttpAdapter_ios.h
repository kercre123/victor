//
//  httpAdapterCreator.h
//  cozmoGame
//
//  Created by damjan stulic on 8/8/15.
//
//

#ifndef __Ios_httpAdapterCreator_H__
#define __Ios_httpAdapterCreator_H__

// Forward declaration:
namespace Anki {
namespace Util {
  class IHttpAdapter;
} // end namespace Cozmo
} // end namespace Anki

Anki::Util::IHttpAdapter* CreateHttpAdapter();

#endif // end __Ios_httpAdapterCreator_H__
