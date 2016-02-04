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

#include "transferQueueMgr.h"
#include "testTransferDL.h"

namespace Anki {
  
  namespace Util {

    TestTransferDL::TestTransferDL(TransferQueueMgr* transferQueueMgr, IHttpAdapter* httpAdapter) : ITransferable( transferQueueMgr )
    {
      
      _dispatchQueue = Util::Dispatch::Create();
    }
    TestTransferDL::~TestTransferDL()
    {
      Util::Dispatch::Release(_dispatchQueue);
    }
    bool TestTransferDL::RequestData(IHttpAdapter* httpAdapter)
    {
      HttpRequestCallback callback =
      [this] (const HttpRequest& request,
              const int responseCode,
              const std::map<std::string, std::string>& responseHeaders,
              const std::vector<uint8_t>& responseBody) {
        if (!isHttpSuccessCode(responseCode))
        {
          printf("Error");
          
          return;
        }
        printf("got a hit!");
      };
      
      // Just do a get
      HttpRequest request;
      request.uri = "http://www.google.com";
      request.method = Anki::Util::HttpMethodGet;
      httpAdapter->StartRequest(request, _dispatchQueue, callback);
      
      return true;
    }

  } // namespace Cozmo
} // namespace Anki