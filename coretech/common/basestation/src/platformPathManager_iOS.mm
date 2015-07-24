#import <Foundation/Foundation.h>

#import "anki/common/basestation/platformPathManager_iOS_interface.h"

@implementation PlatformPathManager_iOS

+ (NSString*)appBundleMetaPath
{
  NSString* metaPath = nil;
  if (!metaPath) {
    metaPath = [[NSBundle mainBundle] pathForResource:@"cozmo_resources/meta" ofType:@""];
  }

  NSLog(@"appBundleMetaPath %@", metaPath);

  return metaPath;
}

+ (NSString*)appBundleAssetPath
{
  NSString* assetPath = nil;
  if (!assetPath) {
    assetPath = [[NSBundle mainBundle] pathForResource:@"cozmo_resources/asset" ofType:@""];
  }

  NSLog(@"appBundleAssetPath %@", assetPath);

  return assetPath;
}

+ (NSString*)getPathWithScope:(PlatformPathManager_iOS_Scope)scope
{
  NSString *path = nil;

  switch (scope) {
    case PlatformPathManager_iOS_Scope_Test:
    {
      NSString* fullPath = nil;
      if (!fullPath) {
        fullPath = [[PlatformPathManager_iOS appBundleMetaPath] stringByAppendingString:@"/"];
      }
      path = [fullPath copy];
    }
      break;

    case PlatformPathManager_iOS_Scope_Config:
    {
      NSString* fullPath = nil;
      if (!fullPath) {
        fullPath = [[PlatformPathManager_iOS appBundleMetaPath] stringByAppendingString:@"/basestation/config/"];
      }
      path = [fullPath copy];
    }
      break;

    case PlatformPathManager_iOS_Scope_Animation:
    {
      NSString* fullPath = nil;
      if (!fullPath) {
        fullPath = [[PlatformPathManager_iOS appBundleAssetPath] stringByAppendingString:@"/animations/"];
      }
      path = [fullPath copy];
    }
      break;

    case PlatformPathManager_iOS_Scope_Sound:
    {
      NSString* fullPath = nil;
      if (!fullPath) {
        fullPath = [[PlatformPathManager_iOS appBundleAssetPath] stringByAppendingString:@"/sounds/"];
      }
      path = [fullPath copy];
    }
      break;

    case PlatformPathManager_iOS_Scope_FaceAnimation:
    {
      NSString* fullPath = nil;
      if (!fullPath) {
        fullPath = [[PlatformPathManager_iOS appBundleAssetPath] stringByAppendingString:@"/faceAnimation/"];
      }
      path = [fullPath copy];
    }
      break;
      
    case PlatformPathManager_iOS_Scope_Resource:
    {
      NSString* fullPath = nil;
      if (!fullPath) {
        fullPath = [[PlatformPathManager_iOS appBundleMetaPath] stringByAppendingString:@"/"];
      }
      path = [fullPath copy];
    }
      break;

    default:
      break;
  }

  NSLog(@"getPathWithScope %@", path);

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