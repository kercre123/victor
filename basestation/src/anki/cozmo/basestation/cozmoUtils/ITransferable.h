/**
 * File: ITransferable.h
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Manager for queueing data meant to be uploaded or downloaded at a later time
 *              Primary use cases for DAS and RAMS needing External delayed transfers since
 *              unlike overdrive Cozmo can't assume internet connection.
 *
 *              Because it's valid for a service to delete itself, we don't just want to bind member function
 *              so let this constructor/destructor interface.
 *
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_ITRANSFERABLE_H
#define ANKI_COZMO_ITRANSFERABLE_H

#include <string>
#include "util/signals/simpleSignal_fwd.h"
#include "transferQueueMgr.h"

namespace Util {
  namespace Dispatch {
    class Queue;
  }
}

namespace Anki {
  
  namespace Util {
    
    class ITransferable
    {
    public:
      ITransferable(TransferQueueMgr* transferQueueMgr);
      ~ITransferable();
      
      // Because Listening requires some vitual functions, object must be fully constructed before messages get sent.
      virtual void Init();
      
      // This can occur multiple times.
      virtual void OnTransferReady( TransferQueueMgr::StartRequestFunc funcStartRequest ) = 0;
      
      virtual void OnTransferComplete(const HttpRequest& request,const int responseCode, const std::map<std::string,
                                      std::string>& responseHeaders, const std::vector<uint8_t>& responseBody) {};
    protected:
      Util::Dispatch::Queue* m_DispatchQueue;
      
    private:
      TransferQueueMgr* m_QueueMgr;
      Signal::SmartHandle m_Handle;
    }; 
    
    
  } // namespace Util
} // namespace Anki

#endif // ANKI_COZMO_ITRANSFERABLE_H