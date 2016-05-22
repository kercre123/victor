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
    
    NSString *dasVersion = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"com.anki.das.version"];
    [[DASClientInfo sharedInfo] eventsMainStart:"build" appVersion:dasVersion.UTF8String product:"cozmo"];
  }
}
