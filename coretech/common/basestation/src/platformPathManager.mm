#import <Foundation/Foundation.h>

#import "anki/common/basestation/platformPathManager_iOS_interface.h"

@implementation PlatformPathManager_iOS

+ (NSString*)appBundleMetaPath
{
  static NSString* metaPath = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    metaPath = [[NSBundle mainBundle] pathForResource:@"cozmo_resources/meta" ofType:@""];
  });

  return metaPath;
}

+ (NSString*)appBundleAssetPath
{
  static NSString* assetPath = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    assetPath = [[NSBundle mainBundle] pathForResource:@"cozmo_resources/asset" ofType:@""];
  });

  return assetPath;
}

+ (NSString*)getPathWithScope:(PlatformPathManager_iOS_Scope)scope
{
  NSString *path = nil;

  switch (scope) {
    case PlatformPathManager_iOS_Scope_Test:
      path = [[PlatformPathManager_iOS appBundleMetaPath] stringByAppendingString:@"/"];
      break;

    case PlatformPathManager_iOS_Scope_Config:
      path = [[PlatformPathManager_iOS appBundleMetaPath] stringByAppendingString:@"/basestation/config/"];
      break;

    case PlatformPathManager_iOS_Scope_Animation:
      path = [[PlatformPathManager_iOS appBundleMetaPath] stringByAppendingString:@"/basestation/animations/"];
      break;

    case PlatformPathManager_iOS_Scope_Sound:
      path = [[PlatformPathManager_iOS appBundleAssetPath] stringByAppendingString:@"/sounds/"];
      break;

    case PlatformPathManager_iOS_Scope_Resource:
      path = [[PlatformPathManager_iOS appBundleMetaPath] stringByAppendingString:@"/"];
      break;

    default:
      break;
  }

  return path;
}

bool PlatformPathManager_iOS_GetPath(PlatformPathManager_iOS_Scope scope, char *buffer, const int MAX_PATH_LENGTH)
{
  NSString* path = [PlatformPathManager_iOS getPathWithScope:scope];

  BOOL succeeded = [path getCString:buffer
                          maxLength:MAX_PATH_LENGTH
                           encoding:NSASCIIStringEncoding];

  if(succeeded) {
    return true;
  } else {
    return false;
  }
}


@end