/**
* File: main.cpp
*
* Author: Kevin Yoon
* Created: 6/26/17
*
* Description: Cozmo Anim Process on Android
*
* Copyright: Anki, inc. 2017
*
*/

#include <stdio.h>
#include <chrono>
#include <thread>

#include "cozmoAnim/cozmoAnim.h"
#include "util/logging/logging.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"

#define TIC_TIME_MS 33

using namespace Anki;
using namespace Anki::Cozmo;

int main(void)
{
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
  
  
  std::string filesPath = "/data/local/tmp";
  std::string cachePath = "/data/local/tmp";
  std::string externalPath = "/data/local/tmp";
  std::string resourcesPath = "/data/local/tmp";
  std::string resourcesBasePath = "/data/local/tmp";
  
  
  Anki::Util::Data::DataPlatform* dataPlatform = new Anki::Util::Data::DataPlatform(filesPath, cachePath, externalPath, resourcesPath);
  
  // Create and init CozmoAnim
  CozmoAnim cozmoAnim(dataPlatform);
  
  Json::Value dummyJson;
  cozmoAnim.Init(dummyJson);
  
  auto start = std::chrono::steady_clock::now();
  auto timeOffset = start;
  
  while (1) {
    
    //BaseStationTime_t currTime_ns = std::chrono::duration<BaseStationTime_t, std::chrono::nanoseconds>(start - timeOffset);
    std::chrono::nanoseconds currTime_ns = start - timeOffset;
    cozmoAnim.Update(currTime_ns.count());
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::chrono::duration<int, std::micro> sleepTime = std::chrono::milliseconds(TIC_TIME_MS) - elapsed;
    std::this_thread::sleep_for(sleepTime);
    //printf("CozmoAnim: currTime: %d us, elapsed: %lld us, Sleep time: %d us\n", static_cast<u32>(0.001 * currTime_ns.count()), elapsed.count(), sleepTime.count());

    start = std::chrono::steady_clock::now();

  }
  
}
