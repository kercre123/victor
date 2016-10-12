/**
 * File: gameLogTransferTask
 *
 * Author: baustin
 * Created: 7/28/16
 *
 * Description: Performs upload of game logs when internet available
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/util/transferQueue/gameLogTransferTask.h"
#include "anki/cozmo/basestation/debug/devLoggingSystem.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include <algorithm>

#if USE_DAS
#include <DAS/DAS.h>
#include <DAS/DASPlatform.h>
#endif

namespace Anki {
namespace Util {

static const size_t kMaxUploadSize = 10 * 1024 * 1024;

void GameLogTransferTask::OnReady(const StartRequestFunc& requestFunc)
{
  Cozmo::DevLoggingSystem* devLoggingSystem = Cozmo::DevLoggingSystem::GetInstance();
  if (devLoggingSystem == nullptr) {
    return;
  }

  #if USE_DAS
  std::string deviceId{DASGetPlatform()->GetDeviceId()};
  #else
  std::string deviceId{"no-id"};
  #endif

  devLoggingSystem->PrepareForUpload(deviceId);
  auto filesToUpload = devLoggingSystem->GetLogFilenamesForUpload();

  size_t bytesQueued = 0;

  for (const std::string& filename : filesToUpload) {
    HttpRequest request;
    try {
      request.storageFilePath = filename;
      request.method = HttpMethodPut;

      bytesQueued += FileUtils::GetFileSize(filename);

      // get filename
      auto filenameStartIndex = filename.find_last_of('/');
      filenameStartIndex = (filenameStartIndex == std::string::npos) ? 0 : filenameStartIndex + 1;
      std::string baseFilename = filename.substr(filenameStartIndex);
      request.uri = "https://blobstore-dev.api.anki.com/1/cozmo/blobs/" + baseFilename;

      // add headers
      request.headers.emplace("Anki-App-Key", "toh5awu3kee1ahfaikeeGh");
      
      // get apprun
      Json::Value appRunData = devLoggingSystem->GetAppRunData(Cozmo::DevLoggingSystem::GetAppRunFilename(filename));
      if (!appRunData.empty()) {
        std::string fileAppRun;
        if (JsonTools::GetValueOptional(appRunData, Cozmo::DevLoggingSystem::kAppRunKey, fileAppRun))
        {
          request.headers.emplace("Usr-apprun", std::move(fileAppRun));
        }
        
        // This gets a little complicated in the interest of supporting uint64 on platforms that can:
        const Json::Value& child(appRunData[Cozmo::DevLoggingSystem::kTimeSinceEpochKey]);
        if(!child.isNull())
        {
          const auto appStartTimeSinceEpoch_ms = child.asLargestUInt();
          request.headers.emplace("Usr-Time-Since-Epoch", std::to_string(appStartTimeSinceEpoch_ms));
        }
        
        std::string fileDeviceID;
        if (JsonTools::GetValueOptional(appRunData, Cozmo::DevLoggingSystem::kDeviceIdKey, fileDeviceID))
        {
          request.headers.emplace("Usr-Deviceid", fileDeviceID);
        }
      }
    }
    catch (const std::exception& e) {
      PRINT_NAMED_WARNING("GameLogTransferTask.RequestException", "%s", e.what());
      break;
    }

    // submit request
    requestFunc(request, [filename] (const HttpRequest& innerRequest,
                                     const int responseCode,
                                     const std::map<std::string, std::string>&,
                                     const std::vector<uint8_t>&) {
      if (isHttpSuccessCode(responseCode)) {
        auto* loggingSystem = Cozmo::DevLoggingSystem::GetInstance();
        if (loggingSystem != nullptr) {
          loggingSystem->DeleteLog(filename);
        }
        else {
          FileUtils::DeleteFile(filename);
        }
        auto it = innerRequest.headers.find("Usr-apprun");
        const char* appRunId = (it == innerRequest.headers.end()) ? "" : it->second.c_str();
        PRINT_NAMED_INFO("GameLogTransferTask", "uploaded %s, apprun %s", innerRequest.uri.c_str(), appRunId);
      }
      else {
        PRINT_NAMED_WARNING("GameLogTransferTask", "could not upload %s", innerRequest.uri.c_str());
      }
    });

    if (bytesQueued > kMaxUploadSize) {
      // upload more later, don't do too much at once
      break;
    }
  }
}

}
}
