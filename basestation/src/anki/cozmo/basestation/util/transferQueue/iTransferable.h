/**
 * File: iTransferable.h
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

#ifndef __Cozmo_Basestation_Util_TransferQueue_ITransferable_H__
#define __Cozmo_Basestation_Util_TransferQueue_ITransferable_H__

#include <string>
#include "util/signals/simpleSignal_fwd.h"
#include "anki/cozmo/basestation/util/transferQueue/transferQueueMgr.h"

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
      virtual ~ITransferable();
      
      // Because Listening requires some vitual functions, object must be fully constructed before messages get sent.
      virtual void Init(TransferQueueMgr* transferQueueMgr);
      
      // For this to be called, Init had to have been called
      virtual void OnTransferReady( TransferQueueMgr::StartRequestFunc funcStartRequest ) = 0;
      
      virtual void OnTransferComplete(const HttpRequest& request,const int responseCode, const std::map<std::string,
                                      std::string>& responseHeaders, const std::vector<uint8_t>& responseBody) {};
    protected:
      Util::Dispatch::Queue* _dispatchQueue;
      
    private:
      Signal::SmartHandle _handle;
    }; 
    
    
  } // namespace Util
} // namespace Anki

#endif // __Cozmo_Basestation_Util_TransferQueue_ITransferable_H__