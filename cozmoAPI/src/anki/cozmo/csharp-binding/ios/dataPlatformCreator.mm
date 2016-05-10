//
//  dataPlatformCreator.m
//  cozmoGame
//
//  Created by damjan stulic on 8/8/15.
//
//

#import <Foundation/Foundation.h>
#import "dataPlatformCreator.h"
#include <string>
#include "anki/common/basestation/utils/data/dataPlatform.h"

Anki::Util::Data::DataPlatform* CreateDataPlatform()
{
  NSString* nsCachePath = [NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES) lastObject];
  NSString* nsDocPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
  NSString* nsResourcePath = [[NSBundle mainBundle] pathForResource:@"cozmo_resources" ofType:@""];
  std::string resourcePath = [nsResourcePath UTF8String];
  std::string filesPath = [nsDocPath UTF8String];
  std::string cachePath = [nsCachePath UTF8String];
  std::string externalPath = [nsCachePath UTF8String];
  return new Anki::Util::Data::DataPlatform(filesPath, cachePath, externalPath, resourcePath);
}
