/**
 * File: transferQueueMgr.h
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Manager for queueing data meant to be uploaded or downloaded at a later time
 *              Primary use cases for DAS and RAMS needing External delayed transfers since
 *              unlike overdrive Cozmo can't assume internet connection.
 *
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "transferQueueMgr.h"
#include "ITransferable.h"

#include <string>
// TODO: DAS is in it's own world and just gets uploaded in it's own library.
//#include "DAS.h"

namespace Anki {
  
  namespace Util {
    
    TransferQueueMgr::TransferQueueMgr()
    {
    }
    TransferQueueMgr::~TransferQueueMgr()
    {
    }
    
    // TODO: this also needs to be a platform function ( with a shared header )
    bool TransferQueueMgr::OnNativeSendGo(std::string& page_data, std::string& url)
    {
      printf("OnNativeSendGo %s\n",page_data.c_str());
      return true;
    }
    
    Signal::SmartHandle TransferQueueMgr::RegisterPushCallback( OnSendReadyFunc func )
    {
      Signal::SmartHandle handle = m_Signal.ScopedSubscribe(func);
      return handle;
    }
    
    // Called from OS background threads when we have internet again....
    void TransferQueueMgr::StartDataPull()
    {
      // TODO: tell DAS it can upload now
      //DASForceFlushNow();
      
      // Pass back any information we need in some kind of packaging...
      // In some "Page of data"
      
      std::function<bool (std::string&, std::string&)> func = std::bind(&TransferQueueMgr::OnNativeSendGo, this, std::placeholders::_1,std::placeholders::_2);
      
      m_Signal.emit(func);
      
      // TODO: call a callback native or mock test to send the strings per call...
    }
    
  } // namespace Util
} // namespace Anki