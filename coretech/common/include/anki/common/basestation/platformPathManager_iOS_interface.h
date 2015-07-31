#ifndef ANKI_COMMON_PLATFORM_PATH_MANAGER_IOS_H_
#define ANKI_COMMON_PLATFORM_PATH_MANAGER_IOS_H_

#ifndef NS_ENUM
#define NS_ENUM(_type, _name) enum _name : _type _name; enum _name : _type
#endif

typedef NS_ENUM(int, PlatformPathManager_iOS_Scope) {
  PlatformPathManager_iOS_Scope_Test = 0,
  PlatformPathManager_iOS_Scope_Config,
  PlatformPathManager_iOS_Scope_Animation,
  PlatformPathManager_iOS_Scope_Sound,
  PlatformPathManager_iOS_Scope_FaceAnimation,
  PlatformPathManager_iOS_Scope_Resource,
  //Temp
  PlatformPathManager_iOS_Scope_Count
};


bool PlatformPathManager_iOS_GetPath(PlatformPathManager_iOS_Scope scope, char *buffer, const int MAX_PATH_LENGTH);

#endif // ANKI_COMMON_PLATFORM_PATH_MANAGER_IOS_H_


#ifdef __OBJC__

@interface PlatformPathManager_iOS : NSObject

+ (NSString*)getPathWithScope:(PlatformPathManager_iOS_Scope)scope;

@end

#endif // __OBJC__
