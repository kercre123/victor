/**
 * File: httpAdapterCreator.h
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Create native interface for http connections.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/


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
