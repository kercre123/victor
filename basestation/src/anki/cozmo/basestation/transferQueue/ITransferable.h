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

namespace Anki {
  
  namespace Util {
    
    class TransferQueueMgr;
    
    class ITransferable
    {
    public:
      ITransferable(TransferQueueMgr* transferQueueMgr);
      ~ITransferable();
      virtual bool SendData( TransferQueueMgr::NativeSendFunc func_native_send ) { return true; };
      
      
      virtual void OnRecieveReadyCallback(std::string data) { };
      //void RequestData( std::string urlpost);
      // void OnDataRecieved
    protected:
      TransferQueueMgr* m_QueueMgr;
      
    private:
      // Handl to emit
      Signal::SmartHandle m_Handle;
      
      Signal::SmartHandle m_UploadHandle;
    }; 
    
    
  } // namespace Util
} // namespace Anki

#endif // ANKI_COZMO_ITRANSFERABLE_H