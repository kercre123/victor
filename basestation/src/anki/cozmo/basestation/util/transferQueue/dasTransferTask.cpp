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

#include "anki/cozmo/basestation/util/transferQueue/dasTransferTask.h"
#include "util/dispatchQueue/dispatchQueue.h"

#if USE_DAS
#include <DAS/DAS.h>
#endif

namespace Anki {
namespace Util {

void DasTransferTask::OnTransferReady(Dispatch::Queue* queue, const TransferQueueMgr::TaskCompleteFunc& completionFunc)
{
  #if USE_DAS
  DASForceFlushWithCallback(completionFunc);
  #else
  completionFunc();
  #endif
}

}
}
