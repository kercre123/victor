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
#include "util/logging/androidLogPrintLogger_android.h"

#include <stdio.h>
#include <chrono>
#include <fstream>
#include <thread>

using namespace Anki;
using namespace Anki::Cozmo;

namespace {
AnimEngine* animEngine = nullptr;
}

void Cleanup(int signum)
{
  if (animEngine != nullptr)
  {
    delete animEngine;
    animEngine = nullptr;
  }
  
  exit(signum);
}

int main(void)
{
  signal(SIGTERM, Cleanup);
  
  // - create and set logger
  Util::AndroidLogPrintLogger logPrintLogger("anim");
  Util::gLoggerProvider = &logPrintLogger;

  // TODO: Load DataPlatform paths from json or however engine does it
  /*
  const char* configuration_data = "{}";
  
  Json::Reader reader;
  Json::Value config;
  if (!reader.parse(configuration_data, configuration_data + std::strlen(configuration_data), config)) {
    PRINT_STREAM_ERROR("cozmo_startup", "json configuration parsing error: " << reader.getFormattedErrorMessages());
    return -1;
  }
  
  // Create the data platform with the folders sent from Unity
  std::string filesPath = config["DataPlatformFilesPath"].asCString();
  std::string cachePath = config["DataPlatformCachePath"].asCString();
  std::string externalPath = config["DataPlatformExternalPath"].asCString();
  std::string resourcesPath = config["DataPlatformResourcesPath"].asCString();
  std::string resourcesBasePath = config["DataPlatformResourcesBasePath"].asCString();
  std::string appRunId = config["appRunId"].asCString();
  */
  
  
  // Check /data/data/com.anki.cozmoengine/files/assets/current for the hash of the assets directory to use
  std::string rootDir = "/data/data/com.anki.cozmoengine";
  std::string assetHash;
  std::string assetHashFileName = rootDir + "/files/assets/current";
  std::ifstream assetHashFile(assetHashFileName.c_str());
  if (assetHashFile.is_open()) {
    getline(assetHashFile, assetHash);
    assetHashFile.close();
    PRINT_NAMED_INFO("main.AssetHashFound", "%s", assetHash.c_str());
  } else {
    PRINT_NAMED_ERROR("main.AssetHashFileNotFound", "%s not found", assetHashFileName.c_str());
    exit(-1);
  }
  
  
  std::string filesPath = rootDir + "/files/output";
  std::string cachePath = rootDir + "/cache";
  std::string externalPath = "/data/local/tmp";
  std::string resourcesPath = rootDir + "/files/assets/" + assetHash + "/cozmo_resources";
  
  
  Util::Data::DataPlatform* dataPlatform = new Util::Data::DataPlatform(filesPath, cachePath, externalPath, resourcesPath);
  
  // Create and init AnimEngine
  animEngine = new AnimEngine(dataPlatform);
  
  animEngine->Init();
  
  using namespace std::chrono;
  using TimeClock = steady_clock;

  const auto runStart = TimeClock::now();
  
  // Set the target time for the end of the first frame
  auto targetEndFrameTime = runStart + (microseconds)(ANIM_TIME_STEP_US);
  

  while (1) {

    const auto tickStart = TimeClock::now();
    const duration<double> curTime_s = tickStart - runStart;
    const BaseStationTime_t curTime_ns = Util::numeric_cast<BaseStationTime_t>(Util::SecToNanoSec(curTime_s.count()));

    if (animEngine->Update(curTime_ns) != RESULT_OK) {
      PRINT_NAMED_WARNING("CozmoAnimMain.Update.Failed", "Exiting...");
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
}
