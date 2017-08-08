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

#ifndef ANKI_COZMO_BASETATION_TRANSFERTASK_H
#define ANKI_COZMO_BASETATION_TRANSFERTASK_H

#include "util/signals/signalHolder.h"
#include "engine/util/transferQueue/transferQueueMgr.h"

namespace Anki {
namespace Util {

class TransferQueueMgr;

class TransferTask : private SignalHolder
{
public:
  virtual ~TransferTask() = default;

  void Init(TransferQueueMgr* transferQueueMgr);

protected:
  // Called by TransferQueueMgr after Init()
  virtual void OnTransferReady(Dispatch::Queue* queue, const TransferQueueMgr::TaskCompleteFunc& completionFunc) = 0;
};


} // namespace Util
} // namespace Anki

#endif
