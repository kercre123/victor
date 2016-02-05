/**
 * File: testTransferUL.h
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Testcase for push data from the server.
 *              An example might be wanting to upload a save file to some service for a public facing
 *              achievements leaderboard website to look at.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "TestTransferUL.h"

namespace Anki {
  
  namespace Util {
    
    TestTransferUL::TestTransferUL(TransferQueueMgr* transferQueueMgr) : ITransferable( transferQueueMgr )
    {
      m_Data = "SomeTestSpamString";
    }
    TestTransferUL::~TestTransferUL()
    {
    }
    
    void TestTransferUL::SetData(std::string str)
    {
      m_Data = str;
    }
    
    void TestTransferUL::OnTransferReady( TransferQueueMgr::StartRequestFunc funcStartRequest )
    {
      if( m_Data != "")
      {
        HttpRequestCallback callback =
          std::bind(&ITransferable::OnTransferComplete, this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4);
        HttpRequest request;
        request.uri = "http://localhost:8000";
        request.method = Anki::Util::HttpMethodPost;
        std::map<std::string, std::string> params;
        params["MessageBody"] = m_Data;
        request.params = params;
        
        funcStartRequest(request, m_DispatchQueue, callback);
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
      m_Data = "";
      printf("TestTransferUL::OnTransferComplete success\n");
    }
    

  } // namespace Cozmo
} // namespace Anki