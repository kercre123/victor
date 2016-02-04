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

#include "util/signals/simpleSignal_fwd.h"
#include "util/signals/simpleSignal.hpp"
#include <vector>


namespace Anki {
  
  namespace Util {
    
    class ITransferable;
    
    class TransferQueueMgr
    {
    public:
      
      

      // Reciever
      using OnPushFunction = std::function<bool (std::string&, std::string&)>;
      using OnPullFunction = std::function<void(void)>;
      
      using NativeSendFunc = std::function<bool (std::string&, std::string&)>;
      using OnSendReadyFunc = std::function<bool (NativeSendFunc)>;
      
      // ----------
      
      TransferQueueMgr();
      ~TransferQueueMgr();
      
      // Instead we register Push/Pull
      Signal::SmartHandle RegisterPushCallback( OnSendReadyFunc func );
      
      // PushData, PullData
      
      // A platform specific thread tells this mgr to tell it's registered classes that they're ready?
      void StartDataPush();
      
      // Return some IDs and pointers...
      // A platform specific thread tells this mgr to tell it's registered classes that they're can upload
      virtual void StartDataPull();
      
      // Prob. need a list of IDs to confirm clear?
      void OnPullComplete();
      
      bool OnNativeSendGo(std::string& page_data, std::string& url);
      
    protected:
      
      // Sender for us
      using TestSignal = Signal::Signal< bool (NativeSendFunc) >;
      TestSignal m_Signal;
      
    }; // class
    
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_TRANSFER_QUEUE_H