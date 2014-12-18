#ifndef ANKI_COMMON_PLATFORM_PATH_MANAGER_IOS_H_
#define ANKI_COMMON_PLATFORM_PATH_MANAGER_IOS_H_

enum PlatformPathManager_iOS_Scope {
  PlatformPathManager_iOS_Scope_Test,
  PlatformPathManager_iOS_Scope_Config,
  PlatformPathManager_iOS_Scope_Animation,
  PlatformPathManager_iOS_Scope_Sound,
  PlatformPathManager_iOS_Scope_Resource
  //Temp
};

bool PlatformPathManager_iOS_GetPath(PlatformPathManager_iOS_Scope scope, char *buffer, const int MAX_PATH_LENGTH);

#endif // ANKI_COMMON_PLATFORM_PATH_MANAGER_IOS_H_


#if __OBJC__

@interface PlatformPathManager_iOS : NSObject

+ (NSString*)getPathWithScope:(PlatformPathManager_iOS_Scope)scope;

@end

#endif // __OBJC__
