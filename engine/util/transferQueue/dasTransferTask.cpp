/**
 * File: dasTransferTask
 *
 * Author: baustin
 * Created: 7/20/16
 *
 * Description: Alerts DAS to perform upload when internet connection is available
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/util/transferQueue/dasTransferTask.h"
#include "util/dispatchQueue/dispatchQueue.h"

#if USE_DAS
#include "util/logging/logging.h"
#include <DAS/DAS.h>
#endif

namespace Anki {
namespace Util {

void DasTransferTask::OnTransferReady(Dispatch::Queue* queue, const TransferQueueMgr::TaskCompleteFunc& completionFunc)
{
  #if USE_DAS
  auto callbackWrapper = [completionFunc] (bool success, std::string response) {
    (void) response;
    LOG_EVENT(success ? "das.upload" : "das.upload.fail", "background");
    completionFunc();
  };
  DASForceFlushWithCallback(callbackWrapper);
  #else
  completionFunc();
  #endif
}

}
}
