//
//  dataPlatformCreator.m
//  cozmoGame
//
//  Created by damjan stulic on 8/8/15.
//
//

#import "CreateHttpAdapter_ios.h"
#include "httpAdapter_mac_ios.h"

Anki::Util::IHttpAdapter* CreateHttpAdapter()
{
  return new Anki::Util::HttpAdapter();
}
