#ifndef ANKI_COMMON_PLATFORM_PATH_MANAGER_IOS_H_
#define ANKI_COMMON_PLATFORM_PATH_MANAGER_IOS_H_

bool PlatformPathManager_iOS_GetConfigPath(char *buffer, const int MAX_PATH_LENGTH);

#endif // ANKI_COMMON_PLATFORM_PATH_MANAGER_IOS_H_



#if __OBJC__

@interface PlatformPathManager_iOS : NSObject

+(NSString*) GetConfigPath;

@end

#endif // __OBJC__
