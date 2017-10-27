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
#include "cozmoAnim/cozmoAnim.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"

#include "util/logging/logging.h"
#include "util/logging/androidLogPrintLogger_android.h"

#include <stdio.h>
#include <chrono>
#include <fstream>
#include <thread>

#define TIC_TIME_MS 33

using namespace Anki;
using namespace Anki::Cozmo;

namespace {
CozmoAnimEngine* cozmoAnim = nullptr;
}

void Cleanup(int signum)
{
  if(cozmoAnim != nullptr)
  {
    delete cozmoAnim;
    cozmoAnim = nullptr;
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
  
  // Create and init CozmoAnim
  cozmoAnim = new CozmoAnimEngine(dataPlatform);
  
  cozmoAnim->Init();
  
  auto start = std::chrono::steady_clock::now();
  const auto timeOffset = start;
  
  while (1) {

    std::chrono::nanoseconds currTime_ns = start - timeOffset;
    if(cozmoAnim != nullptr)
    {
      cozmoAnim->Update(currTime_ns.count());
    }
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::chrono::duration<int, std::micro> sleepTime = std::chrono::milliseconds(TIC_TIME_MS) - elapsed;
    std::this_thread::sleep_for(sleepTime);
    //printf("CozmoAnim: currTime: %d us, elapsed: %lld us, Sleep time: %d us\n", static_cast<u32>(0.001 * currTime_ns.count()), elapsed.count(), sleepTime.count());

    start = std::chrono::steady_clock::now();

  }
  
}
