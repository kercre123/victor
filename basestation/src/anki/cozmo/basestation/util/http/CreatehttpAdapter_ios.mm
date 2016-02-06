/**
 * File: httpAdapterCreator.cpp
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Create native interface for http connections, the iOS/mac version
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#import "CreateHttpAdapter.h"
#include "httpAdapter_mac_ios.h"

Anki::Util::IHttpAdapter* CreateHttpAdapter()
{
  return new Anki::Util::HttpAdapter();
}
