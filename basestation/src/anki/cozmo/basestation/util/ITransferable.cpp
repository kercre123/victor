/**
 * File: ITransferable.h
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Testcase for push data from the server.
 *              An example might be config downloads or just text banner upsells on the main screen.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "ITransferable.h"
#include "transferQueueMgr.h"
#include "util/dispatchQueue/dispatchQueue.h"

namespace Anki {
  
  namespace Util {
    
    ITransferable::ITransferable() : m_DispatchQueue(Util::Dispatch::Create())
    {
    }
    
    void ITransferable::Init(TransferQueueMgr* transferQueueMgr)
    {
      auto func = std::bind(&ITransferable::OnTransferReady, this, std::placeholders::_1);
      m_Handle = transferQueueMgr->RegisterHttpTransferReadyCallback(func);
    }
    
    ITransferable::~ITransferable()
    {
      Util::Dispatch::Release(m_DispatchQueue);
    }
  } // namespace Cozmo
} // namespace Anki