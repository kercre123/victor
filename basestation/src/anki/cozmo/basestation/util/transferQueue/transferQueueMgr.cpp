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

#include "anki/cozmo/basestation/util/transferQueue/transferQueueMgr.h"
#include "anki/cozmo/basestation/util/http/abstractHttpAdapter.h"
#include "anki/cozmo/basestation/util/http/createHttpAdapter.h"
#include "anki/cozmo/basestation/util/transferQueue/iTransferable.h"
#include "util/helpers/templateHelpers.h"

#include <string>

namespace Anki {
  
  namespace Util {
    
    TransferQueueMgr::TransferQueueMgr():
    _canConnect(false),
    _numRequests(0)
    {
      _httpAdapter = nullptr;
#ifdef ANDROID
#elif LINUX
#else
      _httpAdapter = CreateHttpAdapter();
#endif
    }
    
    TransferQueueMgr::~TransferQueueMgr()
    {
      SafeDelete(_httpAdapter);
    }
    
    // Prevents any new requests from going out.
    void TransferQueueMgr::SetCanConnect(bool can_connect)
    {
      _canConnect = can_connect;
      if( _canConnect )
      {
        StartDataTransfer();
      }
    }
    bool TransferQueueMgr::GetCanConnect()
    {
      return _canConnect;
    }
    
    int  TransferQueueMgr::GetNumActiveRequests()
    {
      return _numRequests;
    }
    
    Signal::SmartHandle TransferQueueMgr::RegisterHttpTransferReadyCallback( OnTransferReadyFunc func )
    {
      Signal::SmartHandle handle = _signal.ScopedSubscribe(func);
      // We're already open, let the caller transfer now but don't call everyone
      if( _canConnect )
      {
        StartRequestFunc mgr_callback_func = std::bind(&TransferQueueMgr::StartRequest, this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
        func(mgr_callback_func);
      }
      return handle;
    }
    
    // Called from OS background threads when we have internet again....
    void TransferQueueMgr::StartDataTransfer()
    {
      StartRequestFunc mgr_callback_func = std::bind(&TransferQueueMgr::StartRequest, this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
      _signal.emit(mgr_callback_func);
    }
    
    void TransferQueueMgr::StartRequest(const HttpRequest& request, Util::Dispatch::Queue* queue, HttpRequestCallback user_callback)
    {
      if( _httpAdapter )
      {
        _numRequests++;
        
        HttpRequestCallback callback_wrapper =
        [this, user_callback] (const HttpRequest& request,
                const int responseCode,
                const std::map<std::string, std::string>& responseHeaders,
                const std::vector<uint8_t>& responseBody)
        {
          this->_numRequests--;
          // Tell the user about it.
          user_callback(request,responseCode,responseHeaders,responseBody);
        };
        
        _httpAdapter->StartRequest(request, queue, callback_wrapper);
      }
    }
    
  } // namespace Util
} // namespace Anki