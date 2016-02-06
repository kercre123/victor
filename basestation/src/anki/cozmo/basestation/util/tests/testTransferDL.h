/**
 * File: testTransferDL.h
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Testcase service to pull data from the server using TransferQueueMgr.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_TEST_TRANSFER_DL_H
#define ANKI_COZMO_TEST_TRANSFER_DL_H


 #include "../ITransferable.h"

namespace Anki {
  
  namespace Util {
    
    
    class TestTransferDL : public ITransferable
    {
    public:
      virtual void OnTransferReady( TransferQueueMgr::StartRequestFunc funcStartRequest );
      
      virtual void OnTransferComplete(const HttpRequest& request,const int responseCode, const std::map<std::string,
                                      std::string>& responseHeaders, const std::vector<uint8_t>& responseBody);
    protected:
      
    }; // class
    
    
  } // namespace Util
} // namespace Anki

#endif 