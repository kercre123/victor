/**
 * File: ITransferable.h
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Base class for services that need to upload or download data when app has connection.
 *              An example might be config downloads or just text banner upsells on the main screen.
 *              Or uploading save/achievement data.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/cozmo/basestation/util/transferQueue/iTransferable.h"
#include "anki/cozmo/basestation/util/transferQueue/transferQueueMgr.h"
#include "util/dispatchQueue/dispatchQueue.h"

namespace Anki {
  
  namespace Util {
    
    ITransferable::ITransferable() : _dispatchQueue(Util::Dispatch::Create())
    {
    }
    
    void ITransferable::Init(TransferQueueMgr* transferQueueMgr)
    {
      auto func = std::bind(&ITransferable::OnTransferReady, this, std::placeholders::_1);
      _handle = transferQueueMgr->RegisterHttpTransferReadyCallback(func);
    }
    
    ITransferable::~ITransferable()
    {
      Util::Dispatch::Release(_dispatchQueue);
    }
  } // namespace Cozmo
} // namespace Anki