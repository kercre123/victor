/**
 * File: transferQueueMgr.h
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Manager for queueing data meant to be uploaded or downloaded at a later time
 *              Primary use cases for DAS and RAMS needing External delayed transfers since
 *              unlike overdrive Cozmo can't assume internet connection.
 *              Services that want to register should get an instance of this mgr from context class.
 *
 *
 * Example setup:
 *  Anki::Util::TestTransferUL upload_service;
 *  // Base class registers itself with mangager
 *  upload_service.Init(_context->GetTransferQueue());
 *  // Will let the TransferQueue send whatever it has
 *  _context->GetTransferQueue()->SetCanConnect(true);
 *

 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_Util_TransferQueue_TransferQueueMgr_H__
#define __Cozmo_Basestation_Util_TransferQueue_TransferQueueMgr_H__

#include "util/signals/simpleSignal.hpp"
#include "util/helpers/noncopyable.h"
#include "anki/cozmo/basestation/util/http/abstractHttpAdapter.h"


namespace Anki {
  
  namespace Util {
    
    class TransferQueueMgr : public Util::noncopyable
    {
    public:
      using StartRequestFunc = std::function<void(const HttpRequest&,Util::Dispatch::Queue*,HttpRequestCallback )>;
      using OnTransferReadyFunc = std::function<void (StartRequestFunc)>;
      
      // ----------
      TransferQueueMgr();
      virtual ~TransferQueueMgr();
      
      // Interface for native background thread
      void SetCanConnect(bool can_connect);
      bool GetCanConnect();
      int  GetNumActiveRequests();
      
      // Interface for services base class, ITransferable
      Signal::SmartHandle RegisterHttpTransferReadyCallback( OnTransferReadyFunc func );
      
    protected:
      IHttpAdapter* _httpAdapter;
      // State that says if we should be sending out requests or not.
      bool          _canConnect;
      int           _numRequests;
      
      // Sender for us, OnTransferReadyFunc
      using TransferQueueSignal = Signal::Signal< void (StartRequestFunc) >;
      TransferQueueSignal _signal;
      
      virtual void StartDataTransfer();
      
      // Binded callback of StartRequestFunc
      virtual void StartRequest(const HttpRequest& request, Util::Dispatch::Queue* queue, HttpRequestCallback callback);
      
    }; // class
    
    
  } // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Util_TransferQueue_TransferQueueMgr_H__