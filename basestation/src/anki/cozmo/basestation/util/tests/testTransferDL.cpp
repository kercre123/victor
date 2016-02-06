/**
 * File: testTransferDL.h
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Testcase for push data from the server.
 *              An example might be config downloads or just text banner upsells on the main screen.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "testTransferDL.h"

namespace Anki {
  
  namespace Util {
    
    void TestTransferDL::OnTransferReady( TransferQueueMgr::StartRequestFunc funcStartRequest )
    {
      // If we want to call this a few times in a loop, it's possible.
      // Also if we have no data, do nothing.
      HttpRequestCallback callback =
      std::bind(&ITransferable::OnTransferComplete, this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4);
      
      // Just do a get
      HttpRequest request;
      request.uri = "http://www.google.com";
      request.method = Anki::Util::HttpMethodGet;
      
      funcStartRequest(request, m_DispatchQueue, callback);
    }
    
    void TestTransferDL::OnTransferComplete(const HttpRequest& request,const int responseCode, const std::map<std::string,std::string>& responseHeaders, const std::vector<uint8_t>& responseBody)
    {
      if (!isHttpSuccessCode(responseCode))
      {
        printf("TestTransferDL::OnTransferComplete Error\n");
        return;
      }
      printf("TestTransferDL::OnTransferComplete success\n");
    }

  } // namespace Cozmo
} // namespace Anki