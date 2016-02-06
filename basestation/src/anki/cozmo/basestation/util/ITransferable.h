/**
 * File: ITransferable.h
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Base class for services that need to upload or download data when app has connection.
 *              An example might be config downloads or just text banner upsells on the main screen.
 *              Or uploading save/achievement data.
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
      ITransferable();
      ~ITransferable();
      
      // Because Listening requires some vitual functions, object must be fully constructed before messages get sent.
      virtual void Init(TransferQueueMgr* transferQueueMgr);
      
      // For this to be called, Init had to have been called
      virtual void OnTransferReady( TransferQueueMgr::StartRequestFunc funcStartRequest ) = 0;
      
      virtual void OnTransferComplete(const HttpRequest& request,const int responseCode, const std::map<std::string,
                                      std::string>& responseHeaders, const std::vector<uint8_t>& responseBody) {};
    protected:
      Util::Dispatch::Queue* m_DispatchQueue;
      
    private:
      Signal::SmartHandle m_Handle;
    }; 
    
    
  } // namespace Util
} // namespace Anki

#endif // ANKI_COZMO_ITRANSFERABLE_H