#include "camera/cameraService.h"
#include "cubeBleClient/cubeBleClient.h"
#include "gtest/gtest.h"
#include "osState/osState.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"

namespace Anki {
namespace Cozmo {
  CONSOLE_VAR_EXTERN(bool, kProcFace_HotspotRender)
  CONSOLE_VAR_EXTERN(s32, kProcFace_AntiAliasingSize)
  CONSOLE_VAR_EXTERN(s32, kProcFace_AntiAliasingSize)
  CONSOLE_VAR_EXTERN(uint8_t, kProcFace_AntiAliasingFilter);
}
}

using namespace Anki;
using namespace Cozmo;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST(Cozmo, SimpleCozmoTest)
{
  ASSERT_TRUE(true);
}

#define CONFIGROOT "ANKICONFIGROOT"
#define WORKROOT "ANKIWORKROOT"

std::string resourcePath; // This is externed and used by tests

int main(int argc, char ** argv)
{
  // For victor rendering
  kProcFace_HotspotRender = true;
  kProcFace_AntiAliasingSize = 3;
  kProcFace_AntiAliasingFilter = 1;

  //LEAKING HERE
  Anki::Util::PrintfLoggerProvider* loggerProvider = new Anki::Util::PrintfLoggerProvider();
  loggerProvider->SetMinLogLevel(Anki::Util::LOG_LEVEL_DEBUG);
  Anki::Util::gLoggerProvider = loggerProvider;


  std::string configRoot;
  char* configRootChars = getenv(CONFIGROOT);
  if (configRootChars != NULL) {
    configRoot = configRootChars;
  }

  std::string workRoot;
  char* workRootChars = getenv(WORKROOT);
  if (workRootChars != NULL)
    workRoot = workRootChars;

  if (configRoot.empty() || workRoot.empty()) {
    char cwdPath[1256];
    getcwd(cwdPath, 1255);
    PRINT_NAMED_INFO("CozmoTests.main","cwdPath %s", cwdPath);
    PRINT_NAMED_INFO("CozmoTests.main","executable name %s", argv[0]);
/*  // still troubleshooting different run time environments,
    // need to find a way to detect where the resources folder is located on disk.
    // currently this is relative to the executable.
    // Another option is to pass it in through the environment variables.
    // Get the last position of '/'
    std::string aux(argv[0]);
#if defined(_WIN32) || defined(WIN32)
    size_t pos = aux.rfind('\\');
#else
    size_t pos = aux.rfind('/');
#endif
    std::string path = aux.substr(0,pos);
*/
    std::string path = cwdPath;
    resourcePath = path + "/../../data/assets/cozmo_resources";
  } else {
    // build server specifies configRoot and workRoot
    resourcePath = configRoot + "/resources";
  }

  // Suppress break-on-error for duration of these tests
  Anki::Util::_errBreakOnError = false;

  // Initialize AndroidHAL singleton without supervisor
  CameraService::SetSupervisor(nullptr);

  // Initialize OSState singleton without supervisor
  OSState::SetSupervisor(nullptr);

  // Initialize CubeBleClient singleton without supervisor
  CubeBleClient::SetSupervisor(nullptr);

  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
