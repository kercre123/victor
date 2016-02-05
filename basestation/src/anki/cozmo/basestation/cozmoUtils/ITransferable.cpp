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
    
    ITransferable::ITransferable(TransferQueueMgr* transferQueueMgr) :
      m_DispatchQueue(Util::Dispatch::Create()),
      m_QueueMgr(transferQueueMgr)
    {
    }
    
    void ITransferable::Init()
    {
      auto func = std::bind(&ITransferable::OnTransferReady, this, std::placeholders::_1);
      m_Handle = m_QueueMgr->RegisterHttpTransferReadyCallback(func);
    }
    
    ITransferable::~ITransferable()
    {
      Util::Dispatch::Release(m_DispatchQueue);
    }
  } // namespace Cozmo
} // namespace Anki