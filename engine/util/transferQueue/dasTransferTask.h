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

#ifndef ANKI_COZMO_BASETATION_DASTRANSFERTASK_H
#define ANKI_COZMO_BASETATION_DASTRANSFERTASK_H

#include "engine/util/transferQueue/transferTask.h"

namespace Anki {
namespace Util {

class DasTransferTask : public TransferTask
{
private:
  virtual void OnTransferReady(Dispatch::Queue* queue, const TransferQueueMgr::TaskCompleteFunc& completionFunc) override;
};

}
}

#endif
