#import <Foundation/Foundation.h>
#import "dasConfiguration.h"
#import <DAS/DAS.h>
#import <DASClientInfo.h>
#include <string>
#include "anki/common/basestation/utils/data/dataPlatform.h"

void ConfigureDASForPlatform(Anki::Util::Data::DataPlatform * platform)
{
  @autoreleasepool {
#if TARGET_IPHONE_SIMULATOR
    DASDisableNetwork(DASDisableNetworkReason_Simulator);
#endif
    
    NSString *dasConfigPath = [[NSBundle mainBundle] pathForResource:@"DASConfig" ofType:@"json"];
    if ([[NSFileManager defaultManager] fileExistsAtPath:dasConfigPath]) {
      std::string dasLogPath = platform->pathToResource(Anki::Util::Data::Scope::Cache, "DASLogs");
      std::string gameLogPath = platform->pathToResource(Anki::Util::Data::Scope::CurrentGameLog, "");
      DASConfigure(dasConfigPath.UTF8String, dasLogPath.c_str(), gameLogPath.c_str());
      
      NSString *dasVersion = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"com.anki.das.version"];
      [[DASClientInfo sharedInfo] eventsMainStart:"build" appVersion:dasVersion.UTF8String];
    } else {
      NSLog(@"[WARNING] DAS configuration file does not exist (%@). DAS reporting is disabled.", dasConfigPath);
    }
  }
}