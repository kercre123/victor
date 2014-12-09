#import <Foundation/Foundation.h>

#import "anki/common/basestation/platformPathManager_iOS_interface.h"

@implementation PlatformPathManager_iOS

+(NSString*) GetConfigPath
{
  return [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
}


bool PlatformPathManager_iOS_GetConfigPath(char *buffer, const int MAX_PATH_LENGTH)
{
  NSString* path = [PlatformPathManager_iOS GetConfigPath];
  
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