/**
* File: cozmoWebServerMain.cpp
*
* Author: Paul Terry
* Created: 01/29/18
*
* Description: Cozmo Web Server Process on Android
*
* Copyright: Anki, inc. 2018
*
*/
#include "webService.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/logging/androidLogPrintLogger_android.h"

#include <stdio.h>
#include <chrono>
#include <fstream>
#include <thread>

using namespace Anki;
using namespace Anki::Cozmo;

#define LOG_CHANNEL "CozmoWebServer"

namespace {
  bool gShutdown = false;
}

static void Shutdown(int signum)
{
  LOG_INFO("CozmoWebServer.Shutdown", "Shutdown on signal %d", signum);
  gShutdown = true;
}

Anki::Util::Data::DataPlatform* createPlatform(const std::string& persistentPath,
                                         const std::string& cachePath,
                                         const std::string& resourcesPath)
{
    Anki::Util::FileUtils::CreateDirectory(persistentPath);
    Anki::Util::FileUtils::CreateDirectory(cachePath);
    Anki::Util::FileUtils::CreateDirectory(resourcesPath);

    return new Anki::Util::Data::DataPlatform(persistentPath, cachePath, resourcesPath);
}

Anki::Util::Data::DataPlatform* createPlatform()
{
  char config_file_path[PATH_MAX] = { 0 };
  const char* env_config = getenv("VIC_WEB_SERVER_CONFIG");
  if (env_config != NULL) {
    strncpy(config_file_path, env_config, sizeof(config_file_path));
  }

  Json::Value config;

  printf("config_file: %s\n", config_file_path);
  if (strlen(config_file_path)) {
    std::string config_file{config_file_path};
    if (!Anki::Util::FileUtils::FileExists(config_file)) {
      fprintf(stderr, "config file not found: %s\n", config_file_path);
    }

    std::string jsonContents = Anki::Util::FileUtils::ReadFile(config_file);
    printf("jsonContents: %s\n", jsonContents.c_str());
    Json::Reader reader;
    if (!reader.parse(jsonContents, config)) {
      PRINT_STREAM_ERROR("cozmoWebServerMain.createPlatform.JsonConfigParseError",
        "json configuration parsing error: " << reader.getFormattedErrorMessages());
    }
  }

  std::string persistentPath;
  std::string cachePath;
  std::string resourcesPath;

  if (config.isMember("DataPlatformPersistentPath")) {
    persistentPath = config["DataPlatformPersistentPath"].asCString();
  } else {
    PRINT_NAMED_ERROR("cozmoWebServerMain.createPlatform.DataPlatformPersistentPathUndefined", "");
  }

  if (config.isMember("DataPlatformCachePath")) {
    cachePath = config["DataPlatformCachePath"].asCString();
  } else {
    PRINT_NAMED_ERROR("cozmoWebServerMain.createPlatform.DataPlatformCachePathUndefined", "");
  }

  if (config.isMember("DataPlatformResourcesPath")) {
    resourcesPath = config["DataPlatformResourcesPath"].asCString();
  } else {
    PRINT_NAMED_ERROR("cozmoWebServerMain.createPlatform.DataPlatformResourcesPathUndefined", "");
  }

  Util::Data::DataPlatform* dataPlatform =
    createPlatform(persistentPath, cachePath, resourcesPath);

  return dataPlatform;
}

int main(void)
{
  signal(SIGTERM, Shutdown);

  // - create and set logger
  Util::AndroidLogPrintLogger logPrintLogger("vic-webserver");
  Util::gLoggerProvider = &logPrintLogger;

  Util::Data::DataPlatform* dataPlatform = createPlatform();

  // Create and init cozmoWebServer
  Json::Value wsConfig;
  static const std::string & wsConfigPath = "webserver/webServerConfig_standalone.json";
  const bool success = dataPlatform->readAsJson(Util::Data::Scope::Resources, wsConfigPath, wsConfig);
  if (!success)
  {
    LOG_ERROR("CozmoWebServerMain.WebServerConfigNotFound",
              "Web server config file %s not found or failed to parse",
              wsConfigPath.c_str());
    exit(1);
  }

  auto cozmoWebServer = std::make_unique<WebService::WebService>();
  cozmoWebServer->Start(dataPlatform, wsConfig);

  using namespace std::chrono;
  using TimeClock = steady_clock;

  const auto runStart = TimeClock::now();

  // Set the target time for the end of the first frame
  auto targetEndFrameTime = runStart + (microseconds)(WEB_SERVER_TIME_STEP_US);


  while (!gShutdown)
  {
    cozmoWebServer->Update();

    const auto tickNow = TimeClock::now();
    const auto remaining_us = duration_cast<microseconds>(targetEndFrameTime - tickNow);

    // Complain if we're going overtime
    if (remaining_us < microseconds(-WEB_SERVER_OVERTIME_WARNING_THRESH_US))
    {
      PRINT_NAMED_WARNING("cozmoWebServer.overtime", "Update() (%dms max) is behind by %.3fms",
                          WEB_SERVER_TIME_STEP_MS, (float)(-remaining_us).count() * 0.001f);
    }

    // Now we ALWAYS sleep, but if we're overtime, we 'sleep zero' which still
    // allows other threads to run
    static const auto minimumSleepTime_us = microseconds((long)0);
    std::this_thread::sleep_for(std::max(minimumSleepTime_us, remaining_us));

    // Set the target end time for the next frame
    targetEndFrameTime += (microseconds)(WEB_SERVER_TIME_STEP_US);

    // See if we've fallen very far behind (this happens e.g. after a 5-second blocking
    // load operation); if so, compensate by catching the target frame end time up somewhat.
    // This is so that we don't spend the next SEVERAL frames catching up.
    const auto timeBehind_us = -remaining_us;
    static const auto kusPerFrame = ((microseconds)(WEB_SERVER_TIME_STEP_US)).count();
    static const int kTooFarBehindFramesThreshold = 2;
    static const auto kTooFarBehindThreshold = (microseconds)(kTooFarBehindFramesThreshold * kusPerFrame);
    if (timeBehind_us >= kTooFarBehindThreshold)
    {
      const int framesBehind = (int)(timeBehind_us.count() / kusPerFrame);
      const auto forwardJumpDuration = kusPerFrame * framesBehind;
      targetEndFrameTime += (microseconds)forwardJumpDuration;
      PRINT_NAMED_WARNING("cozmoWebServer.catchup",
                          "Update was too far behind so moving target end frame time forward by an additional %.3fms",
                          (float)(forwardJumpDuration * 0.001f));
    }
  }

  LOG_INFO("CozmoWebServer.main", "Stopping web server");
  cozmoWebServer.reset();

  LOG_INFO("CozmoWebServer.main", "exit(0)");
  Util::gLoggerProvider = nullptr;
  exit(0);

}
