/**
 * File: testTransferUL.cpp
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Testcase service to push data to the server using TransferQueueMgr.
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/cozmo/basestation/util/tests/testTransferUL.h"

namespace Anki {
  
  namespace Util {
    
    TestTransferUL::TestTransferUL() : ITransferable( )
    {
      _data = "SomeTestSpamString";
    }
    TestTransferUL::~TestTransferUL()
    {
    }
    
    void TestTransferUL::SetData(std::string str)
    {
      _data = str;
    }
    
    void TestTransferUL::OnTransferReady( TransferQueueMgr::StartRequestFunc funcStartRequest )
    {
      if( _data != "")
      {
        HttpRequestCallback callback =
          std::bind(&ITransferable::OnTransferComplete, this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4);
        HttpRequest request;
        request.uri = "http://localhost:8000";
        request.method = Anki::Util::HttpMethodPost;
        std::map<std::string, std::string> params;
        params["MessageBody"] = _data;
        request.params = params;
        
        funcStartRequest(request, _dispatchQueue, callback);
      }
    }
    
    void TestTransferUL::OnTransferComplete(const HttpRequest& request,const int responseCode, const std::map<std::string,
                            std::string>& responseHeaders, const std::vector<uint8_t>& responseBody)
    {
      if (!isHttpSuccessCode(responseCode))
      {
        printf("TestTransferUL::OnTransferComplete Error\n");
        return;
      }
      _data = "";
      printf("TestTransferUL::OnTransferComplete success\n");
    }
    

  } // namespace Cozmo
} // namespace Anki
