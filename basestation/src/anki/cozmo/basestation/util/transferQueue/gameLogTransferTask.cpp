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

  for (const std::string& filename : filesToUpload) {
    HttpRequest request;
    request.body = FileUtils::ReadFileAsBinary(filename);
    request.method = HttpMethodPut;

    // get filename
    auto filenameStartIndex = filename.find_last_of('/');
    filenameStartIndex = (filenameStartIndex == std::string::npos) ? 0 : filenameStartIndex + 1;
    std::string baseFilename = filename.substr(filenameStartIndex);
    request.uri = "https://blobstore-dev.api.anki.com/1/cozmo/blobs/" + baseFilename;

    // add headers
    request.headers.emplace("Anki-App-Key", "toh5awu3kee1ahfaikeeGh");

    // submit request
    requestFunc(request, [filename] (const HttpRequest& innerRequest,
                                     const int responseCode,
                                     const std::map<std::string, std::string>&,
                                     const std::vector<uint8_t>&) {
      if (isHttpSuccessCode(responseCode)) {
        FileUtils::DeleteFile(filename);
        PRINT_NAMED_INFO("GameLogTransferTask", "uploaded %s", innerRequest.uri.c_str());
      }
      else {
        PRINT_NAMED_WARNING("GameLogTransferTask", "could not upload %s", innerRequest.uri.c_str());
      }
    });
  }
}

}
}
