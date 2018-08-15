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

#include "util/helpers/noncopyable.h"
#include "util/http/abstractHttpAdapter.h"
#include "util/signals/simpleSignal.hpp"
#include <mutex>


namespace Anki {
  
  namespace Util {
    
    class TransferQueueMgr : public Util::noncopyable
    {
    public:
      using TaskCompleteFunc = std::function<void()>;
      using OnTransferReadyFunc = std::function<void(Dispatch::Queue*, const TaskCompleteFunc&)>;
      
      // ----------
      TransferQueueMgr();
      virtual ~TransferQueueMgr();
      
      void ExecuteTransfers();
      
      Signal::SmartHandle RegisterTask(const OnTransferReadyFunc& func);
      
    protected:
      Dispatch::Queue* _queue;
      int _numRequests;

      Signal::Signal<void(Dispatch::Queue*, const TaskCompleteFunc&)> _signal;
      std::mutex _mutex;
      std::condition_variable _waitVar;
      
      void StartDataTransfer();
      
    }; // class
    
    
  } // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Util_TransferQueue_TransferQueueMgr_H__