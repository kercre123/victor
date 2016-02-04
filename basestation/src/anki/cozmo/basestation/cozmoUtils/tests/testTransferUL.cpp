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


#include "transferQueueMgr.h"
#include "TestTransferUL.h"

#include "../http/httpRequest.h"

namespace Anki {
  
  namespace Util {
    
    TestTransferUL::TestTransferUL(TransferQueueMgr* transferQueueMgr) : ITransferable( transferQueueMgr )
    {
      // Use the Context one...
      /*if( transferQueueMgr )
      {
        //Anki::Util::TransferQueueMgr::OnPushFunction func = std::bind(&TestTransferUL::PushData, this);
        std::function<bool (std::string&, std::string&)> func = std::bind(&TestTransferUL::PushData, this, std::placeholders::_1,std::placeholders::_2);
        
        //auto func = std::bind(&TestTransferUL::PushData, this );
        transferQueueMgr->RegisterPushCallback(func);
        
      }*/

    }
    TestTransferUL::~TestTransferUL()
    {
      m_Data = "destroyed now";
      printf("Destroying class %p\n",this);
    }
    
    void TestTransferUL::SetData(std::string str)
    {
      // In a real program it'd probably cache to a file like das or
      // check for being full and throw out oldest.
      m_Data = str;
    }
    
    bool TestTransferUL::SendData( TransferQueueMgr::NativeSendFunc func_native_send )
    {
      printf("called class SendData %p\n",this);
      printf("Yay SendData was called %s\n", m_Data.c_str());
      std::string page_data = m_Data;
      std::string url = "http://localhost";
      
      // Call this as many times as needed to 
      func_native_send(page_data,url);
      
      return true;
    }

  } // namespace Cozmo
} // namespace Anki