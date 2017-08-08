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

#ifndef ANKI_COZMO_BASETATION_GAMELOGTRANSFERTASK_H
#define ANKI_COZMO_BASETATION_GAMELOGTRANSFERTASK_H

#include "engine/util/transferQueue/transferTaskHttp.h"

namespace Anki {
namespace Util {

class GameLogTransferTask : public TransferTaskHttp
{
private:
  virtual void OnReady(const StartRequestFunc& requestFunc) override;
};

}
}

#endif
