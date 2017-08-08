/**
 * File: transferTask
 *
 * Author: baustin
 * Created: 7/20/16
 *
 * Description: Hook up to TaskQueueMgr to execute a task when we get connectivity
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/util/transferQueue/transferTask.h"
#include <functional>

namespace Anki {
namespace Util {

void TransferTask::Init(TransferQueueMgr* transferQueueMgr)
{
  auto callback = std::bind(&TransferTask::OnTransferReady, this, std::placeholders::_1, std::placeholders::_2);
  AddSignalHandle(transferQueueMgr->RegisterTask(callback));
}

}
}
