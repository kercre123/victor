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
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_TRANSFER_QUEUE_H
#define ANKI_COZMO_TRANSFER_QUEUE_H

#include "util/signals/simpleSignal.hpp"

#include "http/abstractHttpAdapter.h"


namespace Anki {
  
  namespace Util {
    
    class TransferQueueMgr
    {
    public:
      using StartRequestFunc = std::function<void(const HttpRequest&,Util::Dispatch::Queue*,HttpRequestCallback )>;
      using OnTransferReadyFunc = std::function<void (StartRequestFunc)>;
      
      // ----------
      TransferQueueMgr(IHttpAdapter* httpAdapter);
      ~TransferQueueMgr();
      
      // Interface for native background service
      void SetCanConnect(bool can_connect);
      bool GetCanConnect();
      int  GetNumActiveRequests();
      
      // Interface for services.
      Signal::SmartHandle RegisterHttpTransferReadyCallback( OnTransferReadyFunc func );
      
    protected:
      IHttpAdapter* m_HttpAdapter;
      // State that says if we should be sending out requests or not.
      bool          m_CanConnect;
      int           m_NumRequests;
      
      // Sender for us, OnTransferReadyFunc
      using TransferQueueSignal = Signal::Signal< void (StartRequestFunc) >;
      TransferQueueSignal m_Signal;
      
      virtual void StartDataTransfer();
      
      // Binded callback of StartRequestFunc
      virtual void StartRequest(const HttpRequest& request, Util::Dispatch::Queue* queue, HttpRequestCallback callback);
      
    }; // class
    
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_TRANSFER_QUEUE_H