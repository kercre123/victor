#import "dasConfiguration.h"
#import <DAS/DAS.h>
#import <DAS/dasPlatform_ios.h>
#include "anki/common/basestation/utils/data/dataPlatform.h"

void ConfigureDASForPlatform(Anki::Util::Data::DataPlatform * platform, const std::string& apprun)
{
#ifdef TARGET_IPHONE_SIMULATOR
  DASDisableNetwork(DASDisableNetworkReason_Simulator);
#endif

  std::unique_ptr<DAS::DASPlatform_IOS> dasPlatform{new DAS::DASPlatform_IOS(apprun)};
  dasPlatform->Init();
  DASNativeInit(std::move(dasPlatform), "cozmo");
}
