/**
 * File: testTransferUL.h
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Testcase service to push data to the server using TransferQueueMgr.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_Util_Tests_TestTransferUL_H__
#define __Cozmo_Basestation_Util_Tests_TestTransferUL_H__

#include "anki/cozmo/basestation/util/transferQueue/iTransferable.h"
#include <string>

namespace Anki {
  
  namespace Util {
    
    class TransferQueueMgr;
    
    class TestTransferUL : public ITransferable
    {
    public:
      TestTransferUL();
      virtual ~TestTransferUL();
      
      virtual void OnTransferReady( TransferQueueMgr::StartRequestFunc funcStartRequest );
      virtual void OnTransferComplete(const HttpRequest& request,const int responseCode, const std::map<std::string,
                                      std::string>& responseHeaders, const std::vector<uint8_t>& responseBody);
      
      void SetData(std::string str);
      std::string GetData() { return _data; }
      
    protected:
      std::string _data;
      
    }; // class
    
    
  } // namespace Util
} // namespace Anki

#endif 