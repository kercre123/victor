#import "HockeyAppUnity.h"
#import "HockeyAppUnityWrapper.h"

void HockeyApp_StartHockeyManager(char *appID, char *serverURL, char *authType, char *secret, bool updateManagerEnabled, bool autoSendEnabled) {
  
  [HockeyAppUnity startManagerWithIdentifier:[NSString stringWithUTF8String:appID]
                                   serverURL:[NSString stringWithUTF8String:serverURL]
                                    authType:[NSString stringWithUTF8String:authType]
                                      secret:[NSString stringWithUTF8String:secret]
                        updateManagerEnabled:updateManagerEnabled
                             autoSendEnabled:autoSendEnabled];
}

void HockeyApp_ShowFeedbackListView() {
  
  [HockeyAppUnity showFeedbackListView];
}

char* HockeyApp_GetVersionCode() {
  
  const char* versionCode = [[HockeyAppUnity versionCode] UTF8String];
  char* res = (char*)malloc(strlen(versionCode) + 1);
  strcpy(res, versionCode);
  
  return res;
}

char* HockeyApp_GetVersionName() {
  
  const char* versionName = [[HockeyAppUnity versionName] UTF8String];
  char* res = (char*)malloc(strlen(versionName) + 1);
  strcpy(res, versionName);
  
  return res;
}

char* HockeyApp_GetSdkVersion() {
  
  const char* sdkVersion = [[HockeyAppUnity sdkVersion] UTF8String];
  char* res = (char*)malloc(strlen(sdkVersion) + 1);
  strcpy(res, sdkVersion);
  
  return res;
}

char* HockeyApp_GetSdkName() {
  
  const char* sdkName = [[HockeyAppUnity sdkName] UTF8String];
  char* res = (char*)malloc(strlen(sdkName) + 1);
  strcpy(res, sdkName);
  
  return res;
}

char* HockeyApp_GetBundleIdentifier() {
  
  const char* bundleIdentifier = [[HockeyAppUnity bundleIdentifier] UTF8String];
  char* res = (char*)malloc(strlen(bundleIdentifier) + 1);
  strcpy(res, bundleIdentifier);
  
  return res;
}
