/**
 * File: createHttpAdapter_android
 *
 * Author: baustin
 * Date: 7/22/16
 *
 * Description: Create native interface for http connections, Android edition
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/cozmo/basestation/util/http/createHttpAdapter.h"
#include "util/http/httpAdapter_android.h"

Anki::Util::IHttpAdapter* CreateHttpAdapter()
{
  return new Anki::Util::HttpAdapter();
}
