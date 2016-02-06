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
#include "http/abstractHttpAdapter.h"
#include "ITransferable.h"
#include "util/helpers/templateHelpers.h"

#ifdef ANDROID
#elif LINUX
#else
#include "http/CreateHttpAdapter_ios.h"
#endif

#include <string>
// TODO: DAS is in it's own world and just gets uploaded in it's own library.
//#include "DAS.h"

namespace Anki {
  
  namespace Util {
    
    TransferQueueMgr::TransferQueueMgr():
    m_CanConnect(false),
    m_NumRequests(0)
    {
      m_HttpAdapter = nullptr;
#ifdef ANDROID
#elif LINUX
#else
      m_HttpAdapter = CreateHttpAdapter();
#endif
    }
    
    TransferQueueMgr::~TransferQueueMgr()
    {
      SafeDelete(m_HttpAdapter);
    }
    
    // Prevents any new requests from going out.
    void TransferQueueMgr::SetCanConnect(bool can_connect)
    {
      m_CanConnect = can_connect;
      if( m_CanConnect )
      {
        StartDataTransfer();
      }
    }
    bool TransferQueueMgr::GetCanConnect()
    {
      return m_CanConnect;
    }
    
    int  TransferQueueMgr::GetNumActiveRequests()
    {
      return m_NumRequests;
    }
    
    Signal::SmartHandle TransferQueueMgr::RegisterHttpTransferReadyCallback( OnTransferReadyFunc func )
    {
      Signal::SmartHandle handle = m_Signal.ScopedSubscribe(func);
      // We're already open, let the caller transfer now but don't call everyone
      if( m_CanConnect )
      {
        StartRequestFunc mgr_callback_func = std::bind(&TransferQueueMgr::StartRequest, this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
        func(mgr_callback_func);
      }
      return handle;
    }
    
    // Called from OS background threads when we have internet again....
    void TransferQueueMgr::StartDataTransfer()
    {
      // TODO: tell DAS it can upload now
      //DASForceFlushNow();
      
      StartRequestFunc mgr_callback_func = std::bind(&TransferQueueMgr::StartRequest, this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
      m_Signal.emit(mgr_callback_func);
    }
    
    void TransferQueueMgr::StartRequest(const HttpRequest& request, Util::Dispatch::Queue* queue, HttpRequestCallback user_callback)
    {
      if( m_HttpAdapter )
      {
        m_NumRequests++;
        
        HttpRequestCallback callback_wrapper =
        [this, user_callback] (const HttpRequest& request,
                const int responseCode,
                const std::map<std::string, std::string>& responseHeaders,
                const std::vector<uint8_t>& responseBody)
        {
          this->m_NumRequests--;
          // Tell the user about it.
          user_callback(request,responseCode,responseHeaders,responseBody);
        };
        
        m_HttpAdapter->StartRequest(request, queue, callback_wrapper);
      }
    }
    
  } // namespace Util
} // namespace Anki