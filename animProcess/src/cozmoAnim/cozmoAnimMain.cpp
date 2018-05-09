/**
* File: cozmoAnimMain.cpp
*
* Author: Kevin Yoon
* Created: 6/26/17
*
* Description: Cozmo Anim Process on Android
*
* Copyright: Anki, inc. 2017
*
*/
#include "cozmoAnim/animEngine.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "util/logging/logging.h"
#include "util/logging/androidLogPrintLogger_vicos.h"
#include "util/fileUtils/fileUtils.h"


#include <stdio.h>
#include <chrono>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <csignal>

using namespace Anki;
using namespace Anki::Cozmo;

#define LOG_CHANNEL "CozmoAnimMain"

namespace {
  bool gShutdown = false;
}

static void Shutdown(int signum)
{
  LOG_INFO("CozmoAnimMain.Shutdown", "Shutdown on signal %d", signum);
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
  const char* env_config = getenv("VIC_ANIM_CONFIG");
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
      PRINT_STREAM_ERROR("cozmo_startup",
        "json configuration parsing error: " << reader.getFormattedErrorMessages());
    }
  }

  std::string persistentPath;
  std::string cachePath;
  std::string resourcesPath;

  if (config.isMember("DataPlatformPersistentPath")) {
    persistentPath = config["DataPlatformPersistentPath"].asCString();
  } else {
    PRINT_NAMED_ERROR("cozmoAnimMain.createPlatform.DataPlatformPersistentPathUndefined", "");
  }

  if (config.isMember("DataPlatformCachePath")) {
    cachePath = config["DataPlatformCachePath"].asCString();
  } else {
    PRINT_NAMED_ERROR("cozmoAnimMain.createPlatform.DataPlatformCachePathUndefined", "");
  }

  if (config.isMember("DataPlatformResourcesPath")) {
    resourcesPath = config["DataPlatformResourcesPath"].asCString();
  } else {
    PRINT_NAMED_ERROR("cozmoAnimMain.createPlatform.DataPlatformResourcesPathUndefined", "");
  }

  Util::Data::DataPlatform* dataPlatform =
    createPlatform(persistentPath, cachePath, resourcesPath);

  return dataPlatform;
}


int main(void)
{
  signal(SIGTERM, Shutdown);

  // - create and set logger
  Util::AndroidLogPrintLogger logPrintLogger("vic-anim");
  Util::gLoggerProvider = &logPrintLogger;

  Util::Data::DataPlatform* dataPlatform = createPlatform();

  // Create and init AnimEngine
  AnimEngine * animEngine = new AnimEngine(dataPlatform);

  Result result = animEngine->Init();
  if (RESULT_OK != result) {
    LOG_ERROR("CozmoAnimMain.main.InitFailed", "Unable to initialize (exit %d)", result);
    delete animEngine;
    sync();
    exit(result);
  }

  using namespace std::chrono;
  using TimeClock = steady_clock;

  const auto runStart = TimeClock::now();

  // Set the target time for the end of the first frame
  auto targetEndFrameTime = runStart + (microseconds)(ANIM_TIME_STEP_US);

  // Loop until shutdown or error
  while (!gShutdown) {

    const auto tickStart = TimeClock::now();
    const duration<double> curTime_s = tickStart - runStart;
    const BaseStationTime_t curTime_ns = Util::numeric_cast<BaseStationTime_t>(Util::SecToNanoSec(curTime_s.count()));

    result = animEngine->Update(curTime_ns);
    if (RESULT_OK != result) {
      PRINT_NAMED_WARNING("CozmoAnimMain.main.UpdateFailed", "Unable to update (result %d)", result);
      break;
    }

    const auto tickNow = TimeClock::now();
    const auto remaining_us = duration_cast<microseconds>(targetEndFrameTime - tickNow);

    // Complain if we're going overtime
    if (remaining_us < microseconds(-ANIM_OVERTIME_WARNING_THRESH_US))
    {
      //const auto tickDuration_us = duration_cast<microseconds>(tickNow - tickStart);
      //PRINT_NAMED_INFO("CozmoAPI.CozmoInstanceRunner", "targetEndFrameTime:%8lld, tickDuration_us:%8lld, remaining_us:%8lld",
      //                 TimeClock::time_point(targetEndFrameTime).time_since_epoch().count(), tickDuration_us.count(), remaining_us.count());

      PRINT_NAMED_WARNING("CozmoAnimMain.overtime", "Update() (%dms max) is behind by %.3fms",
                          ANIM_TIME_STEP_MS, (float)(-remaining_us).count() * 0.001f);
    }

    // Now we ALWAYS sleep, but if we're overtime, we 'sleep zero' which still
    // allows other threads to run
    static const auto minimumSleepTime_us = microseconds((long)0);
    std::this_thread::sleep_for(std::max(minimumSleepTime_us, remaining_us));

    // Set the target end time for the next frame
    targetEndFrameTime += (microseconds)(ANIM_TIME_STEP_US);

    // See if we've fallen very far behind (this happens e.g. after a 5-second blocking
    // load operation); if so, compensate by catching the target frame end time up somewhat.
    // This is so that we don't spend the next SEVERAL frames catching up.
    const auto timeBehind_us = -remaining_us;
    static const auto kusPerFrame = ((microseconds)(ANIM_TIME_STEP_US)).count();
    static const int kTooFarBehindFramesThreshold = 2;
    static const auto kTooFarBehindThreshold = (microseconds)(kTooFarBehindFramesThreshold * kusPerFrame);
    if (timeBehind_us >= kTooFarBehindThreshold)
    {
      const int framesBehind = (int)(timeBehind_us.count() / kusPerFrame);
      const auto forwardJumpDuration = kusPerFrame * framesBehind;
      targetEndFrameTime += (microseconds)forwardJumpDuration;
      PRINT_NAMED_WARNING("CozmoAnimMain.catchup",
                          "Update was too far behind so moving target end frame time forward by an additional %.3fms",
                          (float)(forwardJumpDuration * 0.001f));
    }

  }

  LOG_INFO("CozmoAnimMain.main.Shutdown", "Shutting down (exit %d)", result);

  delete animEngine;
  sync();
  exit(result);
}
