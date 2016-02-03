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

namespace Anki {
  
  namespace Util {
    
    ITransferable::ITransferable(TransferQueueMgr* transferQueueMgr)
    {
      //m_QueueMgr = transferQueueMgr;
      
        //std::function<bool (std::string&, std::string&)> func = std::bind(&ITransferable::PushData, this, std::placeholders::_1,std::placeholders::_2);
        //transferQueueMgr->RegisterPushCallback(func);
        
        //m_QueueMgr->RegisterTransferable(shared_from_this());
      
      auto func = std::bind(&ITransferable::SendData, this, std::placeholders::_1);
      m_Handle = transferQueueMgr->RegisterPushCallback(func);
    }
    ITransferable::~ITransferable()
    {
        //m_QueueMgr->UnRegisterTransferable(shared_from_this());
    }
  } // namespace Cozmo
} // namespace Anki